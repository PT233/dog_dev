// SPDX-License-Identifier: GPL-2.0
/*
 * ctrl_query - send one UVCIOC_CTRL_QUERY and print the reply.
 *
 *   ctrl_query <dev> <unit> <selector> <query> <size>
 *
 * query: 0x81 GET_CUR, 0x82 GET_MIN, 0x83 GET_MAX, 0x85 GET_LEN, 0x86 GET_INFO
 * Works against both uvcvideo and dogcam, which is the point: vendor scripts
 * that talk to a camera's units this way do not care which driver is bound.
 */
#include <fcntl.h>
#include <linux/uvcvideo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
	unsigned char data[64] = { 0 };
	struct uvc_xu_control_query q = { .data = data };
	int fd, i;

	if (argc < 6) {
		fprintf(stderr, "usage: %s <dev> <unit> <selector> <query> <size>\n",
			argv[0]);
		return 2;
	}
	q.unit = strtoul(argv[2], NULL, 0);
	q.selector = strtoul(argv[3], NULL, 0);
	q.query = strtoul(argv[4], NULL, 0);
	q.size = strtoul(argv[5], NULL, 0);
	if (q.size > sizeof(data))
		return 2;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror(argv[1]);
		return 1;
	}
	if (ioctl(fd, UVCIOC_CTRL_QUERY, &q) < 0) {
		perror("UVCIOC_CTRL_QUERY");
		return 1;
	}
	for (i = 0; i < q.size; i++)
		printf("%02x%s", data[i], i + 1 < q.size ? " " : "\n");
	return 0;
}
