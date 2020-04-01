/*******************************************************************
 * File : psee-spi-set.c                                           *
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

#define MAX_WRITE 16

#define verbose_printf if(verbose) printf

static void print_usage(char *exec_name, FILE* out_fd) {
	fprintf(out_fd, "usage: %s [options] SPI_DEV REGISTER [VALUE] ...\n"
		"SPI_DEV: complete path, such as /dev/spidev1.0\n"
		"REGISTER: address of the first register to be written\n"
		"VALUE: 32-bits values to write in registers\n"
		"options:\n"
		"\t-f:\tfreq: specify the maximum frequency (in Hz) the device supports\n"
		"\t-n:\tdry run: don't actually write the registers\n"
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
	char* max_freq_str = NULL;
	int max_freq_hz = 0;
	struct spi_ioc_transfer xfer[2];
	uint32_t reg_addr;
	int ndata;
	uint32_t buffer[1+MAX_WRITE];
	/* for getopt */
	int opt;
	int dry = 0;
	/* for strtol */
	char* endptr;

	/* options */
	while ((opt = getopt(argc, argv, "f:nvh")) != -1) {
		switch (opt) {
		case 'f':
			max_freq_str = optarg;
			break;
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

	/* MAX FREQ */
	errno = 0;
	if (max_freq_str)
	{
		max_freq_hz = strtol(max_freq_str, &endptr, 0);
	}
	if (errno)
	{
		fprintf(stderr, "Failed to parse frequency: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	verbose_printf("Max device frequency: %d Hz\n", max_freq_hz);

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
	reg_addr |= (0lu << 31); /* write operation */
	optind++;

	/* VALUE */
	ndata = 0;
	while (optind < argc)
	{
		uint32_t val = strtol(argv[optind], &endptr, 0);
		if (errno)
		{
			fprintf(stderr, "Failed to parse value %d: %s\n", ndata, strerror(errno));
			return EXIT_FAILURE;
		}
		if (ndata >= MAX_WRITE)
		{
			fprintf(stderr, "Number of reg values exceeds %d\n", MAX_WRITE);
			return EXIT_FAILURE;
		}
		buffer[1+ndata] = htobe32(val);
		optind++;
		ndata++;
	}
	verbose_printf("ndata: %d\n", ndata);
	reg_addr |= ((ndata>1) << 30); /* burst operation */


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
	xfer[0].speed_hz = max_freq_hz;

	xfer[1].tx_buf = (uint64_t)&buffer[1];
	xfer[1].len = ndata*sizeof(buffer[0]);
	xfer[1].speed_hz = max_freq_hz;

	if (!dry)
		ret = ioctl(spi_dev, SPI_IOC_MESSAGE(2), xfer);
	if (ret != (ndata+1)*sizeof(buffer[0]))
	{
		fprintf(stderr, "Failed to write to the spi bus %d: %s\n", ret, strerror(errno));
		return EXIT_FAILURE;
	}

	verbose_printf("SPI write command for %d registers succesfully sent.\n", ndata);
	return EXIT_SUCCESS;
}
