// SPDX-License-Identifier: GPL-2.0
/*
 * sbs_check - verify that a capture device delivers the robot dog's stereo
 * layout: one YUY2 frame, left eye in x [0, w/2), right eye in [w/2, w).
 *
 *   sbs_check <dev> <frames>
 *
 * Against vcam it checks, for every line of every frame:
 *   - {seq, ~seq} at x = 0 and {~seq, seq} at x = w/2 (eye markers)
 *   - the left eye is colour bars (chroma away from 128)
 *   - the right eye is a grey ramp (chroma exactly 128)
 * which is exactly the assumption stereo_splitter makes when it crops the
 * left half.  Exit status 0 means every frame passed.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define NBUF 4

static uint32_t rd32(const uint8_t *p)
{
	uint32_t v;

	memcpy(&v, p, 4);
	return v;
}

static int check_frame(const uint8_t *f, unsigned w, unsigned h, unsigned stride)
{
	uint32_t seq = rd32(f);
	unsigned half = w / 2, y, x, coloured = 0, total = 0;

	for (y = 0; y < h; y++) {
		const uint8_t *row = f + (size_t)y * stride;
		const uint8_t *r = row + half * 2;

		if (rd32(row) != seq || rd32(row + 4) != ~seq)
			return 1;	/* left marker */
		if (rd32(r) != ~seq || rd32(r + 4) != seq)
			return 2;	/* right marker */
		for (x = 4; x + 1 < half; x += 2) {	/* skip the marker bytes */
			int lu = row[x * 2 + 1], lv = row[x * 2 + 3];
			int ru = r[x * 2 + 1], rv = r[x * 2 + 3];

			total++;
			if (abs(lu - 128) + abs(lv - 128) > 20)
				coloured++;
			if (ru != 128 || rv != 128)
				return 3;	/* right eye must be grey */
		}
	}
	return coloured * 2 > total ? 0 : 4;	/* left eye must be colourful */
}

int main(int argc, char **argv)
{
	struct v4l2_format fmt = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
	struct v4l2_requestbuffers req = { .count = NBUF,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE, .memory = V4L2_MEMORY_MMAP };
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	uint8_t *map[NBUF];
	unsigned frames, i, got = 0, bad = 0;
	int fd;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <dev> <frames>\n", argv[0]);
		return 2;
	}
	frames = strtoul(argv[2], NULL, 0);
	fd = open(argv[1], O_RDWR);
	if (fd < 0 || ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
		perror(argv[1]);
		return 2;
	}
	if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
		fprintf(stderr, "not YUYV\n");
		return 2;
	}
	if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
		perror("REQBUFS");
		return 2;
	}
	for (i = 0; i < req.count; i++) {
		struct v4l2_buffer b = { .type = req.type, .memory = req.memory,
					 .index = i };

		if (ioctl(fd, VIDIOC_QUERYBUF, &b) < 0)
			return 2;
		map[i] = mmap(NULL, b.length, PROT_READ, MAP_SHARED, fd, b.m.offset);
		if (map[i] == MAP_FAILED || ioctl(fd, VIDIOC_QBUF, &b) < 0)
			return 2;
	}
	if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
		perror("STREAMON");
		return 2;
	}
	while (got < frames) {
		struct v4l2_buffer b = { .type = req.type, .memory = req.memory };
		struct pollfd p = { .fd = fd, .events = POLLIN };
		int why;

		if (poll(&p, 1, 3000) <= 0 || ioctl(fd, VIDIOC_DQBUF, &b) < 0) {
			fprintf(stderr, "capture stalled\n");
			break;
		}
		got++;
		why = check_frame(map[b.index], fmt.fmt.pix.width,
				  fmt.fmt.pix.height, fmt.fmt.pix.bytesperline);
		if (why) {
			bad++;
			if (bad == 1)
				fprintf(stderr, "frame %u failed check %d\n",
					b.sequence, why);
		}
		ioctl(fd, VIDIOC_QBUF, &b);
	}
	ioctl(fd, VIDIOC_STREAMOFF, &type);
	printf("layout=sbs %ux%u frames=%u bad=%u\n", fmt.fmt.pix.width,
	       fmt.fmt.pix.height, got, bad);
	return (got == frames && !bad) ? 0 : 1;
}
