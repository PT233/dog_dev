// SPDX-License-Identifier: GPL-2.0
/*
 * qbuf_race - reproduce and measure the "re-queue a V4L2 buffer while the
 * consumer still reads it" bug, and compare the two usual fixes.
 *
 *   qbuf_race <dev> <bug|copy|hold> <frames> <work_ms> [-b nbufs] [-s WxH] [-x]
 *
 *   bug   QBUF straight after DQBUF, then keep using the mapped pointer
 *         (what an async encoder thread does with a raw V4L2 buffer)
 *   copy  deep-copy the frame, QBUF at once, process the private copy
 *   hold  keep the buffer until processing is finished, then QBUF
 *   -x    map the buffers through VIDIOC_EXPBUF dma-buf fds instead of the
 *         V4L2 mmap offset (the zero-copy sharing path)
 *
 * Detection relies on the driver writing {seq, ~seq} at the start of every
 * line.  A frame is "corrupted" when that marker changed under the consumer's
 * hands, "torn" when lines of one frame disagree with each other.
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
#include <time.h>
#include <unistd.h>

#define MAXBUF 16

static uint64_t now_ns(void)
{
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec;
}

static uint32_t rd32(const uint8_t *p)
{
	uint32_t v;

	memcpy(&v, p, 4);
	return v;
}

/* 1 = every line carries the same valid marker, writes it to *seq */
static int frame_consistent(const uint8_t *f, unsigned h, unsigned stride,
			    uint32_t *seq)
{
	uint32_t s = rd32(f);
	unsigned y;

	if (rd32(f + 4) != ~s)
		return 0;
	for (y = 1; y < h; y++) {
		const uint8_t *row = f + (size_t)y * stride;

		if (rd32(row) != s || rd32(row + 4) != ~s)
			return 0;
	}
	*seq = s;
	return 1;
}

static int xioctl(int fd, unsigned long req, void *arg)
{
	int r;

	do {
		r = ioctl(fd, req, arg);
	} while (r < 0 && errno == EINTR);
	return r;
}

