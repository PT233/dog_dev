// SPDX-License-Identifier: GPL-2.0
/*
 * dogcam - minimal USB Video Class capture driver for the robot dog's stereo
 *          camera, used instead of uvcvideo (Linux 5.15 .. 6.8)
 *
 * Scope: one thing done end to end, uncompressed YUY2 over isochronous IN.
 *
 *   probe       parse the class-specific VideoControl / VideoStreaming
 *               descriptors, collect the YUY2 frame sizes and intervals
 *   STREAMON    VS_PROBE / VS_COMMIT negotiation, pick the smallest alternate
 *               setting whose endpoint carries dwMaxPayloadTransferSize,
 *               submit isochronous URBs
 *   URB done    walk the packets, parse each payload header (FID / EOF / ERR),
 *               copy the payload into the current vb2 buffer, hand a frame to
 *               user space only when it is complete
 *   STREAMOFF   kill the URBs, go back to alternate setting 0
 *
 * It binds to the VideoControl interface and claims the streaming interface,
 * exactly like uvcvideo, so only one of the two can own a camera.  There is
 * no static ID table: bind a camera explicitly with
 *
 *   echo <vid> <pid> > /sys/bus/usb/drivers/dogcam/new_id
 *
 * after unbinding (or blacklisting) uvcvideo.  UVCIOC_CTRL_QUERY is passed
 * through to the VideoControl interface so vendor tools that poke extension
 * units (e.g. to switch a stereo module into side-by-side mode) keep working.
 *
 * Not supported: MJPEG and other compressed formats, bulk endpoints, V4L2
 * controls, still capture.
 */
#include <linux/list.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/usb/video.h>
#include <linux/uvcvideo.h>
#include <linux/version.h>
#include <asm/unaligned.h>

#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#define DOGCAM_NAME		"dogcam"
#define DOGCAM_MAX_FRAMES	16
#define DOGCAM_MAX_INTERVALS	8
#define DOGCAM_URBS		5
#define DOGCAM_MAX_PACKETS	32
#define DOGCAM_MIN_BUFS		2
#define DOGCAM_CTRL_TIMEOUT	5000
#define DOGCAM_XU_MAX		1024
#define DOGCAM_INTERVAL_30FPS	333333	/* 100 ns units */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
#define dogcam_set_min_bufs(q, n)	((q)->min_queued_buffers = (n))
#else
#define dogcam_set_min_bufs(q, n)	((q)->min_buffers_needed = (n))
#endif

static const u8 dogcam_guid_yuy2[16] = {
	'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00,
	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
};

struct dogcam_frame {
	u32 w, h;
	u8 index;			/* bFrameIndex */
	u32 def_interval;
	u8 nintervals;
	u32 intervals[DOGCAM_MAX_INTERVALS];
};

struct dogcam_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

struct dogcam_stats {
	u32 frames;			/* delivered */
	u32 incomplete;			/* short or flagged ERR, dropped */
	u32 no_buffer;			/* nothing queued when a frame arrived */
	u32 pkt_errors;			/* iso packets with a bad status */
	u32 bad_header;
};

struct dogcam {
	struct usb_device *udev;
	struct usb_interface *vc_intf;
	struct usb_interface *vs_intf;
	u8 vc_ifnum;
	u8 vs_ifnum;
	u16 bcd_uvc;
	u8 ctrl_len;			/* probe/commit block: 26, 34 or 48 */
	u8 format_index;		/* bFormatIndex of the YUY2 format */
	struct dogcam_frame frames[DOGCAM_MAX_FRAMES];
	unsigned int nframes;

	struct v4l2_device v4l2_dev;
	struct video_device vdev;
	struct vb2_queue q;
	struct mutex lock;		/* ioctls, vb2 queue, USB control I/O */
	bool disconnected;

	/* current selection */
	const struct dogcam_frame *frame;
	u32 interval;
	struct v4l2_pix_format fmt;

	/* negotiated per STREAMON */
	u32 max_payload;
	u8 alt;
	u8 ep_addr;
	u8 ep_interval;
	u32 psize;
	struct urb *urbs[DOGCAM_URBS];
	u32 urb_size;
	bool streaming;

	/* frame assembly, all under qlock (URB completion context) */
	spinlock_t qlock;
	struct list_head active;
	struct dogcam_buf *cur;
	u8 *cur_vaddr;
	u32 filled;
	int last_fid;
	bool skip;			/* frame ended, wait for FID to toggle */
	bool frame_err;
	u64 frame_ts;
	u32 sequence;
	struct dogcam_stats stats;
};

