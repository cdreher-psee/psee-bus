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

static void print_usage(char *exec_name) {
	printf("usage: %s [options] SPI_DEV REGISTER [VALUE] ...\n"
		"SPI_DEV: complete path, such as /dev/spidev1.0\n"
		"REGISTER: address of the first register to be written\n"
		"VALUE: 32-bits values to write in registers\n"
		"options:\n"
		"\t-n:\tdry run: don't actually write the memory\n"
		"\t-h:\tdisplay this message and quit with success\n",
		exec_name);
}

int main(int argc, char* argv[])
{
	char* spi_dev_name;
	int spi_dev;
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
	while ((opt = getopt(argc, argv, "nh")) != -1) {
		switch (opt) {
		case 'n':
			dry = 1;
			break;
		case 'h':
			print_usage(argv[0]);
			return EXIT_SUCCESS;
			break;
		default:
			print_usage(argv[0]);
			return EXIT_FAILURE;
			break;
		}
	}

	/* check for enough mandatory arguments */
	if (optind + 2 > argc) {
		printf("Missing some arguments.\n");
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* SPI_DEV */
	spi_dev_name = argv[optind];
	if ((spi_dev = open(spi_dev_name,O_RDWR)) < 0)
	{
		printf("Failed to open the bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	optind++;

	/* REGISTER */
	errno = 0;
	reg_addr = strtol(argv[optind], &endptr, 0);
	if (errno)
	{
		printf("Failed to parse reg address: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	buffer[0] = htobe32(reg_addr);
	optind++;

	/* VALUE */
	ndata = 0;
	while (optind < argc)
	{
		uint32_t val = strtol(argv[optind], &endptr, 0);
		if (errno)
		{
			printf("Failed to parse value %d: %s\n", ndata, strerror(errno));
			return EXIT_FAILURE;
		}
		if (ndata >= MAX_WRITE)
		{
			printf("Number of reg values exceeds %d\n", MAX_WRITE);
			return EXIT_FAILURE;
		}
		buffer[1+ndata] = htobe32(val);
		optind++;
		ndata++;
	}

	/* Actual program starts here */

	memset(xfer, 0, sizeof xfer);
	xfer[0].tx_buf = (uint64_t)&buffer[0];
	xfer[0].len = sizeof(buffer[0]);

	xfer[1].tx_buf = (uint64_t)&buffer[1];
	xfer[1].len = ndata*sizeof(buffer[0]);

	if (!dry && (ioctl(spi_dev, SPI_IOC_MESSAGE(2), xfer)!=0))
	{
		printf("Failed to write to the spi bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	printf("SPI write command for %d registers succesfully sent.\n", ndata);
	return EXIT_SUCCESS;
}
