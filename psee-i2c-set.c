/*******************************************************************
 * File : psee-i2c-set.c                                           *
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

#define MAX_WRITE 16

static void print_usage(char *exec_name) {
	printf("usage: %s [options] I2C_BUS DEV_ADDR REGISTER [VALUE] ...\n"
		"I2C_BUS: complete path, such as /dev/i2c-1\n"
		"DEV_ADDR: device address (Huahine is 0x3c)\n"
		"REGISTER: address of the first register to be read\n"
		"VALUE: 32-bits values to write in registers\n"
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
	int ndata;
	ssize_t write_bytes = 0;
	ssize_t written_bytes = 0;
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

	if (ioctl(i2c_dev,I2C_SLAVE,slave_addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	write_bytes = (ndata+1)*sizeof(buffer[0]);
	if (!dry && (written_bytes = write(i2c_dev,buffer,write_bytes)) != write_bytes)
	{
		printf("Failed to write to the i2c bus (written %ld, expected %ld): %s\n",
			written_bytes, write_bytes, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("I2C write command for %d registers succesfully sent.\n", ndata);
	return EXIT_SUCCESS;
}