/* ---- descriptor parsing -------------------------------------------------- */

static int dogcam_parse_vc(struct dogcam *d)
{
	const u8 *buf = d->vc_intf->altsetting[0].extra;
	int len = d->vc_intf->altsetting[0].extralen;

	while (len >= 3 && buf[0] >= 3 && buf[0] <= len) {
		if (buf[1] == USB_DT_CS_INTERFACE && buf[2] == UVC_VC_HEADER) {
			if (buf[0] < 13 || buf[11] < 1)
				return -EINVAL;
			d->bcd_uvc = get_unaligned_le16(buf + 3);
			d->vs_ifnum = buf[12];	/* baInterfaceNr[0] */
			if (d->bcd_uvc >= 0x0150)
				d->ctrl_len = 48;
			else if (d->bcd_uvc >= 0x0110)
				d->ctrl_len = 34;
			else
				d->ctrl_len = 26;
			return 0;
		}
		len -= buf[0];
		buf += buf[0];
	}
	return -ENODEV;
}

static void dogcam_parse_frame(struct dogcam *d, const u8 *buf)
{
	struct dogcam_frame *f;
	unsigned int i, n;

	if (buf[0] < 26 || d->nframes >= DOGCAM_MAX_FRAMES)
		return;

	f = &d->frames[d->nframes];
	f->index = buf[3];
	f->w = get_unaligned_le16(buf + 5);
	f->h = get_unaligned_le16(buf + 7);
	f->def_interval = get_unaligned_le32(buf + 21);
	if (!f->w || !f->h || !f->def_interval)
		return;

	n = buf[25];			/* 0 means a continuous range */
	if (buf[0] < 26 + 4 * n)
		return;
	n = min_t(unsigned int, n, DOGCAM_MAX_INTERVALS);
	for (i = 0; i < n; i++)
		f->intervals[i] = get_unaligned_le32(buf + 26 + 4 * i);
	f->nintervals = n;
	d->nframes++;
}

static int dogcam_parse_vs(struct dogcam *d)
{
	const u8 *buf = d->vs_intf->altsetting[0].extra;
	int len = d->vs_intf->altsetting[0].extralen;
	bool in_yuy2 = false;

	while (len >= 3 && buf[0] >= 3 && buf[0] <= len) {
		if (buf[1] == USB_DT_CS_INTERFACE) {
			switch (buf[2]) {
			case UVC_VS_FORMAT_UNCOMPRESSED:
				in_yuy2 = buf[0] >= 27 && !d->format_index &&
					  !memcmp(buf + 5, dogcam_guid_yuy2, 16);
				if (in_yuy2)
					d->format_index = buf[3];
				break;
			case UVC_VS_FRAME_UNCOMPRESSED:
				if (in_yuy2)
					dogcam_parse_frame(d, buf);
				break;
			case UVC_VS_FORMAT_MJPEG:
			case UVC_VS_FORMAT_FRAME_BASED:
				in_yuy2 = false;
				break;
			}
		}
		len -= buf[0];
		buf += buf[0];
	}
	return d->nframes ? 0 : -ENODEV;
}

/* ---- format helpers ------------------------------------------------------ */

