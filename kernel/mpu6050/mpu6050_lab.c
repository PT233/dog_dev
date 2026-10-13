// SPDX-License-Identifier: GPL-2.0
/*
 * mpu6050_lab - MPU-6050 six-axis IMU, I2C client driver (Linux 5.15 API)
 *
 *  - device-tree binding "hyq,mpu6050-lab": reg, interrupts (DATA_RDY)
 *  - regmap based register access, one 14-byte burst per sample
 *  - hard IRQ only stamps ktime; the I2C burst runs in the IRQ thread
 *  - without an "interrupts" property it falls back to an hrtimer + work
 *  - single-producer / single-consumer kfifo, drop-newest on overflow
 *  - char device (read/poll) exported through a misc device
 *  - runtime PM: the chip sleeps unless the node is open
 *
 * Lifetime: struct mpu is reference counted because an open file may outlive
 * the I2C client (rmmod, overlay unload).  After remove() the file only ever
 * sees m->dead and never touches the regmap or the client.
 */
#include <linux/atomic.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/kref.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/poll.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <asm/unaligned.h>

#include "mpu6050_lab_uapi.h"

#define MPU_REG_SMPLRT_DIV	0x19
#define MPU_REG_CONFIG		0x1a
#define MPU_REG_GYRO_CONFIG	0x1b
#define MPU_REG_ACCEL_CONFIG	0x1c
#define MPU_REG_INT_PIN_CFG	0x37
#define MPU_REG_INT_ENABLE	0x38
#define MPU_REG_ACCEL_XOUT_H	0x3b
#define MPU_REG_PWR_MGMT_1	0x6b
#define MPU_REG_WHO_AM_I	0x75

#define MPU_PWR_RESET		BIT(7)
#define MPU_PWR_SLEEP		BIT(6)
#define MPU_PWR_CLKSEL_PLL_X	0x01
#define MPU_INT_DATA_RDY	BIT(0)
#define MPU_DLPF_184HZ		1	/* gyro output rate becomes 1 kHz */

#define MPU_BURST_LEN		14
#define MPU_FIFO_LEN		256	/* samples, must be a power of two */
#define MPU_RATE_MIN		4
#define MPU_RATE_MAX		1000
#define MPU_RATE_DEFAULT	100
#define MPU_WAKE_MS		50
#define MPU_AUTOSUSPEND_MS	2000
#define MPU_I2C_TRIES		3

struct mpu {
	struct device *dev;
	struct regmap *regmap;
	struct miscdevice misc;
	struct kref ref;
	int irq;
	bool use_irq;

	struct mutex lock;		/* opened, dead */
	struct mutex cfg_lock;		/* rate, hardware (re)configuration */
	struct mutex read_lock;		/* single consumer of the kfifo */
	bool opened;
	bool dead;

	wait_queue_head_t wq;
	DECLARE_KFIFO(fifo, struct mpu6050_lab_sample, MPU_FIFO_LEN);

	/* producer side, only touched from the IRQ thread / work item */
	u32 seq;
	u32 pending_drop;

	/* hard context -> thread: event timestamps (SPSC, 8 deep) */
	DECLARE_KFIFO(ts_fifo, u64, 8);
	u32 rate_hz;
	u32 period_ns;
	struct hrtimer timer;
	struct work_struct work;

	atomic_t irq_count;
	atomic_t missed;		/* events coalesced before the thread ran */
	atomic_t overflow;
	atomic_t i2c_errors;
	atomic_t i2c_retries;
};

static const struct regmap_config mpu_regmap_cfg = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x75,
	.cache_type = REGCACHE_NONE,
};

static void mpu_free(struct kref *ref)
{
	kfree(container_of(ref, struct mpu, ref));
}

/* ---- sample path -------------------------------------------------------- */

/*
 * @lost: events that fired since the last read but whose samples the chip
 * has already overwritten (the thread was late, only the newest is readable).
 */