int main(int argc, char **argv)
{
	const char *mode;
	unsigned frames, work_ms, nbuf = 4, w = 0, h = 0, i;
	int use_expbuf = 0, fd;
	struct v4l2_format fmt = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
	struct v4l2_requestbuffers req = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
					   .memory = V4L2_MEMORY_MMAP };
	uint8_t *map[MAXBUF];
	size_t len;
	uint8_t *priv;
	unsigned long got = 0, corrupted = 0, torn = 0, gaps = 0, copies = 0;
	uint64_t copy_ns = 0, t0;
	uint32_t last_seq = 0;
	int have_last = 0;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (argc < 5) {
		fprintf(stderr, "usage: %s <dev> <bug|copy|hold> <frames> <work_ms> "
			"[-b nbufs] [-s WxH] [-x]\n", argv[0]);
		return 2;
	}
	mode = argv[2];
	frames = strtoul(argv[3], NULL, 0);
	work_ms = strtoul(argv[4], NULL, 0);
	for (i = 5; i < (unsigned)argc; i++) {
		if (!strcmp(argv[i], "-b") && i + 1 < (unsigned)argc)
			nbuf = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-s") && i + 1 < (unsigned)argc)
			sscanf(argv[++i], "%ux%u", &w, &h);
		else if (!strcmp(argv[i], "-x"))
			use_expbuf = 1;
	}
	if (strcmp(mode, "bug") && strcmp(mode, "copy") && strcmp(mode, "hold"))
		return 2;
	if (nbuf < 2 || nbuf > MAXBUF)
		return 2;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	if (xioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
		perror("G_FMT");
		return 1;
	}
	if (w && h) {
		fmt.fmt.pix.width = w;
		fmt.fmt.pix.height = h;
		if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
			perror("S_FMT");
			return 1;
		}
	}
	len = fmt.fmt.pix.sizeimage;
	priv = malloc(len);

	req.count = nbuf;
	if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
		perror("REQBUFS");
		return 1;
	}
	nbuf = req.count;

	for (i = 0; i < nbuf; i++) {
		struct v4l2_buffer b = { .type = req.type, .memory = req.memory,
					 .index = i };

		if (xioctl(fd, VIDIOC_QUERYBUF, &b) < 0) {
			perror("QUERYBUF");
			return 1;
		}
		if (use_expbuf) {
			struct v4l2_exportbuffer e = { .type = req.type,
						       .index = i,
						       .flags = O_RDWR };

			if (xioctl(fd, VIDIOC_EXPBUF, &e) < 0) {
				perror("EXPBUF");
				return 1;
			}
			map[i] = mmap(NULL, b.length, PROT_READ | PROT_WRITE,
				      MAP_SHARED, e.fd, 0);
			close(e.fd);	/* the mapping keeps the dma-buf alive */
		} else {
			map[i] = mmap(NULL, b.length, PROT_READ | PROT_WRITE,
				      MAP_SHARED, fd, b.m.offset);
		}
		if (map[i] == MAP_FAILED) {
			perror("mmap");
			return 1;
		}
		if (xioctl(fd, VIDIOC_QBUF, &b) < 0) {
			perror("QBUF");
			return 1;
		}
	}

	if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
		perror("STREAMON");
		return 1;
	}

	t0 = now_ns();
	while (got < frames) {
		struct v4l2_buffer b = { .type = req.type, .memory = req.memory };
		struct pollfd p = { .fd = fd, .events = POLLIN };
		const uint8_t *frame;
		uint32_t seq0, seq1;
		unsigned ph = fmt.fmt.pix.height, stride = fmt.fmt.pix.bytesperline;

		if (poll(&p, 1, 3000) <= 0) {
			fprintf(stderr, "poll timeout\n");
			break;
		}
		if (xioctl(fd, VIDIOC_DQBUF, &b) < 0) {
			perror("DQBUF");
			break;
		}
		got++;
		if (have_last && b.sequence != last_seq + 1)
			gaps += b.sequence - last_seq - 1;
		last_seq = b.sequence;
		have_last = 1;

		frame = map[b.index];
		if (!frame_consistent(frame, ph, stride, &seq0)) {
			torn++;
			xioctl(fd, VIDIOC_QBUF, &b);
			continue;
		}

		if (!strcmp(mode, "bug")) {
			xioctl(fd, VIDIOC_QBUF, &b);	/* returned too early */
			usleep(work_ms * 1000);
			if (!frame_consistent(frame, ph, stride, &seq1))
				torn++;
			else if (seq1 != seq0)
				corrupted++;
		} else if (!strcmp(mode, "copy")) {
			uint64_t c0 = now_ns();

			memcpy(priv, frame, len);
			copy_ns += now_ns() - c0;
			copies++;
			xioctl(fd, VIDIOC_QBUF, &b);
			usleep(work_ms * 1000);
			if (!frame_consistent(priv, ph, stride, &seq1) ||
			    seq1 != seq0)
				corrupted++;
		} else {
			usleep(work_ms * 1000);
			if (!frame_consistent(frame, ph, stride, &seq1) ||
			    seq1 != seq0)
				corrupted++;
			xioctl(fd, VIDIOC_QBUF, &b);
		}
	}
	{
		double secs = (now_ns() - t0) / 1e9;

		printf("mode=%s %ux%u bufs=%u work=%ums%s frames=%lu corrupted=%lu "
		       "torn=%lu seq_gaps=%lu", mode, fmt.fmt.pix.width,
		       fmt.fmt.pix.height, nbuf, work_ms, use_expbuf ? " expbuf" : "",
		       got, corrupted, torn, gaps);
		if (copies)
			printf(" copy_avg=%.0fus", copy_ns / 1e3 / copies);
		printf(" rate=%.1ffps\n", got / secs);
	}
	xioctl(fd, VIDIOC_STREAMOFF, &type);
	close(fd);
	return 0;
}
