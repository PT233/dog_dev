// SPDX-License-Identifier: GPL-2.0
/*
 * vcam - software stand-in for the robot dog's USB stereo camera
 *        (Linux 5.15 .. 6.8)
 *
 *   [vcam-sensor subdev] --media link--> [vcam capture video node]
 *     pattern generator                   videobuf2 queue + producer thread
 *     brightness / test pattern ctrls     MMAP and DMABUF export (EXPBUF)
 *
 * The real camera is a UVC stereo module that delivers one YUY2 640x480@30
 * frame with the two eyes side by side; stereo_splitter crops x in [0, 320)
 * as the left eye.  vcam offers the same capture caps and, by default, the
 * same side-by-side layout, so scripts/start_camera_stream.sh and the vision
 * front end run against it unchanged when no camera is plugged in.
 *
 * It is shaped like a sensor + bridge driver: the sensor is a v4l2_subdev
 * with pad ops and a control handler, the bridge owns the vb2 queue and hands
 * buffers back in the states the vb2 core requires.
 *
 * Markers (little endian) let user space check what it received:
 *   every line starts with {seq, ~seq}                 -> left eye / frame id
 *   in the side-by-side pattern x = width/2 carries {~seq, seq} -> right eye
 * A frame overwritten while still in use shows up as a changed marker.
 */
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <asm/unaligned.h>

#include <media/media-device.h>
#include <media/media-entity.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#define VCAM_NAME		"vcam"
#define VCAM_MAX_W		1280
#define VCAM_MIN_BUFS		2
#define VCAM_DEFAULT_FPS	30

struct vcam_size {
	u32 w, h;
};

static const struct vcam_size vcam_sizes[] = {
	{ 320, 240 }, { 640, 480 }, { 1280, 720 },
};

/* discrete frame rates; 1/fps is what S_PARM / ENUM_FRAMEINTERVALS use */
static const u32 vcam_fps[] = { 60, 30, 15, 10, 5, 1 };

static const char * const vcam_pattern_names[] = {
	"Color bars", "Horizontal gradient", "Flat gray", "Stereo side-by-side",
};

#define VCAM_PAT_SBS	3	/* left eye: colour bars, right eye: grey ramp */

/* Y, U, V for white, yellow, cyan, green, magenta, red, blue, black */
static const u8 vcam_bars[8][3] = {
	{ 235, 128, 128 }, { 210, 16, 146 }, { 170, 166, 16 }, { 145, 54, 34 },
	{ 106, 202, 222 }, { 81, 90, 240 }, { 41, 240, 110 }, { 16, 128, 128 },
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
#define vcam_set_min_bufs(q, n)	((q)->min_queued_buffers = (n))
#define vcam_sd_try_fmt(sd, st, pad)	v4l2_subdev_state_get_format(st, pad)
#else
#define vcam_set_min_bufs(q, n)	((q)->min_buffers_needed = (n))
#define vcam_sd_try_fmt(sd, st, pad)	v4l2_subdev_get_try_format(sd, st, pad)
#endif

/* ---- sensor subdev ------------------------------------------------------- */

struct vcam_sensor {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_ctrl_handler hdl;
	struct v4l2_mbus_framefmt fmt;
	struct mutex lock;		/* fmt */
	int brightness;
	int pattern;
	u8 line[VCAM_MAX_W * 2];	/* one generated line, YUYV */
};

static void vcam_default_mbus(struct v4l2_mbus_framefmt *f)
{
	memset(f, 0, sizeof(*f));
	f->width = vcam_sizes[1].w;
	f->height = vcam_sizes[1].h;
	f->code = MEDIA_BUS_FMT_YUYV8_2X8;
	f->field = V4L2_FIELD_NONE;
	f->colorspace = V4L2_COLORSPACE_SRGB;
}

static int vcam_sensor_enum_mbus_code(struct v4l2_subdev *sd,
				      struct v4l2_subdev_state *state,
				      struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index)
		return -EINVAL;
	code->code = MEDIA_BUS_FMT_YUYV8_2X8;
	return 0;
}