static void dogcam_fill_pix(struct v4l2_pix_format *pix,
			    const struct dogcam_frame *f)
{
	pix->width = f->w;
	pix->height = f->h;
	pix->pixelformat = V4L2_PIX_FMT_YUYV;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = f->w * 2;
	pix->sizeimage = pix->bytesperline * f->h;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix->quantization = V4L2_QUANTIZATION_DEFAULT;
	pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

static u32 dogcam_pick_interval(const struct dogcam_frame *f, u32 want)
{
	u32 best = f->def_interval, diff = U32_MAX, dlt;
	unsigned int i;

	for (i = 0; i < f->nintervals; i++) {
		dlt = abs((int)f->intervals[i] - (int)want);
		if (dlt < diff) {
			diff = dlt;
			best = f->intervals[i];
		}
	}
	return best;
}

static void dogcam_select(struct dogcam *d, const struct dogcam_frame *f)
{
	d->frame = f;
	d->interval = dogcam_pick_interval(f, DOGCAM_INTERVAL_30FPS);
	dogcam_fill_pix(&d->fmt, f);
}

/* ---- USB control --------------------------------------------------------- */

static int dogcam_ctrl(struct dogcam *d, u8 query, u8 unit, u8 ifnum, u8 cs,
		       void *data, u16 len)
{
	bool in = query & 0x80;
	u8 type = USB_TYPE_CLASS | USB_RECIP_INTERFACE |
		  (in ? USB_DIR_IN : USB_DIR_OUT);
	unsigned int pipe = in ? usb_rcvctrlpipe(d->udev, 0)
			       : usb_sndctrlpipe(d->udev, 0);
	int ret;

	ret = usb_control_msg(d->udev, pipe, query, type, cs << 8,
			      unit << 8 | ifnum, data, len,
			      DOGCAM_CTRL_TIMEOUT);
	if (ret < 0)
		return ret;
	return ret == len ? 0 : -EIO;
}

/*
 * VS_PROBE_CONTROL / VS_COMMIT_CONTROL.  Offsets in the control block:
 * bmHint 0, bFormatIndex 2, bFrameIndex 3, dwFrameInterval 4,
 * dwMaxVideoFrameSize 18, dwMaxPayloadTransferSize 22.
 */
static int dogcam_negotiate(struct dogcam *d)
{
	u8 *c;
	int ret;

	c = kzalloc(48, GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	put_unaligned_le16(1, c);		/* keep dwFrameInterval fixed */
	c[2] = d->format_index;
	c[3] = d->frame->index;
	put_unaligned_le32(d->interval, c + 4);

	ret = dogcam_ctrl(d, UVC_SET_CUR, 0, d->vs_ifnum, UVC_VS_PROBE_CONTROL,
			  c, d->ctrl_len);
	if (!ret)
		ret = dogcam_ctrl(d, UVC_GET_CUR, 0, d->vs_ifnum,
				  UVC_VS_PROBE_CONTROL, c, d->ctrl_len);
	if (ret)
		goto out;

	if (c[2] != d->format_index || c[3] != d->frame->index) {
		dev_err(&d->udev->dev, "camera refused format %u frame %u\n",
			d->format_index, d->frame->index);
		ret = -EINVAL;
		goto out;
	}
	d->max_payload = get_unaligned_le32(c + 22);
	if (!d->max_payload) {
		ret = -EINVAL;
		goto out;
	}

	ret = dogcam_ctrl(d, UVC_SET_CUR, 0, d->vs_ifnum,
			  UVC_VS_COMMIT_CONTROL, c, d->ctrl_len);
out:
	kfree(c);
	return ret;
}

/* bytes one isochronous endpoint can move per service interval */
static u32 dogcam_ep_bpi(struct usb_device *udev,
			 struct usb_host_endpoint *ep)
{
	if (udev->speed >= USB_SPEED_SUPER)
		return le16_to_cpu(ep->ss_ep_comp.wBytesPerInterval);
	return usb_endpoint_maxp(&ep->desc) * usb_endpoint_maxp_mult(&ep->desc);
}

/* smallest alternate setting that still carries max_payload */
static int dogcam_select_alt(struct dogcam *d)
{
	struct usb_interface *intf = d->vs_intf;
	u32 best = U32_MAX;
	unsigned int i, j;

	for (i = 0; i < intf->num_altsetting; i++) {
		struct usb_host_interface *alt = &intf->altsetting[i];

		for (j = 0; j < alt->desc.bNumEndpoints; j++) {
			struct usb_host_endpoint *ep = &alt->endpoint[j];
			u32 psize;

			if (!usb_endpoint_is_isoc_in(&ep->desc))
				continue;
			psize = dogcam_ep_bpi(d->udev, ep);
			if (psize >= d->max_payload && psize < best) {
				best = psize;
				d->alt = alt->desc.bAlternateSetting;
				d->ep_addr = ep->desc.bEndpointAddress;
				d->ep_interval = ep->desc.bInterval;
			}
		}
	}
	if (best == U32_MAX)
		return -ENOSPC;
	d->psize = best;
	return 0;
}

/* ---- frame assembly (URB completion context, qlock held) ---------------- */

static void dogcam_frame_end(struct dogcam *d)
{
	struct dogcam_buf *buf = d->cur;
	u32 seq = d->sequence++;	/* dropped frames leave a gap */

	if (!buf) {
		d->stats.no_buffer++;
	} else if (d->filled != d->fmt.sizeimage || d->frame_err) {
		d->stats.incomplete++;	/* keep the buffer for the next frame */
	} else {
		d->cur = NULL;
		vb2_set_plane_payload(&buf->vb.vb2_buf, 0, d->filled);
		buf->vb.vb2_buf.timestamp = d->frame_ts;
		buf->vb.sequence = seq;
		buf->vb.field = V4L2_FIELD_NONE;
		d->stats.frames++;
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
	}
	d->filled = 0;
	d->frame_err = false;
}

static void dogcam_decode(struct dogcam *d, const u8 *data, unsigned int len)
{
	unsigned int hlen, plen, room, n;
	int fid;

	if (len < 2)
		return;			/* empty microframe */
	hlen = data[0];
	if (hlen < 2 || hlen > len) {
		d->stats.bad_header++;
		return;
	}
	fid = data[1] & UVC_STREAM_FID;

	if (fid != d->last_fid) {
		/* FID toggled: this payload opens a new frame */
		if (d->filled && !d->skip)
			dogcam_frame_end(d);	/* previous one had no EOF */
		d->last_fid = fid;
		d->skip = false;
		d->filled = 0;
		d->frame_err = false;
		d->frame_ts = ktime_get_ns();
	}
	if (d->skip)
		return;
	if (data[1] & UVC_STREAM_ERR)
		d->frame_err = true;

	plen = len - hlen;
	if (plen) {
		if (!d->cur && !list_empty(&d->active)) {
			d->cur = list_first_entry(&d->active, struct dogcam_buf,
						  list);
			list_del(&d->cur->list);
			d->cur_vaddr = vb2_plane_vaddr(&d->cur->vb.vb2_buf, 0);
		}
		room = d->fmt.sizeimage - d->filled;
		n = min(plen, room);
		if (plen > room)
			d->frame_err = true;	/* more data than one frame */
		if (d->cur)
			memcpy(d->cur_vaddr + d->filled, data + hlen, n);
		d->filled += n;
	}

	/* EOF closes the frame; cameras that never set it are closed on size */
	if ((data[1] & UVC_STREAM_EOF) || d->filled == d->fmt.sizeimage) {
		dogcam_frame_end(d);
		d->skip = true;
	}
}

static void dogcam_urb_complete(struct urb *urb)
{
	struct dogcam *d = urb->context;
	unsigned long flags;
	int i;

	if (urb->status)
		return;			/* killed, unplugged or fatal: stop here */

	spin_lock_irqsave(&d->qlock, flags);
	for (i = 0; i < urb->number_of_packets; i++) {
		struct usb_iso_packet_descriptor *p = &urb->iso_frame_desc[i];

		if (p->status < 0) {
			d->stats.pkt_errors++;
			d->frame_err = true;	/* a hole in this frame */
			continue;
		}
		dogcam_decode(d, urb->transfer_buffer + p->offset,
			      p->actual_length);
	}
	spin_unlock_irqrestore(&d->qlock, flags);

	if (READ_ONCE(d->streaming))
		usb_submit_urb(urb, GFP_ATOMIC);	/* fails once killed */
}

/* ---- URB management ------------------------------------------------------ */

static void dogcam_free_urbs(struct dogcam *d)
{
	unsigned int i;

	for (i = 0; i < DOGCAM_URBS; i++) {
		struct urb *urb = d->urbs[i];

		if (!urb)
			continue;
		usb_kill_urb(urb);
		usb_free_coherent(d->udev, d->urb_size, urb->transfer_buffer,
				  urb->transfer_dma);
		usb_free_urb(urb);
		d->urbs[i] = NULL;
	}
}

static int dogcam_alloc_urbs(struct dogcam *d)
{
	u32 npackets = min_t(u32, DOGCAM_MAX_PACKETS,
			     DIV_ROUND_UP(d->fmt.sizeimage, d->psize));
	unsigned int i, j;

	d->urb_size = npackets * d->psize;
	for (i = 0; i < DOGCAM_URBS; i++) {
		struct urb *urb = usb_alloc_urb(npackets, GFP_KERNEL);

		if (!urb)
			goto err;
		urb->transfer_buffer = usb_alloc_coherent(d->udev, d->urb_size,
							  GFP_KERNEL,
							  &urb->transfer_dma);
		if (!urb->transfer_buffer) {
			usb_free_urb(urb);
			goto err;
		}
		urb->dev = d->udev;
		urb->context = d;
		urb->pipe = usb_rcvisocpipe(d->udev, d->ep_addr);
		urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
		urb->interval = d->ep_interval;
		urb->complete = dogcam_urb_complete;
		urb->number_of_packets = npackets;
		urb->transfer_buffer_length = d->urb_size;
		for (j = 0; j < npackets; j++) {
			urb->iso_frame_desc[j].offset = j * d->psize;
			urb->iso_frame_desc[j].length = d->psize;
		}
		d->urbs[i] = urb;
	}
	return 0;
err:
	dogcam_free_urbs(d);
	return -ENOMEM;
}

/* ---- vb2 ops ------------------------------------------------------------- */

static void dogcam_return_buffers(struct dogcam *d, enum vb2_buffer_state state)
{
	struct dogcam_buf *buf, *tmp;
	unsigned long flags;
	LIST_HEAD(drain);

	spin_lock_irqsave(&d->qlock, flags);
	if (d->cur) {
		list_add(&d->cur->list, &drain);
		d->cur = NULL;
	}
	list_splice_init(&d->active, &drain);
	spin_unlock_irqrestore(&d->qlock, flags);

	list_for_each_entry_safe(buf, tmp, &drain, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
}

static int dogcam_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			      unsigned int *nplanes, unsigned int sizes[],
			      struct device *alloc_devs[])
{
	struct dogcam *d = vb2_get_drv_priv(vq);

	if (*nplanes)
		return sizes[0] < d->fmt.sizeimage ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = d->fmt.sizeimage;
	return 0;
}

static int dogcam_buf_prepare(struct vb2_buffer *vb)
{
	struct dogcam *d = vb2_get_drv_priv(vb->vb2_queue);

	if (vb2_plane_size(vb, 0) < d->fmt.sizeimage)
		return -EINVAL;
	vb2_set_plane_payload(vb, 0, d->fmt.sizeimage);
	return 0;
}

static void dogcam_buf_queue(struct vb2_buffer *vb)
{
	struct dogcam *d = vb2_get_drv_priv(vb->vb2_queue);
	struct dogcam_buf *buf = container_of(to_vb2_v4l2_buffer(vb),
					      struct dogcam_buf, vb);
	unsigned long flags;

	spin_lock_irqsave(&d->qlock, flags);
	list_add_tail(&buf->list, &d->active);
	spin_unlock_irqrestore(&d->qlock, flags);
}

static int dogcam_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct dogcam *d = vb2_get_drv_priv(vq);
	unsigned int i;
	int ret;

	if (d->disconnected) {
		ret = -ENODEV;
		goto err_buffers;
	}

	ret = dogcam_negotiate(d);
	if (!ret)
		ret = dogcam_select_alt(d);
	if (ret) {
		dev_err(&d->udev->dev, "stream setup failed: %d\n", ret);
		goto err_buffers;
	}
	ret = usb_set_interface(d->udev, d->vs_ifnum, d->alt);
	if (ret)
		goto err_buffers;
	ret = dogcam_alloc_urbs(d);
	if (ret)
		goto err_alt;

	d->last_fid = -1;
	d->skip = false;
	d->filled = 0;
	d->frame_err = false;
	d->sequence = 0;
	memset(&d->stats, 0, sizeof(d->stats));
	WRITE_ONCE(d->streaming, true);

	for (i = 0; i < DOGCAM_URBS; i++) {
		ret = usb_submit_urb(d->urbs[i], GFP_KERNEL);
		if (ret) {
			dev_err(&d->udev->dev, "submit urb %u: %d\n", i, ret);
			goto err_urbs;
		}
	}
	dev_dbg(&d->udev->dev, "streaming: alt %u, %u bytes/interval\n",
		d->alt, d->psize);
	return 0;

err_urbs:
	WRITE_ONCE(d->streaming, false);
	dogcam_free_urbs(d);
err_alt:
	usb_set_interface(d->udev, d->vs_ifnum, 0);
err_buffers:
	/* the core requires every queued buffer back as QUEUED */
	dogcam_return_buffers(d, VB2_BUF_STATE_QUEUED);
	return ret;
}

static void dogcam_stop_streaming(struct vb2_queue *vq)
{
	struct dogcam *d = vb2_get_drv_priv(vq);

	WRITE_ONCE(d->streaming, false);
	dogcam_free_urbs(d);		/* usb_kill_urb waits for completions */
	if (!d->disconnected)
		usb_set_interface(d->udev, d->vs_ifnum, 0);
	dogcam_return_buffers(d, VB2_BUF_STATE_ERROR);
	dev_info(&d->udev->dev,
		 "stream stopped: %u frames, %u incomplete, %u without buffer, %u packet errors\n",
		 d->stats.frames, d->stats.incomplete, d->stats.no_buffer,
		 d->stats.pkt_errors);
}

static const struct vb2_ops dogcam_vb2_ops = {
	.queue_setup = dogcam_queue_setup,
	.buf_prepare = dogcam_buf_prepare,
	.buf_queue = dogcam_buf_queue,
	.start_streaming = dogcam_start_streaming,
	.stop_streaming = dogcam_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/* ---- video ioctls -------------------------------------------------------- */

static int dogcam_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	struct dogcam *d = video_drvdata(file);

	strscpy(cap->driver, DOGCAM_NAME, sizeof(cap->driver));
	strscpy(cap->card, d->vdev.name, sizeof(cap->card));
	usb_make_path(d->udev, cap->bus_info, sizeof(cap->bus_info));
	return 0;
}

static int dogcam_enum_fmt(struct file *file, void *priv,
			   struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;
	f->pixelformat = V4L2_PIX_FMT_YUYV;
	return 0;
}

static int dogcam_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct dogcam *d = video_drvdata(file);

	f->fmt.pix = d->fmt;
	return 0;
}