static void mpu_capture(struct mpu *m, u64 ts, u32 lost)
{
	struct mpu6050_lab_sample s;
	u8 raw[MPU_BURST_LEN];
	int ret = 0, i;

	for (i = 0; i < MPU_I2C_TRIES; i++) {
		ret = regmap_bulk_read(m->regmap, MPU_REG_ACCEL_XOUT_H,
				       raw, sizeof(raw));
		if (!ret)
			break;
		atomic_inc(&m->i2c_retries);
		usleep_range(100, 200);
	}
	if (ret) {
		atomic_inc(&m->i2c_errors);
		m->pending_drop += lost + 1;
		return;
	}

	if (!READ_ONCE(m->opened))
		return;

	if (kfifo_is_full(&m->fifo)) {
		/* only the reader may drop old data, so drop the new one */
		m->pending_drop += lost + 1;
		atomic_inc(&m->overflow);
		return;
	}

	memset(&s, 0, sizeof(s));
	s.timestamp_ns = ts;
	s.seq = m->seq++;
	s.dropped_before = m->pending_drop + lost;
	m->pending_drop = 0;
	for (i = 0; i < 3; i++) {
		s.accel[i] = (s16)get_unaligned_be16(raw + 2 * i);
		s.gyro[i] = (s16)get_unaligned_be16(raw + 8 + 2 * i);
	}
	s.temp = (s16)get_unaligned_be16(raw + 6);

	kfifo_in(&m->fifo, &s, 1);
	wake_up_interruptible(&m->wq);
}

/* hard-IRQ / hrtimer context: only stamp the time, no bus access */
static void mpu_event(struct mpu *m)
{
	atomic_inc(&m->irq_count);
	if (!kfifo_put(&m->ts_fifo, ktime_get_ns()))
		atomic_inc(&m->missed);	/* consumer is hopelessly late */
}

/* thread context: one I2C burst per wake-up, stamped with the newest event */
static void mpu_drain(struct mpu *m)
{
	u64 ts, last = 0;
	u32 n = 0;

	while (kfifo_get(&m->ts_fifo, &ts)) {
		last = ts;
		n++;
	}
	if (!n)
		return;
	if (n > 1)
		atomic_add(n - 1, &m->missed);
	mpu_capture(m, last, n - 1);
}

static irqreturn_t mpu_irq_hard(int irq, void *p)
{
	mpu_event(p);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t mpu_irq_thread(int irq, void *p)
{
	mpu_drain(p);
	return IRQ_HANDLED;
}

static enum hrtimer_restart mpu_timer_fn(struct hrtimer *t)
{
	struct mpu *m = container_of(t, struct mpu, timer);

	mpu_event(m);
	schedule_work(&m->work);
	hrtimer_forward_now(t, ns_to_ktime(READ_ONCE(m->period_ns)));
	return HRTIMER_RESTART;
}

static void mpu_work_fn(struct work_struct *w)
{
	struct mpu *m = container_of(w, struct mpu, work);

	mpu_drain(m);
}

/* ---- hardware ------------------------------------------------------------ */

static int mpu_apply_rate(struct mpu *m)
{
	/* gyro output rate is 1 kHz with the DLPF on: rate = 1000 / (div + 1) */
	u32 div = DIV_ROUND_CLOSEST(1000, m->rate_hz) - 1;

	m->rate_hz = 1000 / (div + 1);
	WRITE_ONCE(m->period_ns, NSEC_PER_SEC / m->rate_hz);
	return regmap_write(m->regmap, MPU_REG_SMPLRT_DIV, div);
}

static int mpu_hw_start(struct mpu *m)
{
	int ret;

	ret = regmap_write(m->regmap, MPU_REG_PWR_MGMT_1, MPU_PWR_CLKSEL_PLL_X);
	if (ret)
		return ret;
	msleep(MPU_WAKE_MS);

	ret = regmap_write(m->regmap, MPU_REG_CONFIG, MPU_DLPF_184HZ);
	if (!ret)
		ret = regmap_write(m->regmap, MPU_REG_GYRO_CONFIG, 0);
	if (!ret)
		ret = regmap_write(m->regmap, MPU_REG_ACCEL_CONFIG, 0);
	if (!ret)
		ret = mpu_apply_rate(m);
	if (!ret)	/* active high, push-pull, 50 us pulse */
		ret = regmap_write(m->regmap, MPU_REG_INT_PIN_CFG, 0);
	if (ret)
		return ret;

	kfifo_reset(&m->ts_fifo);	/* producer and consumer are stopped */
	if (m->use_irq)
		return regmap_write(m->regmap, MPU_REG_INT_ENABLE,
				    MPU_INT_DATA_RDY);

	hrtimer_start(&m->timer, ns_to_ktime(m->period_ns), HRTIMER_MODE_REL);
	return 0;
}

static void mpu_hw_stop(struct mpu *m)
{
	if (m->use_irq) {
		regmap_write(m->regmap, MPU_REG_INT_ENABLE, 0);
		synchronize_irq(m->irq);
	} else {
		hrtimer_cancel(&m->timer);
		cancel_work_sync(&m->work);
	}
	regmap_write(m->regmap, MPU_REG_PWR_MGMT_1, MPU_PWR_SLEEP);
}

static int mpu_runtime_resume(struct device *dev)
{
	struct mpu *m = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&m->cfg_lock);
	ret = mpu_hw_start(m);
	mutex_unlock(&m->cfg_lock);
	if (ret)
		dev_err(dev, "start failed: %d\n", ret);
	return ret;
}