static int vcam_sensor_enum_frame_size(struct v4l2_subdev *sd,
				       struct v4l2_subdev_state *state,
				       struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->pad || fse->index >= ARRAY_SIZE(vcam_sizes) ||
	    fse->code != MEDIA_BUS_FMT_YUYV8_2X8)
		return -EINVAL;
	fse->min_width = vcam_sizes[fse->index].w;
	fse->max_width = fse->min_width;
	fse->min_height = vcam_sizes[fse->index].h;
	fse->max_height = fse->min_height;
	return 0;
}

static int vcam_sensor_get_fmt(struct v4l2_subdev *sd,
			       struct v4l2_subdev_state *state,
			       struct v4l2_subdev_format *fmt)
{
	struct vcam_sensor *s = container_of(sd, struct vcam_sensor, sd);

	if (fmt->pad)
		return -EINVAL;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		struct v4l2_mbus_framefmt *try = vcam_sd_try_fmt(sd, state, 0);

		if (!try->width)	/* never set: report the default */
			vcam_default_mbus(try);
		fmt->format = *try;
		return 0;
	}

	mutex_lock(&s->lock);
	fmt->format = s->fmt;
	mutex_unlock(&s->lock);
	return 0;
}

static int vcam_sensor_set_fmt(struct v4l2_subdev *sd,
			       struct v4l2_subdev_state *state,
			       struct v4l2_subdev_format *fmt)
{
	struct vcam_sensor *s = container_of(sd, struct vcam_sensor, sd);
	const struct vcam_size *sz;

	if (fmt->pad)
		return -EINVAL;

	sz = v4l2_find_nearest_size(vcam_sizes, ARRAY_SIZE(vcam_sizes), w, h,
				    fmt->format.width, fmt->format.height);
	vcam_default_mbus(&fmt->format);
	fmt->format.width = sz->w;
	fmt->format.height = sz->h;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		*vcam_sd_try_fmt(sd, state, 0) = fmt->format;
		return 0;
	}

	mutex_lock(&s->lock);
	s->fmt = fmt->format;
	mutex_unlock(&s->lock);
	return 0;
}

static const struct v4l2_subdev_pad_ops vcam_sensor_pad_ops = {
	.enum_mbus_code = vcam_sensor_enum_mbus_code,
	.enum_frame_size = vcam_sensor_enum_frame_size,
	.get_fmt = vcam_sensor_get_fmt,
	.set_fmt = vcam_sensor_set_fmt,
};

static const struct v4l2_subdev_ops vcam_sensor_ops = {
	.pad = &vcam_sensor_pad_ops,
};

static int vcam_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vcam_sensor *s =
		container_of(ctrl->handler, struct vcam_sensor, hdl);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		WRITE_ONCE(s->brightness, ctrl->val);
		return 0;
	case V4L2_CID_TEST_PATTERN:
		WRITE_ONCE(s->pattern, ctrl->val);
		return 0;
	}
	return -EINVAL;
}

static const struct v4l2_ctrl_ops vcam_ctrl_ops = {
	.s_ctrl = vcam_s_ctrl,
};

static void vcam_pixel(u8 *p, int y, int u, int v, int br)
{
	y = clamp(y + br - 128, 16, 235);
	p[0] = y;
	p[1] = u;
	p[2] = y;
	p[3] = v;
}

