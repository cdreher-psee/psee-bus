/*******************************************************************
 * File : psee-i2c-get.c                                           *
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
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_READ 16

static void print_usage(char *exec_name) {
	printf("usage: %s [options] I2C_BUS DEV_ADDR REGISTER [NDATA] \n"
		"I2C_BUS: complete path, such as /dev/i2c-1\n"
		"DEV_ADDR: device address (Huahine is 0x3c)\n"
		"REGISTER: address of the first register to be read\n"
		"NDATA: number of 32-bits registers to read (default: 1)\n"
		"options:\n"
		"\t-n:\tdry run: don't actually write the memory\n"
		"\t-h:\tdisplay this message and quit with success\n",
		exec_name);
}

int main(int argc, char* argv[])
{
	char* i2c_dev_name;
	int i2c_dev;
	int slave_addr;
	uint32_t reg_addr;
	int ndata = 1;
	ssize_t read_bytes = 0;
	uint32_t buffer[MAX_READ];
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
	if (optind + 3 >= argc) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* I2C_BUS */
	i2c_dev_name = argv[optind];
	if ((i2c_dev = open(i2c_dev_name,O_RDWR)) < 0)
	{
		printf("Failed to open the bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	optind++;

	/* DEV_ADDR */
	errno = 0;
	slave_addr = strtol(argv[optind], &endptr, 0);
	if (errno)
	{
		printf("Failed to parse slave address: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if ((slave_addr < 0x4) || (slave_addr >= 0x78))
	{
		printf("Addr 0x%x is either invalid or reserved\n", slave_addr);
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
	optind++;

	/* NDATA */
	errno = 0;
	if (optind != argc)
		ndata = strtol(argv[optind], &endptr, 0);
	if (errno)
	{
		printf("Failed to parse NDATA: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if ((ndata <= 0) || (ndata > MAX_READ))
	{
		printf("Can't transfer %d data\n", ndata);
		return EXIT_FAILURE;
	}
	optind++;

	/* check for spare arguments */
	if (optind != argc) {
		printf("Too many arguments\n");
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}


	/* Actual program starts here */

	if (ioctl(i2c_dev,I2C_SLAVE,slave_addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	buffer[0] = htobe32(reg_addr);
	if (!dry && write(i2c_dev,buffer,sizeof(buffer[0])) != sizeof(buffer[0]))
	{
		printf("Failed to write to the i2c bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	if (!dry)
	{
		read_bytes =  read(i2c_dev,buffer,ndata);
	}
	printf("Read %ld bytes with status %s\n", read_bytes, strerror(errno));
	for (ndata = 0; ndata < (read_bytes / sizeof(buffer[0])); ndata++)
	{
		printf("data[%d] = 0x%08x\n", ndata, be32toh(buffer[ndata]));
	}

	return EXIT_SUCCESS;
}
