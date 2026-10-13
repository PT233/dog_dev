// SPDX-License-Identifier: GPL-2.0
/*
 * crbufs_probe - show why v4l2-compliance from an older v4l-utils reports
 * "check_0(crbufs.reserved)" on kernels >= 6.8.
 *
 * Kernel 6.8 carved `flags` and `max_num_buffers` out of the old
 * v4l2_create_buffers.reserved[].  This program is compiled against the *old*
 * userspace layout on purpose and prints those words.  Exit status 0 means
 * "reserved[1] holds max_num_buffers" i.e. the report is an ABI version skew.
 */
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
	struct v4l2_create_buffers cb;
	int fd, i;

	if (argc < 2)
		return 2;
	fd = open(argv[1], O_RDWR);
	if (fd < 0)
		return 2;
	memset(&cb, 0, sizeof(cb));
	cb.memory = V4L2_MEMORY_MMAP;
	cb.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_FMT, &cb.format) ||
	    ioctl(fd, VIDIOC_CREATE_BUFS, &cb))
		return 2;
	printf("capabilities=0x%x reserved[] =", cb.capabilities);
	for (i = 0; i < (int)(sizeof(cb.reserved) / sizeof(cb.reserved[0])); i++)
		printf(" %u", cb.reserved[i]);
	printf("\n");
	return !(cb.reserved[0] == 0 && cb.reserved[1] != 0);
}