/* what the "sensor" puts on the wire for one frame, written into @dst */
static void vcam_sensor_gen_frame(struct vcam_sensor *s, u8 *dst, u32 w,
				  u32 h, u32 stride, u32 seq)
{
	int br = READ_ONCE(s->brightness);
	int pat = READ_ONCE(s->pattern);
	u32 half = w / 2;
	u32 x, y, shift;

	for (x = 0; x < w; x += 2) {
		u8 *p = s->line + x * 2;

		if (pat == VCAM_PAT_SBS) {
			if (x < half) {
				const u8 *c = vcam_bars[x * 8 / half];

				vcam_pixel(p, c[0], c[1], c[2], br);
			} else {
				vcam_pixel(p, 16 + (x - half) * 219 / half,
					   128, 128, br);
			}
		} else if (pat == 0) {
			const u8 *c = vcam_bars[x * 8 / w];

			vcam_pixel(p, c[0], c[1], c[2], br);
		} else if (pat == 1) {
			vcam_pixel(p, 16 + x * 219 / w, 128, 128, br);
		} else {
			vcam_pixel(p, 128, 128, 128, br);
		}
	}

	if (pat == VCAM_PAT_SBS) {
		/* each eye scrolls inside its own half, the seam never moves */
		shift = ((seq * 8) % half) & ~1U;
		for (y = 0; y < h; y++) {
			u8 *row = dst + y * stride;
			const u8 *l = s->line, *r = s->line + half * 2;

			memcpy(row, l + shift * 2, (half - shift) * 2);
			memcpy(row + (half - shift) * 2, l, shift * 2);
			memcpy(row + half * 2, r + shift * 2, (half - shift) * 2);
			memcpy(row + (w - shift) * 2, r, shift * 2);
			put_unaligned_le32(seq, row);
			put_unaligned_le32(~seq, row + 4);
			put_unaligned_le32(~seq, row + half * 2);
			put_unaligned_le32(seq, row + half * 2 + 4);
		}
		return;
	}

	/* scroll horizontally so motion is visible and frames differ */
	shift = ((seq * 8) % w) & ~1U;
	for (y = 0; y < h; y++) {
		u8 *row = dst + y * stride;

		memcpy(row, s->line + shift * 2, (w - shift) * 2);
		memcpy(row + (w - shift) * 2, s->line, shift * 2);
		put_unaligned_le32(seq, row);
		put_unaligned_le32(~seq, row + 4);
	}
}

/* ---- capture bridge ------------------------------------------------------ */

struct vcam_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

struct vcam {
	struct v4l2_device v4l2_dev;
	struct media_device mdev;
	struct video_device vdev;
	struct media_pad vpad;
	struct vcam_sensor sensor;
	struct vb2_queue q;

	struct mutex lock;		/* ioctls and the vb2 queue */
	spinlock_t qlock;		/* active list */
	struct list_head active;
	struct task_struct *thread;

	struct v4l2_pix_format fmt;
	u32 fps;
	u32 seq;
	u32 frames;
	u32 overruns;			/* ticks with no buffer queued */
};