static const struct dogcam_frame *dogcam_nearest(struct dogcam *d, u32 w, u32 h)
{
	return v4l2_find_nearest_size(d->frames, d->nframes, w, h, w, h);
}

static int dogcam_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct dogcam *d = video_drvdata(file);

	dogcam_fill_pix(&f->fmt.pix, dogcam_nearest(d, f->fmt.pix.width,
						    f->fmt.pix.height));
	return 0;
}

static int dogcam_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct dogcam *d = video_drvdata(file);

	if (vb2_is_busy(&d->q))
		return -EBUSY;
	dogcam_select(d, dogcam_nearest(d, f->fmt.pix.width,
					f->fmt.pix.height));
	f->fmt.pix = d->fmt;
	return 0;
}

static const struct dogcam_frame *dogcam_find(struct dogcam *d, u32 w, u32 h)
{
	unsigned int i;

	for (i = 0; i < d->nframes; i++)
		if (d->frames[i].w == w && d->frames[i].h == h)
			return &d->frames[i];
	return NULL;
}

static int dogcam_enum_framesizes(struct file *file, void *priv,
				  struct v4l2_frmsizeenum *fsize)
{
	struct dogcam *d = video_drvdata(file);

	if (fsize->index >= d->nframes ||
	    fsize->pixel_format != V4L2_PIX_FMT_YUYV)
		return -EINVAL;
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = d->frames[fsize->index].w;
	fsize->discrete.height = d->frames[fsize->index].h;
	return 0;
}