static int mpu_runtime_suspend(struct device *dev)
{
	struct mpu *m = dev_get_drvdata(dev);

	mutex_lock(&m->cfg_lock);
	mpu_hw_stop(m);
	mutex_unlock(&m->cfg_lock);
	return 0;
}

/* ---- file operations ----------------------------------------------------- */

static int mpu_open(struct inode *inode, struct file *file)
{
	struct miscdevice *md = file->private_data;
	struct mpu *m = container_of(md, struct mpu, misc);
	int ret;

	mutex_lock(&m->lock);
	if (m->dead) {
		ret = -ENODEV;
		goto out;
	}
	if (m->opened) {
		ret = -EBUSY;
		goto out;
	}
	ret = pm_runtime_resume_and_get(m->dev);
	if (ret < 0)
		goto out;

	mutex_lock(&m->read_lock);
	kfifo_reset_out(&m->fifo);	/* consumer-side reset, producer-safe */
	mutex_unlock(&m->read_lock);
	m->pending_drop = 0;
	WRITE_ONCE(m->opened, true);

	kref_get(&m->ref);
	file->private_data = m;
	ret = 0;
out:
	mutex_unlock(&m->lock);
	return ret;
}

static int mpu_release(struct inode *inode, struct file *file)
{
	struct mpu *m = file->private_data;

	mutex_lock(&m->lock);
	WRITE_ONCE(m->opened, false);
	if (!m->dead) {	/* remove() already balanced the PM count otherwise */
		pm_runtime_mark_last_busy(m->dev);
		pm_runtime_put_autosuspend(m->dev);
	}
	mutex_unlock(&m->lock);

	kref_put(&m->ref, mpu_free);
	return 0;
}

static ssize_t mpu_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	struct mpu *m = file->private_data;
	unsigned int copied;
	int ret;

	if (count < sizeof(struct mpu6050_lab_sample))
		return -EINVAL;

	for (;;) {
		if (READ_ONCE(m->dead))
			return -ENODEV;

		if (kfifo_is_empty(&m->fifo)) {
			if (file->f_flags & O_NONBLOCK)
				return -EAGAIN;
			ret = wait_event_interruptible(m->wq,
						       !kfifo_is_empty(&m->fifo) ||
						       READ_ONCE(m->dead));
			if (ret)
				return ret;
			continue;
		}

		ret = mutex_lock_interruptible(&m->read_lock);
		if (ret)
			return ret;
		ret = kfifo_to_user(&m->fifo, buf, count, &copied);
		mutex_unlock(&m->read_lock);

		if (ret)
			return ret;
		if (copied)
			return copied;
		/* another reader won the race, go around */
	}
}