static void vcam_fill_pix(struct v4l2_pix_format *pix,
			  const struct vcam_size *sz)
{
	pix->width = sz->w;
	pix->height = sz->h;
	pix->pixelformat = V4L2_PIX_FMT_YUYV;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = sz->w * 2;
	pix->sizeimage = pix->bytesperline * sz->h;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix->quantization = V4L2_QUANTIZATION_DEFAULT;
	pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

static void vcam_produce_one(struct vcam *v)
{
	struct vcam_buf *buf = NULL;
	unsigned long flags;
	void *vaddr;

	spin_lock_irqsave(&v->qlock, flags);
	if (!list_empty(&v->active)) {
		buf = list_first_entry(&v->active, struct vcam_buf, list);
		list_del(&buf->list);
	}
	spin_unlock_irqrestore(&v->qlock, flags);

	if (!buf) {
		v->overruns++;	/* nothing queued, the frame is lost */
		v->seq++;	/* keep the sequence gap visible to user space */
		return;
	}

	vaddr = vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
	vcam_sensor_gen_frame(&v->sensor, vaddr, v->fmt.width, v->fmt.height,
			      v->fmt.bytesperline, v->seq);
	vb2_set_plane_payload(&buf->vb.vb2_buf, 0, v->fmt.sizeimage);
	buf->vb.vb2_buf.timestamp = ktime_get_ns();
	buf->vb.sequence = v->seq++;
	buf->vb.field = V4L2_FIELD_NONE;
	v->frames++;
	vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

static int vcam_thread(void *data)
{
	struct vcam *v = data;
	ktime_t next = ktime_get();

	while (!kthread_should_stop()) {
		ktime_t period = ktime_set(0, NSEC_PER_SEC / v->fps);

		next = ktime_add(next, period);
		if (ktime_before(next, ktime_get()))	/* fell far behind */
			next = ktime_add(ktime_get(), period);

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop()) {
			__set_current_state(TASK_RUNNING);
			break;
		}
		schedule_hrtimeout(&next, HRTIMER_MODE_ABS);
		if (kthread_should_stop())
			break;
		vcam_produce_one(v);
	}
	return 0;
}

static void vcam_return_buffers(struct vcam *v, enum vb2_buffer_state state)
{
	struct vcam_buf *buf, *tmp;
	unsigned long flags;
	LIST_HEAD(drain);

	spin_lock_irqsave(&v->qlock, flags);
	list_splice_init(&v->active, &drain);
	spin_unlock_irqrestore(&v->qlock, flags);

	list_for_each_entry_safe(buf, tmp, &drain, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
}

/* ---- vb2 ops ------------------------------------------------------------- */

static int vcam_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			    unsigned int *nplanes, unsigned int sizes[],
			    struct device *alloc_devs[])
{
	struct vcam *v = vb2_get_drv_priv(vq);

	if (*nplanes)
		return sizes[0] < v->fmt.sizeimage ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = v->fmt.sizeimage;
	return 0;
}

static int vcam_buf_prepare(struct vb2_buffer *vb)
{
	struct vcam *v = vb2_get_drv_priv(vb->vb2_queue);

	if (vb2_plane_size(vb, 0) < v->fmt.sizeimage)
		return -EINVAL;
	vb2_set_plane_payload(vb, 0, v->fmt.sizeimage);
	return 0;
}

static void vcam_buf_queue(struct vb2_buffer *vb)
{
	struct vcam *v = vb2_get_drv_priv(vb->vb2_queue);
	struct vcam_buf *buf = container_of(to_vb2_v4l2_buffer(vb),
					    struct vcam_buf, vb);
	unsigned long flags;

	spin_lock_irqsave(&v->qlock, flags);
	list_add_tail(&buf->list, &v->active);
	spin_unlock_irqrestore(&v->qlock, flags);
}

static int vcam_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct vcam *v = vb2_get_drv_priv(vq);
	struct task_struct *t;

	v->seq = 0;
	v->frames = 0;
	v->overruns = 0;

	t = kthread_run(vcam_thread, v, "vcam-producer");
	if (IS_ERR(t)) {
		/* the core requires every queued buffer back as QUEUED */
		vcam_return_buffers(v, VB2_BUF_STATE_QUEUED);
		return PTR_ERR(t);
	}
	v->thread = t;
	return 0;
}

static void vcam_stop_streaming(struct vb2_queue *vq)
{
	struct vcam *v = vb2_get_drv_priv(vq);

	if (v->thread) {
		kthread_stop(v->thread);
		v->thread = NULL;
	}
	vcam_return_buffers(v, VB2_BUF_STATE_ERROR);
	dev_info(v->v4l2_dev.dev, "stream stopped: %u frames, %u overruns\n",
		 v->frames, v->overruns);
}