/*
 * Frame interval (100 ns units) -> the small fraction applications expect.
 * 333333 is "30 fps" but 333333/10000000 does not reduce, and user space asks
 * for exactly 1/30.  Expand to a continued fraction and cut it at the first
 * large term: 333333/10^7 = [0; 30, 33333] -> [0; 30] = 1/30.
 */
static void dogcam_fract(struct v4l2_fract *fr, u32 interval)
{
	u32 term[8], x = interval, y = 10000000U, r;
	unsigned int n, i;

	for (n = 0; n < ARRAY_SIZE(term) && y; n++) {
		term[n] = x / y;
		if (term[n] >= 333) {
			if (n < 2)
				n++;
			break;
		}
		r = x - term[n] * y;
		x = y;
		y = r;
	}

	x = 0;
	y = 1;
	for (i = n; i > 0; i--) {
		r = y;
		y = term[i - 1] * y + x;
		x = r;
	}
	fr->numerator = y;
	fr->denominator = x;
}

static int dogcam_enum_input(struct file *file, void *priv,
			     struct v4l2_input *inp)
{
	if (inp->index)
		return -EINVAL;
	inp->type = V4L2_INPUT_TYPE_CAMERA;
	strscpy(inp->name, "Camera", sizeof(inp->name));
	return 0;
}

