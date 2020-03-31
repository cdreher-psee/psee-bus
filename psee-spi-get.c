/*******************************************************************
 * File : psee-spi-get.c                                           *
 *                                                                 *
 * Copyright: (c) 2020 Prophesee                                   *
 *******************************************************************/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_READ 16

#define verbose_printf if(verbose) printf

static void print_usage(char *exec_name, FILE* out_fd) {
	fprintf(out_fd, "usage: %s [options] SPI_DEV REGISTER [NDATA] \n"
		"SPI_DEV: complete path, such as /dev/spidev1.0\n"
		"REGISTER: address of the first register to be read\n"
		"NDATA: number of 32-bits registers to read (default: 1)\n"
		"options:\n"
		"\t-n:\tdry run: don't actually read the registers\n"
		"\t-v:\tverbose: display transfer information on stdout\n"
		"\t-h:\tdisplay this message and quit with success\n",
		exec_name);
}

int main(int argc, char* argv[])
{
	int ret = 0;
	int verbose = 0;
	char* spi_dev_name;
	int spi_dev;
	int mode = SPI_MODE_3;
	struct spi_ioc_transfer xfer[2];
	uint32_t reg_addr;
	int ndata = 1;
	uint32_t buffer[MAX_READ];
	/* for getopt */
	int opt;
	int dry = 0;
	int i = 0;
	/* for strtol */
	char* endptr;

	/* options */
	while ((opt = getopt(argc, argv, "nvh")) != -1) {
		switch (opt) {
		case 'n':
			dry = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			print_usage(argv[0], stdout);
			return EXIT_SUCCESS;
			break;
		default:
			print_usage(argv[0], stderr);
			return EXIT_FAILURE;
			break;
		}
	}

	if (dry) verbose_printf("-- dry run --\n");

	/* check for enough mandatory arguments */
	if (optind + 2 > argc) {
		fprintf(stderr, "Missing some arguments.\n");
		print_usage(argv[0], stderr);
		return EXIT_FAILURE;
	}

	/* SPI_DEV */
	spi_dev_name = argv[optind];
	if ((spi_dev = open(spi_dev_name,O_RDWR)) < 0)
	{
		fprintf(stderr, "Failed to open the bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	verbose_printf("SPI device: %s\n", spi_dev_name);
	optind++;

	/* REGISTER */
	errno = 0;
	reg_addr = strtol(argv[optind], &endptr, 0);
	if (errno)
	{
		fprintf(stderr, "Failed to parse reg address: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	verbose_printf("Register: 0x%X\n", reg_addr);
	/* mutate address to add command */
	reg_addr >>= 2;
	reg_addr |= (1lu << 31); /* read operation */
	optind++;

	/* NDATA */
	errno = 0;
	if (optind != argc)
	{
		ndata = strtol(argv[optind], &endptr, 0);
		optind++;
	}
	if (errno)
	{
		fprintf(stderr, "Failed to parse NDATA: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if ((ndata <= 0) || (ndata > MAX_READ))
	{
		fprintf(stderr, "Can't transfer %d data\n", ndata);
		return EXIT_FAILURE;
	}
	verbose_printf("ndata: %d\n", ndata);
	reg_addr |= ((ndata>1) << 30); /* burst operation */

	/* check for spare arguments */
	if (optind != argc) {
		fprintf(stderr, "Too many arguments\n");
		print_usage(argv[0], stderr);
		return EXIT_FAILURE;
	}

	/* Actual program starts here */
	if (ioctl(spi_dev, SPI_IOC_WR_MODE, &mode)!=0)
	{
		fprintf(stderr, "Failed to set SPI device mode: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	buffer[0] = htobe32(reg_addr);
	memset(xfer, 0, sizeof xfer);
	xfer[0].tx_buf = (uint64_t)&buffer[0];
	xfer[0].len = sizeof(buffer[0]);
	xfer[0].delay_usecs = 4; //need to wait 64 clk before read 3.2us@20MHz

	xfer[1].rx_buf = (uint64_t)&buffer[0];
	xfer[1].len = ndata*sizeof(buffer[0]);

	if (!dry)
		ret = ioctl(spi_dev, SPI_IOC_MESSAGE(2), xfer);
	if (ret != (ndata+1)*sizeof(buffer[0]))
	{
		fprintf(stderr, "Failed to read on the spi bus %d: %s\n", ret, strerror(errno));
		return EXIT_FAILURE;
	}

	verbose_printf("SPI read result: %s\n", strerror(errno));
	for (i = 0; i < ndata; i++)
	{
		if (verbose)
			printf("data[%d] = 0x%08x\n", i, be32toh(buffer[i]));
		else
			printf("0x%08x\n", be32toh(buffer[i]));
	}

	return EXIT_SUCCESS;
}