static const struct vb2_ops vcam_vb2_ops = {
	.queue_setup = vcam_queue_setup,
	.buf_prepare = vcam_buf_prepare,
	.buf_queue = vcam_buf_queue,
	.start_streaming = vcam_start_streaming,
	.stop_streaming = vcam_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/* ---- video ioctls -------------------------------------------------------- */

static int vcam_querycap(struct file *file, void *priv,
			 struct v4l2_capability *cap)
{
	strscpy(cap->driver, VCAM_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Stereo camera stand-in", sizeof(cap->card));
	strscpy(cap->bus_info, "platform:" VCAM_NAME, sizeof(cap->bus_info));
	return 0;
}

static int vcam_enum_fmt(struct file *file, void *priv,
			 struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;
	f->pixelformat = V4L2_PIX_FMT_YUYV;
	return 0;
}

static int vcam_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct vcam *v = video_drvdata(file);

	f->fmt.pix = v->fmt;
	return 0;
}

static int vcam_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	const struct vcam_size *sz;

	sz = v4l2_find_nearest_size(vcam_sizes, ARRAY_SIZE(vcam_sizes), w, h,
				    f->fmt.pix.width, f->fmt.pix.height);
	vcam_fill_pix(&f->fmt.pix, sz);
	return 0;
}

static int vcam_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct vcam *v = video_drvdata(file);
	struct v4l2_subdev_format sfmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	int ret;

	if (vb2_is_busy(&v->q))
		return -EBUSY;

	vcam_try_fmt(file, priv, f);

	/* negotiate with the sensor subdev exactly like a real bridge does */
	sfmt.format.width = f->fmt.pix.width;
	sfmt.format.height = f->fmt.pix.height;
	sfmt.format.code = MEDIA_BUS_FMT_YUYV8_2X8;
	ret = v4l2_subdev_call(&v->sensor.sd, pad, set_fmt, NULL, &sfmt);
	if (ret)
		return ret;
	if (sfmt.format.width != f->fmt.pix.width ||
	    sfmt.format.height != f->fmt.pix.height)
		return -EINVAL;

	v->fmt = f->fmt.pix;
	return 0;
}

static int vcam_enum_framesizes(struct file *file, void *priv,
				struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index >= ARRAY_SIZE(vcam_sizes) ||
	    fsize->pixel_format != V4L2_PIX_FMT_YUYV)
		return -EINVAL;
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = vcam_sizes[fsize->index].w;
	fsize->discrete.height = vcam_sizes[fsize->index].h;
	return 0;
}

static int vcam_enum_frameintervals(struct file *file, void *priv,
				    struct v4l2_frmivalenum *fival)
{
	unsigned int i;
	bool known = false;

	if (fival->index >= ARRAY_SIZE(vcam_fps) ||
	    fival->pixel_format != V4L2_PIX_FMT_YUYV)
		return -EINVAL;
	for (i = 0; i < ARRAY_SIZE(vcam_sizes); i++)
		known |= vcam_sizes[i].w == fival->width &&
			 vcam_sizes[i].h == fival->height;
	if (!known)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = vcam_fps[fival->index];
	return 0;
}

static int vcam_g_parm(struct file *file, void *priv,
		       struct v4l2_streamparm *p)
{
	struct vcam *v = video_drvdata(file);

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	memset(&p->parm.capture, 0, sizeof(p->parm.capture));
	p->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	p->parm.capture.timeperframe.numerator = 1;
	p->parm.capture.timeperframe.denominator = v->fps;
	return 0;
}

static int vcam_s_parm(struct file *file, void *priv,
		       struct v4l2_streamparm *p)
{
	struct vcam *v = video_drvdata(file);
	u32 want = VCAM_DEFAULT_FPS, best = vcam_fps[0], diff = U32_MAX, d;
	unsigned int i;

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (vb2_is_busy(&v->q) && vb2_is_streaming(&v->q))
		return -EBUSY;

	if (p->parm.capture.timeperframe.numerator &&
	    p->parm.capture.timeperframe.denominator)
		want = DIV_ROUND_CLOSEST(p->parm.capture.timeperframe.denominator,
					 p->parm.capture.timeperframe.numerator);

	for (i = 0; i < ARRAY_SIZE(vcam_fps); i++) {
		d = abs((int)vcam_fps[i] - (int)want);
		if (d < diff) {
			diff = d;
			best = vcam_fps[i];
		}
	}
	v->fps = best;
	return vcam_g_parm(file, priv, p);
}

