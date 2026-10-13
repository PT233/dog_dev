// SPDX-License-Identifier: GPL-2.0
/*
 * stub_set - write one register of an i2c-stub chip with SMBus "write byte
 * data", so the test needs neither i2c-tools nor libi2c.
 *
 *   stub_set <bus> <addr> <reg> <val> [<reg> <val> ...]
 */
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int smbus_write_byte(int fd, unsigned char reg, unsigned char val)
{
	union i2c_smbus_data d = { .byte = val };
	struct i2c_smbus_ioctl_data a = {
		.read_write = I2C_SMBUS_WRITE,
		.command = reg,
		.size = I2C_SMBUS_BYTE_DATA,
		.data = &d,
	};

	return ioctl(fd, I2C_SMBUS, &a);
}

int main(int argc, char **argv)
{
	char path[32];
	int fd, i;

	if (argc < 5 || (argc - 3) % 2) {
		fprintf(stderr, "usage: %s <bus> <addr> <reg> <val> ...\n", argv[0]);
		return 2;
	}
	snprintf(path, sizeof(path), "/dev/i2c-%d", atoi(argv[1]));
	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return 1;
	}
	if (ioctl(fd, I2C_SLAVE_FORCE, strtoul(argv[2], NULL, 0)) < 0) {
		perror("I2C_SLAVE_FORCE");
		return 1;
	}
	for (i = 3; i + 1 < argc; i += 2) {
		if (smbus_write_byte(fd, strtoul(argv[i], NULL, 0),
				     strtoul(argv[i + 1], NULL, 0)) < 0) {
			perror("smbus write");
			return 1;
		}
	}
	close(fd);
	return 0;
}
