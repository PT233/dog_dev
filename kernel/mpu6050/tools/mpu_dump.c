// SPDX-License-Identifier: GPL-2.0
/*
 * mpu_dump - read samples from /dev/mpu6050_* and report interval statistics.
 *
 *   mpu_dump <dev> <count> [-v] [-n]
 *     -v  print every sample
 *     -n  O_NONBLOCK + poll(), exercises the poll path
 *     -d <ms>  sleep after open before reading (forces FIFO overflow)
 *
 * The interval column is the difference of the driver-side timestamps (taken
 * in the IRQ/timer entry), not user-space arrival time, so it measures the
 * sampling jitter of the source and not scheduling latency of this program.
 */
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../mpu6050_lab_uapi.h"

int main(int argc, char **argv)
{
	struct mpu6050_lab_sample s;
	int verbose = 0, nonblock = 0, delay_ms = 0, fd, i;
	unsigned long want, got = 0;
	uint64_t prev = 0;
	double sum = 0, sumsq = 0, min = 1e18, max = 0;
	unsigned long n_int = 0, dropped = 0, seq_gaps = 0;
	uint32_t exp_seq = 0;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <dev> <count> [-v] [-n]\n", argv[0]);
		return 2;
	}
	want = strtoul(argv[2], NULL, 0);
	for (i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "-v"))
			verbose = 1;
		else if (!strcmp(argv[i], "-n"))
			nonblock = 1;
		else if (!strcmp(argv[i], "-d") && i + 1 < argc)
			delay_ms = atoi(argv[++i]);
	}

	fd = open(argv[1], O_RDONLY | (nonblock ? O_NONBLOCK : 0));
	if (fd < 0) {
		perror("open");
		return 1;
	}

	if (delay_ms)
		usleep(delay_ms * 1000);

	while (got < want) {
		ssize_t r;

		if (nonblock) {
			struct pollfd p = { .fd = fd, .events = POLLIN };

			if (poll(&p, 1, 2000) <= 0) {
				fprintf(stderr, "poll timeout\n");
				break;
			}
			if (p.revents & (POLLHUP | POLLERR)) {
				fprintf(stderr, "device gone\n");
				break;
			}
		}
		r = read(fd, &s, sizeof(s));
		if (r < 0) {
			if (errno == EAGAIN)
				continue;
			perror("read");
			break;
		}
		if (r != sizeof(s)) {
			fprintf(stderr, "short read %zd\n", r);
			break;
		}
		got++;
		dropped += s.dropped_before;
		if (got > 1 && s.seq != exp_seq)
			seq_gaps++;
		exp_seq = s.seq + 1;

		if (prev) {
			double d = (double)(s.timestamp_ns - prev) / 1000.0;

			sum += d;
			sumsq += d * d;
			n_int++;
			if (d < min)
				min = d;
			if (d > max)
				max = d;
		}
		prev = s.timestamp_ns;

		if (verbose)
			printf("seq=%u t=%llu a=%6d %6d %6d  g=%6d %6d %6d  T=%.2fC drop=%u\n",
			       s.seq, (unsigned long long)s.timestamp_ns,
			       s.accel[0], s.accel[1], s.accel[2],
			       s.gyro[0], s.gyro[1], s.gyro[2],
			       s.temp / 340.0 + 36.53, s.dropped_before);
	}

	if (n_int) {
		double mean = sum / n_int;
		double sd = sqrt(sumsq / n_int - mean * mean);

		printf("samples=%lu  interval_us: mean=%.1f sd=%.1f min=%.1f max=%.1f"
		       "  dropped=%lu seq_gaps=%lu\n",
		       got, mean, sd, min, max, dropped, seq_gaps);
	} else {
		printf("samples=%lu\n", got);
	}
	close(fd);
	return 0;
}