static const struct v4l2_ioctl_ops vcam_ioctl_ops = {
	.vidioc_querycap = vcam_querycap,
	.vidioc_enum_fmt_vid_cap = vcam_enum_fmt,
	.vidioc_g_fmt_vid_cap = vcam_g_fmt,
	.vidioc_s_fmt_vid_cap = vcam_s_fmt,
	.vidioc_try_fmt_vid_cap = vcam_try_fmt,
	.vidioc_enum_framesizes = vcam_enum_framesizes,
	.vidioc_enum_frameintervals = vcam_enum_frameintervals,
	.vidioc_g_parm = vcam_g_parm,
	.vidioc_s_parm = vcam_s_parm,

	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,

	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct v4l2_file_operations vcam_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

/* ---- device setup / teardown -------------------------------------------- */

/* last reference to the v4l2_device dropped: no fd, no node, safe to free */
static void vcam_v4l2_release(struct v4l2_device *v4l2_dev)
{
	struct vcam *v = container_of(v4l2_dev, struct vcam, v4l2_dev);

	v4l2_ctrl_handler_free(&v->sensor.hdl);
	v4l2_device_unregister(&v->v4l2_dev);
	media_device_cleanup(&v->mdev);
	kfree(v);
}

static int vcam_sensor_init(struct vcam *v)
{
	struct vcam_sensor *s = &v->sensor;
	struct v4l2_ctrl *c;
	int ret;

	mutex_init(&s->lock);
	vcam_default_mbus(&s->fmt);
	s->brightness = 128;
	s->pattern = VCAM_PAT_SBS;	/* look like the stereo camera by default */

	v4l2_ctrl_handler_init(&s->hdl, 2);
	v4l2_ctrl_new_std(&s->hdl, &vcam_ctrl_ops, V4L2_CID_BRIGHTNESS,
			  0, 255, 1, 128);
	c = v4l2_ctrl_new_std_menu_items(&s->hdl, &vcam_ctrl_ops,
					 V4L2_CID_TEST_PATTERN,
					 ARRAY_SIZE(vcam_pattern_names) - 1,
					 0, VCAM_PAT_SBS, vcam_pattern_names);
	if (s->hdl.error || !c) {
		ret = s->hdl.error ? s->hdl.error : -ENOMEM;
		v4l2_ctrl_handler_free(&s->hdl);
		return ret;
	}

	v4l2_subdev_init(&s->sd, &vcam_sensor_ops);
	strscpy(s->sd.name, "vcam-sensor", sizeof(s->sd.name));
	s->sd.owner = THIS_MODULE;
	s->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	s->sd.ctrl_handler = &s->hdl;
	s->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	s->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&s->sd.entity, 1, &s->pad);
	if (ret)
		v4l2_ctrl_handler_free(&s->hdl);
	return ret;
}