static int dogcam_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int dogcam_s_input(struct file *file, void *priv, unsigned int i)
{
	return i ? -EINVAL : 0;
}

static int dogcam_enum_frameintervals(struct file *file, void *priv,
				      struct v4l2_frmivalenum *fival)
{
	struct dogcam *d = video_drvdata(file);
	const struct dogcam_frame *f;
	u32 interval;

	if (fival->pixel_format != V4L2_PIX_FMT_YUYV)
		return -EINVAL;
	f = dogcam_find(d, fival->width, fival->height);
	if (!f)
		return -EINVAL;

	if (f->nintervals) {
		if (fival->index >= f->nintervals)
			return -EINVAL;
		interval = f->intervals[fival->index];
	} else {
		if (fival->index)
			return -EINVAL;
		interval = f->def_interval;
	}
	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	dogcam_fract(&fival->discrete, interval);
	return 0;
}

static int dogcam_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *p)
{
	struct dogcam *d = video_drvdata(file);

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	memset(&p->parm.capture, 0, sizeof(p->parm.capture));
	p->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	dogcam_fract(&p->parm.capture.timeperframe, d->interval);
	return 0;
}

static int dogcam_s_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *p)
{
	struct dogcam *d = video_drvdata(file);
	struct v4l2_fract *tpf = &p->parm.capture.timeperframe;
	u32 want = DOGCAM_INTERVAL_30FPS;

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (vb2_is_streaming(&d->q))
		return -EBUSY;

	if (tpf->numerator && tpf->denominator)
		want = div_u64(10000000ULL * tpf->numerator, tpf->denominator);
	d->interval = dogcam_pick_interval(d->frame, want);
	return dogcam_g_parm(file, priv, p);
}