static __poll_t mpu_poll(struct file *file, poll_table *wait)
{
	struct mpu *m = file->private_data;
	__poll_t mask = 0;

	poll_wait(file, &m->wq, wait);
	if (READ_ONCE(m->dead))
		return EPOLLHUP | EPOLLERR;
	if (!kfifo_is_empty(&m->fifo))
		mask |= EPOLLIN | EPOLLRDNORM;
	return mask;
}

static const struct file_operations mpu_fops = {
	.owner = THIS_MODULE,
	.open = mpu_open,
	.release = mpu_release,
	.read = mpu_read,
	.poll = mpu_poll,
	.llseek = no_llseek,
};

/* ---- sysfs --------------------------------------------------------------- */

static ssize_t sample_rate_hz_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mpu *m = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%u\n", READ_ONCE(m->rate_hz));
}

static ssize_t sample_rate_hz_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t len)
{
	struct mpu *m = dev_get_drvdata(dev);
	unsigned int rate;
	int ret;

	ret = kstrtouint(buf, 0, &rate);
	if (ret)
		return ret;
	if (rate < MPU_RATE_MIN || rate > MPU_RATE_MAX)
		return -EINVAL;

	mutex_lock(&m->cfg_lock);
	m->rate_hz = rate;
	/* if the chip is awake, program it now; otherwise resume will */
	if (pm_runtime_get_if_in_use(dev) > 0) {
		ret = mpu_apply_rate(m);
		pm_runtime_put(dev);
	} else {
		ret = 0;
		m->rate_hz = 1000 / DIV_ROUND_CLOSEST(1000, rate);
		WRITE_ONCE(m->period_ns, NSEC_PER_SEC / m->rate_hz);
	}
	mutex_unlock(&m->cfg_lock);

	return ret ? ret : len;
}
static DEVICE_ATTR_RW(sample_rate_hz);

static ssize_t stats_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct mpu *m = dev_get_drvdata(dev);

	return sysfs_emit(buf,
			  "mode %s\nevents %d\nmissed %d\nfifo_overflow %d\n"
			  "i2c_retries %d\ni2c_errors %d\n",
			  m->use_irq ? "irq" : "hrtimer",
			  atomic_read(&m->irq_count),
			  atomic_read(&m->missed),
			  atomic_read(&m->overflow),
			  atomic_read(&m->i2c_retries),
			  atomic_read(&m->i2c_errors));
}
static DEVICE_ATTR_RO(stats);

static struct attribute *mpu_attrs[] = {
	&dev_attr_sample_rate_hz.attr,
	&dev_attr_stats.attr,
	NULL,
};
ATTRIBUTE_GROUPS(mpu);

/* ---- bus glue ------------------------------------------------------------ */

