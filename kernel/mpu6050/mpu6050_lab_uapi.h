/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * mpu6050_lab user-space ABI.
 *
 * read() on /dev/mpu6050_<bus>-<addr> returns an integral number of
 * struct mpu6050_lab_sample records.  The layout has no implicit padding so
 * it is identical on 32-bit (armhf) and 64-bit user space.
 */
#ifndef _UAPI_MPU6050_LAB_H
#define _UAPI_MPU6050_LAB_H

#include <linux/types.h>

struct mpu6050_lab_sample {
	__u64 timestamp_ns;	/* CLOCK_MONOTONIC, taken on IRQ/timer entry */
	__u32 seq;		/* counts delivered samples, wraps at 2^32 */
	__s16 accel[3];		/* X, Y, Z raw, +-2 g  -> 16384 LSB/g */
	__s16 temp;		/* raw, deg C = raw / 340 + 36.53 */
	__s16 gyro[3];		/* X, Y, Z raw, +-250 dps -> 131 LSB/dps */
	__u16 reserved;
	/*
	 * Samples lost since the previous one: events coalesced by a late
	 * thread, I2C errors or FIFO overflow.
	 */
	__u32 dropped_before;
};

#define MPU6050_LAB_ACCEL_LSB_PER_G	16384
#define MPU6050_LAB_GYRO_LSB_PER_DPS	131

#endif /* _UAPI_MPU6050_LAB_H */