static int vcam_probe(struct platform_device *pdev)
{
	struct vcam *v;
	struct vb2_queue *q;
	int ret;

	v = kzalloc(sizeof(*v), GFP_KERNEL);
	if (!v)
		return -ENOMEM;

	mutex_init(&v->lock);
	spin_lock_init(&v->qlock);
	INIT_LIST_HEAD(&v->active);
	v->fps = VCAM_DEFAULT_FPS;
	vcam_fill_pix(&v->fmt, &vcam_sizes[1]);
	platform_set_drvdata(pdev, v);

	v->mdev.dev = &pdev->dev;
	strscpy(v->mdev.model, VCAM_NAME, sizeof(v->mdev.model));
	media_device_init(&v->mdev);
	v->v4l2_dev.mdev = &v->mdev;
	v->v4l2_dev.release = vcam_v4l2_release;
	strscpy(v->v4l2_dev.name, VCAM_NAME, sizeof(v->v4l2_dev.name));

	ret = v4l2_device_register(&pdev->dev, &v->v4l2_dev);
	if (ret)
		goto err_mdev;

	ret = vcam_sensor_init(v);
	if (ret)
		goto err_v4l2;
	ret = v4l2_device_register_subdev(&v->v4l2_dev, &v->sensor.sd);
	if (ret)
		goto err_ctrl;

	q = &v->q;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = v;
	q->buf_struct_size = sizeof(struct vcam_buf);
	q->ops = &vcam_vb2_ops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &v->lock;
	q->dev = &pdev->dev;
	vcam_set_min_bufs(q, VCAM_MIN_BUFS);
	ret = vb2_queue_init(q);
	if (ret)
		goto err_subdev;

	strscpy(v->vdev.name, VCAM_NAME, sizeof(v->vdev.name));
	v->vdev.v4l2_dev = &v->v4l2_dev;
	v->vdev.fops = &vcam_fops;
	v->vdev.ioctl_ops = &vcam_ioctl_ops;
	v->vdev.release = video_device_release_empty;
	v->vdev.vfl_dir = VFL_DIR_RX;
	v->vdev.queue = q;
	v->vdev.lock = &v->lock;
	v->vdev.ctrl_handler = &v->sensor.hdl;
	v->vdev.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	video_set_drvdata(&v->vdev, v);

	v->vpad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&v->vdev.entity, 1, &v->vpad);
	if (ret)
		goto err_subdev;

	ret = video_register_device(&v->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto err_subdev;

	ret = media_create_pad_link(&v->sensor.sd.entity, 0, &v->vdev.entity, 0,
				    MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	if (ret)
		goto err_video;
	ret = v4l2_device_register_subdev_nodes(&v->v4l2_dev);
	if (ret)
		goto err_video;
	ret = media_device_register(&v->mdev);
	if (ret)
		goto err_video;

	dev_info(&pdev->dev, "registered %s, sensor %s\n",
		 video_device_node_name(&v->vdev), v->sensor.sd.name);
	return 0;

err_video:
	vb2_video_unregister_device(&v->vdev);
err_subdev:
	v4l2_device_unregister_subdev(&v->sensor.sd);
err_ctrl:
	v4l2_ctrl_handler_free(&v->sensor.hdl);
err_v4l2:
	v4l2_device_unregister(&v->v4l2_dev);
err_mdev:
	media_device_cleanup(&v->mdev);
	kfree(v);
	return ret;
}

static int vcam_remove(struct platform_device *pdev)
{
	struct vcam *v = platform_get_drvdata(pdev);

	media_device_unregister(&v->mdev);
	vb2_video_unregister_device(&v->vdev);	/* stops streaming, wakes pollers */
	v4l2_device_disconnect(&v->v4l2_dev);
	v4l2_device_put(&v->v4l2_dev);		/* frees v once the last fd closes */
	return 0;
}

static struct platform_driver vcam_driver = {
	.probe = vcam_probe,
	.remove = vcam_remove,
	.driver = { .name = VCAM_NAME },
};

static struct platform_device *vcam_pdev;

static int __init vcam_init(void)
{
	int ret;

	ret = platform_driver_register(&vcam_driver);
	if (ret)
		return ret;

	vcam_pdev = platform_device_register_simple(VCAM_NAME, -1, NULL, 0);
	if (IS_ERR(vcam_pdev)) {
		platform_driver_unregister(&vcam_driver);
		return PTR_ERR(vcam_pdev);
	}
	if (!platform_get_drvdata(vcam_pdev)) {	/* probe failed */
		platform_device_unregister(vcam_pdev);
		platform_driver_unregister(&vcam_driver);
		return -ENODEV;
	}
	return 0;
}

static void __exit vcam_exit(void)
{
	platform_device_unregister(vcam_pdev);
	platform_driver_unregister(&vcam_driver);
}

module_init(vcam_init);
module_exit(vcam_exit);

MODULE_DESCRIPTION("V4L2 stand-in for the robot dog's USB stereo camera");
MODULE_AUTHOR("Huang Yuqi");
MODULE_LICENSE("GPL");