/* i2c_driver.probe lost its id argument in 6.3, .remove became void in 6.1 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
static int mpu_probe(struct i2c_client *client)
#else
static int mpu_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	struct device *dev = &client->dev;
	struct mpu *m;
	unsigned int who;
	int ret;

	m = kzalloc(sizeof(*m), GFP_KERNEL);
	if (!m)
		return -ENOMEM;

	kref_init(&m->ref);
	mutex_init(&m->lock);
	mutex_init(&m->cfg_lock);
	mutex_init(&m->read_lock);
	init_waitqueue_head(&m->wq);
	INIT_KFIFO(m->fifo);
	INIT_KFIFO(m->ts_fifo);
	INIT_WORK(&m->work, mpu_work_fn);
	hrtimer_init(&m->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	m->timer.function = mpu_timer_fn;
	m->dev = dev;
	m->rate_hz = MPU_RATE_DEFAULT;
	m->period_ns = NSEC_PER_SEC / MPU_RATE_DEFAULT;
	dev_set_drvdata(dev, m);

	m->regmap = devm_regmap_init_i2c(client, &mpu_regmap_cfg);
	if (IS_ERR(m->regmap)) {
		ret = PTR_ERR(m->regmap);
		goto err_free;
	}

	ret = regmap_read(m->regmap, MPU_REG_WHO_AM_I, &who);
	if (ret) {
		dev_err(dev, "WHO_AM_I read failed: %d\n", ret);
		goto err_free;
	}
	if (who != 0x68 && who != 0x70 && who != 0x71 && who != 0x72) {
		dev_err(dev, "unexpected WHO_AM_I 0x%02x\n", who);
		ret = -ENODEV;
		goto err_free;
	}

	ret = regmap_write(m->regmap, MPU_REG_PWR_MGMT_1, MPU_PWR_RESET);
	if (ret)
		goto err_free;
	msleep(100);
	ret = regmap_write(m->regmap, MPU_REG_PWR_MGMT_1, MPU_PWR_SLEEP);
	if (ret)
		goto err_free;

	if (client->irq > 0) {
		m->irq = client->irq;
		ret = request_threaded_irq(m->irq, mpu_irq_hard, mpu_irq_thread,
					   0, "mpu6050_lab", m);
		if (ret) {
			dev_err(dev, "request_irq %d: %d\n", m->irq, ret);
			goto err_free;
		}
		m->use_irq = true;
	} else {
		dev_info(dev, "no interrupt in DT, polling with hrtimer\n");
	}

	pm_runtime_set_autosuspend_delay(dev, MPU_AUTOSUSPEND_MS);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_set_suspended(dev);	/* chip is asleep after the reset */
	pm_runtime_enable(dev);

	m->misc.minor = MISC_DYNAMIC_MINOR;
	m->misc.name = devm_kasprintf(dev, GFP_KERNEL, "mpu6050_%s",
				      dev_name(dev));
	m->misc.fops = &mpu_fops;
	m->misc.parent = dev;
	if (!m->misc.name) {
		ret = -ENOMEM;
		goto err_pm;
	}
	ret = misc_register(&m->misc);
	if (ret)
		goto err_pm;

	dev_info(dev, "WHO_AM_I 0x%02x, %s mode, /dev/%s\n", who,
		 m->use_irq ? "irq" : "hrtimer", m->misc.name);
	return 0;

err_pm:
	pm_runtime_disable(dev);
	pm_runtime_dont_use_autosuspend(dev);
	if (m->use_irq)
		free_irq(m->irq, m);
err_free:
	kfree(m);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#define MPU_REMOVE_RET	void
#define MPU_REMOVE_OK
#else
#define MPU_REMOVE_RET	int
#define MPU_REMOVE_OK	0
#endif

static MPU_REMOVE_RET mpu_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct mpu *m = dev_get_drvdata(dev);

	misc_deregister(&m->misc);	/* no new open() after this */

	mutex_lock(&m->lock);
	m->dead = true;
	if (m->opened)			/* release() will skip its put */
		pm_runtime_put_noidle(dev);
	mutex_unlock(&m->lock);
	wake_up_all(&m->wq);

	pm_runtime_disable(dev);
	pm_runtime_dont_use_autosuspend(dev);
	if (m->use_irq)
		free_irq(m->irq, m);	/* waits for a running IRQ thread */
	mutex_lock(&m->cfg_lock);
	mpu_hw_stop(m);
	mutex_unlock(&m->cfg_lock);
	pm_runtime_set_suspended(dev);

	kref_put(&m->ref, mpu_free);
	return MPU_REMOVE_OK;
}

static const struct dev_pm_ops mpu_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(mpu_runtime_suspend, mpu_runtime_resume, NULL)
};

static const struct of_device_id mpu_of_match[] = {
	{ .compatible = "hyq,mpu6050-lab" },
	{ }
};
MODULE_DEVICE_TABLE(of, mpu_of_match);

static const struct i2c_device_id mpu_id[] = {
	{ "mpu6050-lab", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu_id);

static struct i2c_driver mpu_driver = {
	.driver = {
		.name = "mpu6050_lab",
		.of_match_table = mpu_of_match,
		.pm = &mpu_pm_ops,
		.dev_groups = mpu_groups,
	},
	.probe = mpu_probe,
	.remove = mpu_remove,
	.id_table = mpu_id,
};
module_i2c_driver(mpu_driver);

MODULE_DESCRIPTION("MPU-6050 IMU I2C driver (lab)");
MODULE_AUTHOR("Huang Yuqi");
MODULE_LICENSE("GPL");