/*
 * UVCIOC_CTRL_QUERY: raw class request to a unit of the VideoControl
 * interface.  Same ABI as uvcvideo, so vendor scripts keep working.
 */
static long dogcam_default(struct file *file, void *fh, bool valid_prio,
			   unsigned int cmd, void *arg)
{
	struct dogcam *d = video_drvdata(file);
	struct uvc_xu_control_query *q = arg;
	bool in;
	u8 *buf;
	long ret;

	if (cmd != UVCIOC_CTRL_QUERY)
		return -ENOTTY;
	if (d->disconnected)
		return -ENODEV;

	switch (q->query) {
	case UVC_SET_CUR:
	case UVC_GET_CUR:
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_RES:
	case UVC_GET_LEN:
	case UVC_GET_INFO:
	case UVC_GET_DEF:
		break;
	default:
		return -EINVAL;
	}
	if (!q->size || q->size > DOGCAM_XU_MAX)
		return -EINVAL;
	in = q->query & 0x80;

	if (in) {
		buf = kzalloc(q->size, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;
	} else {
		buf = memdup_user(q->data, q->size);
		if (IS_ERR(buf))
			return PTR_ERR(buf);
	}

	ret = dogcam_ctrl(d, q->query, q->unit, d->vc_ifnum, q->selector,
			  buf, q->size);
	if (!ret && in && copy_to_user(q->data, buf, q->size))
		ret = -EFAULT;
	kfree(buf);
	return ret;
}

static const struct v4l2_ioctl_ops dogcam_ioctl_ops = {
	.vidioc_querycap = dogcam_querycap,
	.vidioc_enum_fmt_vid_cap = dogcam_enum_fmt,
	.vidioc_g_fmt_vid_cap = dogcam_g_fmt,
	.vidioc_s_fmt_vid_cap = dogcam_s_fmt,
	.vidioc_try_fmt_vid_cap = dogcam_try_fmt,
	.vidioc_enum_framesizes = dogcam_enum_framesizes,
	.vidioc_enum_frameintervals = dogcam_enum_frameintervals,
	.vidioc_g_parm = dogcam_g_parm,
	.vidioc_s_parm = dogcam_s_parm,
	.vidioc_enum_input = dogcam_enum_input,
	.vidioc_g_input = dogcam_g_input,
	.vidioc_s_input = dogcam_s_input,

	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,

	.vidioc_default = dogcam_default,
};

static const struct v4l2_file_operations dogcam_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

/* ---- bus glue ------------------------------------------------------------ */

static struct usb_driver dogcam_driver;

/* last reference to the v4l2_device dropped: no fd, no node, safe to free */
static void dogcam_v4l2_release(struct v4l2_device *v4l2_dev)
{
	struct dogcam *d = container_of(v4l2_dev, struct dogcam, v4l2_dev);

	v4l2_device_unregister(&d->v4l2_dev);
	usb_put_dev(d->udev);
	kfree(d);
}

static int dogcam_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	struct usb_host_interface *alt = intf->cur_altsetting;
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_interface *vs_alt;
	struct vb2_queue *q;
	struct dogcam *d;
	unsigned int i;
	int ret;

	/* like uvcvideo: bind to VideoControl, claim VideoStreaming from here */
	if (alt->desc.bInterfaceClass != USB_CLASS_VIDEO ||
	    alt->desc.bInterfaceSubClass != UVC_SC_VIDEOCONTROL)
		return -ENODEV;

	d = kzalloc(sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;
	mutex_init(&d->lock);
	spin_lock_init(&d->qlock);
	INIT_LIST_HEAD(&d->active);
	d->udev = usb_get_dev(udev);
	d->vc_intf = intf;
	d->vc_ifnum = alt->desc.bInterfaceNumber;

	ret = dogcam_parse_vc(d);
	if (ret)
		goto err_free;
	d->vs_intf = usb_ifnum_to_if(udev, d->vs_ifnum);
	if (!d->vs_intf) {
		ret = -ENODEV;
		goto err_free;
	}
	vs_alt = &d->vs_intf->altsetting[0];
	if (vs_alt->desc.bInterfaceClass != USB_CLASS_VIDEO ||
	    vs_alt->desc.bInterfaceSubClass != UVC_SC_VIDEOSTREAMING) {
		ret = -ENODEV;
		goto err_free;
	}
	ret = dogcam_parse_vs(d);
	if (ret) {
		dev_err(&intf->dev, "no uncompressed YUY2 format\n");
		goto err_free;
	}

	/* default to 640x480, the mode the vision pipeline asks for */
	dogcam_select(d, dogcam_nearest(d, 640, 480));

	ret = usb_driver_claim_interface(&dogcam_driver, d->vs_intf, d);
	if (ret)
		goto err_free;

	d->v4l2_dev.release = dogcam_v4l2_release;
	ret = v4l2_device_register(&intf->dev, &d->v4l2_dev);
	if (ret)
		goto err_claim;

	q = &d->q;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = d;
	q->buf_struct_size = sizeof(struct dogcam_buf);
	q->ops = &dogcam_vb2_ops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &d->lock;
	q->dev = &intf->dev;
	dogcam_set_min_bufs(q, DOGCAM_MIN_BUFS);
	ret = vb2_queue_init(q);
	if (ret)
		goto err_v4l2;

	if (udev->product)
		strscpy(d->vdev.name, udev->product, sizeof(d->vdev.name));
	else
		strscpy(d->vdev.name, DOGCAM_NAME, sizeof(d->vdev.name));
	d->vdev.v4l2_dev = &d->v4l2_dev;
	d->vdev.fops = &dogcam_fops;
	d->vdev.ioctl_ops = &dogcam_ioctl_ops;
	d->vdev.release = video_device_release_empty;
	d->vdev.vfl_dir = VFL_DIR_RX;
	d->vdev.queue = q;
	d->vdev.lock = &d->lock;
	d->vdev.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	video_set_drvdata(&d->vdev, d);

	usb_set_intfdata(intf, d);
	ret = video_register_device(&d->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto err_intfdata;

	dev_info(&intf->dev, "%s: UVC %x.%02x, YUY2 format %u, %u frame sizes\n",
		 video_device_node_name(&d->vdev), d->bcd_uvc >> 8,
		 d->bcd_uvc & 0xff, d->format_index, d->nframes);
	for (i = 0; i < d->nframes; i++)
		dev_info(&intf->dev, "  %ux%u, %u intervals\n", d->frames[i].w,
			 d->frames[i].h, d->frames[i].nintervals);
	return 0;

err_intfdata:
	usb_set_intfdata(intf, NULL);
err_v4l2:
	v4l2_device_unregister(&d->v4l2_dev);
err_claim:
	usb_driver_release_interface(&dogcam_driver, d->vs_intf);
err_free:
	usb_put_dev(d->udev);
	kfree(d);
	return ret;
}

static void dogcam_disconnect(struct usb_interface *intf)
{
	struct dogcam *d = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	/* the streaming interface goes away with the control interface */
	if (!d || intf->cur_altsetting->desc.bInterfaceSubClass !=
		  UVC_SC_VIDEOCONTROL)
		return;

	vb2_video_unregister_device(&d->vdev);	/* stops streaming, wakes pollers */
	mutex_lock(&d->lock);
	d->disconnected = true;
	mutex_unlock(&d->lock);
	usb_driver_release_interface(&dogcam_driver, d->vs_intf);
	v4l2_device_disconnect(&d->v4l2_dev);
	v4l2_device_put(&d->v4l2_dev);		/* frees d once the last fd closes */
}

/* IDs are added at run time through sysfs new_id, see the header comment */
static const struct usb_device_id dogcam_ids[] = {
	{ }
};

static struct usb_driver dogcam_driver = {
	.name = DOGCAM_NAME,
	.probe = dogcam_probe,
	.disconnect = dogcam_disconnect,
	.id_table = dogcam_ids,
};
module_usb_driver(dogcam_driver);

MODULE_DESCRIPTION("Minimal UVC YUY2 capture driver for the robot dog camera");
MODULE_AUTHOR("Huang Yuqi");
MODULE_LICENSE("GPL");
