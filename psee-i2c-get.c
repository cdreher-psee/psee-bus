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

#define verbose_printf if(verbose) printf

static void print_usage(char *exec_name, FILE* out_fd) {
	printf("usage: %s [options] I2C_BUS DEV_ADDR REGISTER [NDATA] \n"
		"I2C_BUS: complete path, such as /dev/i2c-1\n"
		"DEV_ADDR: device address (Huahine is 0x3c)\n"
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
	if (optind + 3 > argc) {
		fprintf(stderr, "Missing some arguments.\n");
		print_usage(argv[0], stderr);
		return EXIT_FAILURE;
	}

	/* I2C_BUS */
	i2c_dev_name = argv[optind];
	if ((i2c_dev = open(i2c_dev_name,O_RDWR)) < 0)
	{
		fprintf(stderr, "Failed to open the bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	verbose_printf("I2C bus: %s\n", i2c_dev_name);
	optind++;

	/* DEV_ADDR */
	errno = 0;
	slave_addr = strtol(argv[optind], &endptr, 0);
	if (errno)
	{
		fprintf(stderr, "Failed to parse slave address: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if ((slave_addr < 0x4) || (slave_addr >= 0x78))
	{
		fprintf(stderr, "Addr 0x%x is either invalid or reserved\n", slave_addr);
		return EXIT_FAILURE;
	}
	verbose_printf("Device address: 0x%X\n", slave_addr);
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

	/* check for spare arguments */
	if (optind != argc) {
		fprintf(stderr, "Too many arguments\n");
		print_usage(argv[0], stderr);
		return EXIT_FAILURE;
	}

	/* Actual program starts here */

	if (ioctl(i2c_dev,I2C_SLAVE,slave_addr) < 0)
	{
		fprintf(stderr, "Failed to acquire bus access or slave: %s\n", strerror(errno));
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
		read_bytes = read(i2c_dev,buffer,ndata*(sizeof(buffer[0])));
	}
	if (read_bytes != ndata*(sizeof(buffer[0])))
	{
		fprintf(stderr, "Failed to read on the i2c bus (got %ld): %s\n", read_bytes, strerror(errno));
		ret = EXIT_FAILURE;
	}
	else
	{
		verbose_printf("Read %ld bytes with status %s\n", read_bytes, strerror(errno));
		ret = EXIT_SUCCESS;
	}

	for (ndata = 0; ndata < (read_bytes / sizeof(buffer[0])); ndata++)
	{
		if (verbose)
			printf("data[%d] = 0x%08x\n", ndata, be32toh(buffer[ndata]));
		else
			printf("0x%08x\n", be32toh(buffer[ndata]));
	}

	return ret;
}
