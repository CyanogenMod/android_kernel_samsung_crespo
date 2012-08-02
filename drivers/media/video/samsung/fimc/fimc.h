/* linux/drivers/media/video/samsung/fimc/fimc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Header file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef __FIMC_H
#define __FIMC_H __FILE__

#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>
#include <plat/media.h>
#include <plat/fimc.h>
#endif

#define FIMC_NAME		"s3c-fimc"

#define FIMC_DEVICES		3
#define FIMC_SUBDEVS		3
#define FIMC_MAXCAMS		5 /* added 1 because of WriteBack */
#define FIMC_PHYBUFS		4
#define FIMC_OUTBUFS		3
#define FIMC_INQUEUES		10
#define FIMC_MAX_CTXS		1
#define FIMC_TPID		3
#define FIMC_CAPBUFS		16
#define FIMC_ONESHOT_TIMEOUT	200
#define FIMC_DQUEUE_TIMEOUT	200
#define FIMC_FIFOOFF_CNT	1000000 /* Sufficiently big value for stop */

#define FORMAT_FLAGS_PACKED	0x1
#define FORMAT_FLAGS_PLANAR	0x2
#define FORMAT_FLAGS_ENCODED	0x3

#define FIMC_ADDR_Y		0
#define FIMC_ADDR_CB		1
#define FIMC_ADDR_CR		2

#define FIMC_HD_WIDTH		1280
#define FIMC_HD_HEIGHT		720

#define FIMC_FHD_WIDTH		1920
#define FIMC_FHD_HEIGHT		1080

#define FIMC_MMAP_IDX		-1
#define FIMC_USERPTR_IDX	-2

#define FIMC_HCLK		0
#define FIMC_SCLK		1
#define FIMC_OVLY_MODE FIMC_OVLY_DMA_AUTO

#define FIMC_PINGPONG 2

/*
 * ENUMERATIONS
*/
enum fimc_status {
	FIMC_READY_OFF		= 0x00,
	FIMC_STREAMOFF		= 0x01,
	FIMC_READY_ON		= 0x02,
	FIMC_STREAMON		= 0x03,
	FIMC_STREAMON_IDLE	= 0x04, /* oneshot mode */
	FIMC_OFF_SLEEP		= 0x05,
	FIMC_ON_SLEEP		= 0x06,
	FIMC_ON_IDLE_SLEEP	= 0x07, /* oneshot mode */
	FIMC_READY_RESUME	= 0x08,
};

enum fimc_fifo_state {
	FIFO_CLOSE,
	FIFO_SLEEP,
};

enum fimc_fimd_state {
	FIMD_OFF,
	FIMD_ON,
};

enum fimc_rot_flip {
	FIMC_XFLIP	= 0x01,
	FIMC_YFLIP	= 0x02,
	FIMC_ROT	= 0x10,
};

enum fimc_input {
	FIMC_SRC_CAM,
	FIMC_SRC_MSDMA,
};

enum fimc_overlay_mode {
	/* Overlay mode isn't fixed. */
	FIMC_OVLY_NOT_FIXED		= 0x0,
	/* Non-destructive Overlay with DMA */
	FIMC_OVLY_DMA_AUTO		= 0x1,
	/* Non-destructive Overlay with DMA */
	FIMC_OVLY_DMA_MANUAL		= 0x2,
	/* Destructive Overlay with DMA single destination buffer */
	FIMC_OVLY_NONE_SINGLE_BUF	= 0x3,
	/* Destructive Overlay with DMA multiple dstination buffer */
	FIMC_OVLY_NONE_MULTI_BUF	= 0x4,
};

enum fimc_autoload {
	FIMC_AUTO_LOAD,
	FIMC_ONE_SHOT,
};

enum fimc_log {
	FIMC_LOG_DEBUG		= 0x1000,
	FIMC_LOG_INFO_L2	= 0x0200,
	FIMC_LOG_INFO_L1	= 0x0100,
	FIMC_LOG_WARN		= 0x0010,
	FIMC_LOG_ERR		= 0x0001,
};

enum fimc_pixel_format_type{
	FIMC_RGB,
	FIMC_YUV420,
	FIMC_YUV422,
	FIMC_YUV444,
};

/*
 * STRUCTURES
*/

/* for reserved memory */
struct fimc_meminfo {
	dma_addr_t	base;		/* buffer base */
	size_t		size;		/* total length */
	dma_addr_t	curr;		/* current addr */
};

struct fimc_buf {
	dma_addr_t	base[3];
	size_t		length[3];
};

struct fimc_overlay_buf {
	u32 vir_addr[3];
	size_t size[3];
	u32 phy_addr[3];
};

struct fimc_overlay {
	enum fimc_overlay_mode mode;
	struct fimc_overlay_buf buf;
	s32 req_idx;
	int	fb_id;
};

/* general buffer */
struct fimc_buf_set {
	int			id;
	/* Plane 0/1/2 for raw data, Plane 3 for padding buffer (if required) */
	dma_addr_t		base[4];
	size_t			length[4];
	size_t			garbage[4];
	enum videobuf_state	state;
	u32			flags;
	atomic_t		mapped_cnt;
	struct list_head	list;
};

/* for capture device */
struct fimc_capinfo {
	struct v4l2_cropcap	cropcap;
	struct v4l2_rect	crop;
	struct v4l2_pix_format	fmt;
	struct fimc_buf_set	bufs[FIMC_CAPBUFS];
	struct list_head	inq;
	int			outq[FIMC_PHYBUFS];
	int			nr_bufs;
	int			irq;
	int			lastirq;

	/* flip: V4L2_CID_xFLIP, rotate: 90, 180, 270 */
	u32			flip;
	u32			rotate;
};

/* for output overlay device */
struct fimc_idx {
	int ctx;
	int idx;
};

struct fimc_ctx_idx {
	struct fimc_idx prev;
	struct fimc_idx active;
	struct fimc_idx next;
};

/* scaler abstraction: local use recommended */
struct fimc_scaler {
	u32 bypass;
	u32 hfactor;
	u32 vfactor;
	u32 pre_hratio;
	u32 pre_vratio;
	u32 pre_dst_width;
	u32 pre_dst_height;
	u32 scaleup_h;
	u32 scaleup_v;
	u32 main_hratio;
	u32 main_vratio;
	u32 real_width;
	u32 real_height;
	u32 shfactor;
	u32 skipline;
};

struct fimc_ctx {
	u32			ctx_num;
	struct v4l2_cropcap	cropcap;
	struct v4l2_rect	crop;
	struct v4l2_pix_format	pix;
	struct v4l2_window	win;
	struct v4l2_framebuffer	fbuf;
	struct fimc_scaler	sc;
	struct fimc_overlay	overlay;

	u32			buf_num;
	u32			is_requested;
	struct fimc_buf_set	src[FIMC_OUTBUFS];
	struct fimc_buf_set	dst[FIMC_OUTBUFS];
	s32			inq[FIMC_OUTBUFS];
	s32			outq[FIMC_OUTBUFS];

	u32			flip;
	u32			rotate;
	enum fimc_status	status;
};

struct fimc_outinfo {
	int			last_ctx;
	spinlock_t		lock_in;
	spinlock_t		lock_out;
	struct fimc_idx		inq[FIMC_INQUEUES];
	struct fimc_ctx		ctx[FIMC_MAX_CTXS];
	struct fimc_ctx_idx	idxs;
};

struct s3cfb_user_window {
	int x;
	int y;
};

enum s3cfb_data_path_t {
	DATA_PATH_FIFO	= 0,
	DATA_PATH_DMA	= 1,
	DATA_PATH_IPC	= 2,
};

enum s3cfb_mem_owner_t {
	DMA_MEM_NONE	= 0,
	DMA_MEM_FIMD	= 1,
	DMA_MEM_OTHER	= 2,
};

enum s3cfb_alpha_t {
	PLANE_BLENDING,
	PIXEL_BLENDING,
};

enum s3cfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

struct s3cfb_alpha {
	enum		s3cfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};


struct s3cfb_chroma {
	int		enabled;
	int		blended;
	unsigned int	key;
	unsigned int	comp_key;
	unsigned int	alpha;
	enum		s3cfb_chroma_dir_t dir;
};

struct s3cfb_window {
	int			id;
	int			enabled;
	atomic_t		in_use;
	int			x;
	int			y;
	enum			s3cfb_data_path_t path;
	enum			s3cfb_mem_owner_t owner;
	unsigned int	other_mem_addr;
	unsigned int	other_mem_size;
	int			local_channel;
	int			dma_burst;
	unsigned int		pseudo_pal[16];
	struct			s3cfb_alpha alpha;
	struct			s3cfb_chroma chroma;
};

#define S3CFB_WIN_OFF_ALL	_IO('F', 202)
#define S3CFB_WIN_POSITION	_IOW('F', 203, struct s3cfb_user_window)
#define S3CFB_GET_LCD_WIDTH	_IOR('F', 302, int)
#define S3CFB_GET_LCD_HEIGHT	_IOR('F', 303, int)
#define S3CFB_SET_WRITEBACK	_IOW('F', 304, u32)
#define S3CFB_SET_WIN_ON	_IOW('F', 306, u32)
#define S3CFB_SET_WIN_OFF	_IOW('F', 307, u32)
#define S3CFB_SET_WIN_PATH	_IOW('F', 308, enum s3cfb_data_path_t)
#define S3CFB_SET_WIN_ADDR	_IOW('F', 309, unsigned long)
#define S3CFB_SET_WIN_MEM	_IOW('F', 310, enum s3cfb_mem_owner_t)
/* ------------------------------------------------------------------------ */

struct fimc_fbinfo {
	struct fb_fix_screeninfo	*fix;
	struct fb_var_screeninfo	*var;
	int				lcd_hres;
	int				lcd_vres;
	u32				is_enable;
};

struct fimc_limit {
	u32 pre_dst_w;
	u32 bypass_w;
	u32 trg_h_no_rot;
	u32 trg_h_rot;
	u32 real_w_no_rot;
	u32 real_h_rot;
};

enum FIMC_EFFECT_FIN {
	FIMC_EFFECT_FIN_BYPASS = 0,
	FIMC_EFFECT_FIN_ARBITRARY_CBCR,
	FIMC_EFFECT_FIN_NEGATIVE,
	FIMC_EFFECT_FIN_ART_FREEZE,
	FIMC_EFFECT_FIN_EMBOSSING,
	FIMC_EFFECT_FIN_SILHOUETTE,
};


struct fimc_effect {
	int ie_on;
	int ie_after_sc;
	enum FIMC_EFFECT_FIN fin;
	int pat_cb;
	int pat_cr;
};

/* fimc controller abstration */
struct fimc_control {
	int				id;		/* controller id */
	char				name[16];
	atomic_t			in_use;
	void __iomem			*regs;		/* register i/o */
	struct clk			*clk;		/* interface clock */
	struct regulator	*regulator;		/* pd regulator */
	struct fimc_meminfo		mem;		/* for reserved mem */

	/* kernel helpers */
	struct mutex			lock;		/* controller lock */
	struct mutex			alloc_lock;
	struct mutex			v4l2_lock;
	wait_queue_head_t		wq;
	struct device			*dev;
	int				irq;

	/* v4l2 related */
	struct video_device		*vd;
	struct v4l2_device		v4l2_dev;

	/* fimc specific */
	struct fimc_limit		*limit;		/* H/W limitation */
	struct s3c_platform_camera	*cam;		/* activated camera */
	struct fimc_capinfo		*cap;		/* capture dev info */
	struct fimc_outinfo		*out;		/* output dev info */
	struct fimc_fbinfo		fb;		/* fimd info */
	struct fimc_scaler		sc;		/* scaler info */
	struct fimc_effect		fe;		/* fimc effect info */

	enum fimc_status		status;
	enum fimc_log			log;

	u32				ctx_busy[FIMC_MAX_CTXS];
};

/* global */
struct fimc_global {
	struct fimc_control		ctrl[FIMC_DEVICES];
	struct s3c_platform_camera	camera[FIMC_MAXCAMS];
	int				camera_isvalid[FIMC_MAXCAMS];
	int				active_camera;
	int				initialized;
};

struct fimc_prv_data {
	struct fimc_control *ctrl;
	int ctx_id;
};

/* debug macro */
#define FIMC_LOG_DEFAULT	(FIMC_LOG_WARN | FIMC_LOG_ERR)

#define FIMC_DEBUG(fmt, ...)						\
	do {								\
		if (ctrl->log & FIMC_LOG_DEBUG)				\
			printk(KERN_DEBUG FIMC_NAME "-%d : "		\
				fmt, ctrl->id, ##__VA_ARGS__);		\
	} while (0)

#define FIMC_INFO_L2(fmt, ...)						\
	do {								\
		if (ctrl->log & FIMC_LOG_INFO_L2)			\
			printk(KERN_INFO FIMC_NAME "-%d : "		\
				fmt, ctrl->id, ##__VA_ARGS__);		\
	} while (0)

#define FIMC_INFO_L1(fmt, ...)						\
	do {								\
		if (ctrl->log & FIMC_LOG_INFO_L1)			\
			printk(KERN_INFO FIMC_NAME "-%d : "		\
				fmt, ctrl->id, ##__VA_ARGS__);		\
	} while (0)

#define FIMC_WARN(fmt, ...)						\
	do {								\
		if (ctrl->log & FIMC_LOG_WARN)				\
			printk(KERN_WARNING FIMC_NAME "-%d : "		\
				fmt, ctrl->id, ##__VA_ARGS__);		\
	} while (0)


#define FIMC_ERROR(fmt, ...)						\
	do {								\
		if (ctrl->log & FIMC_LOG_ERR)				\
			printk(KERN_ERR FIMC_NAME "-%d : "		\
				fmt, ctrl->id, ##__VA_ARGS__);		\
	} while (0)


#define fimc_dbg(fmt, ...)		FIMC_DEBUG(fmt, ##__VA_ARGS__)
#define fimc_info2(fmt, ...)		FIMC_INFO_L2(fmt, ##__VA_ARGS__)
#define fimc_info1(fmt, ...)		FIMC_INFO_L1(fmt, ##__VA_ARGS__)
#define fimc_warn(fmt, ...)		FIMC_WARN(fmt, ##__VA_ARGS__)
#define fimc_err(fmt, ...)		FIMC_ERROR(fmt, ##__VA_ARGS__)

/*
 * EXTERNS
*/
extern struct fimc_global *fimc_dev;
extern struct video_device fimc_video_device[FIMC_DEVICES];
extern const struct v4l2_ioctl_ops fimc_v4l2_ops;
extern struct fimc_limit fimc40_limits[FIMC_DEVICES];
extern struct fimc_limit fimc43_limits[FIMC_DEVICES];
extern struct fimc_limit fimc50_limits[FIMC_DEVICES];

/* general */
extern void s3c_csis_start(int lanes, int settle, int align,
					int width, int height,
					int pixel_format);
extern int fimc_dma_alloc(struct fimc_control *ctrl,
					struct fimc_buf_set *bs,
					int i, int align);
extern void fimc_dma_free(struct fimc_control *ctrl,
					struct fimc_buf_set *bs, int i);
extern u32 fimc_mapping_rot_flip(u32 rot, u32 flip);
extern int fimc_get_scaler_factor(u32 src, u32 tar, u32 *ratio, u32 *shift);
extern void fimc_get_nv12t_size(int img_hres, int img_vres,
					int *y_size, int *cb_size);
extern void fimc_clk_en(struct fimc_control *ctrl, bool on);

/* camera */
extern int fimc_select_camera(struct fimc_control *ctrl);

/* capture device */
extern int fimc_enum_input(struct file *file, void *fh,
					struct v4l2_input *inp);
extern int fimc_g_input(struct file *file, void *fh, unsigned int *i);
extern int fimc_s_input(struct file *file, void *fh, unsigned int i);
extern int fimc_enum_fmt_vid_capture(struct file *file, void *fh,
					struct v4l2_fmtdesc *f);
extern int fimc_g_fmt_vid_capture(struct file *file, void *fh,
					struct v4l2_format *f);
extern int fimc_s_fmt_vid_capture(struct file *file, void *fh,
					struct v4l2_format *f);
extern int fimc_try_fmt_vid_capture(struct file *file, void *fh,
					struct v4l2_format *f);
extern int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b);
extern int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b);
extern int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c);
extern int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c);
extern int fimc_s_ext_ctrls_capture(void *fh, struct v4l2_ext_controls *c);
extern int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a);
extern int fimc_g_crop_capture(void *fh, struct v4l2_crop *a);
extern int fimc_s_crop_capture(void *fh, struct v4l2_crop *a);
extern int fimc_streamon_capture(void *fh);
extern int fimc_streamoff_capture(void *fh);
extern int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b);
extern int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b);
extern int fimc_g_parm(struct file *file, void *fh,
					struct v4l2_streamparm *a);
extern int fimc_s_parm(struct file *file, void *fh,
					struct v4l2_streamparm *a);
extern int fimc_queryctrl(struct file *file, void *fh,
					struct v4l2_queryctrl *qc);
extern int fimc_querymenu(struct file *file, void *fh,
					struct v4l2_querymenu *qm);

#if defined(CONFIG_CPU_S5PV210)
extern int fimc_change_clksrc(struct fimc_control *ctrl, int fimc_clk);
#endif
extern int fimc_release_subdev(struct fimc_control *ctrl);

/* output device */
extern void fimc_outdev_set_src_addr(struct fimc_control *ctrl,
					dma_addr_t *base);
extern int fimc_outdev_set_ctx_param(struct fimc_control *ctrl,
					struct fimc_ctx *ctx);
extern int fimc_start_fifo(struct fimc_control *ctrl,
					struct fimc_ctx *ctx);
extern int fimc_fimd_rect(const struct fimc_control *ctrl,
					const struct fimc_ctx *ctx,
					struct v4l2_rect *fimd_rect);
extern int fimc_outdev_stop_streaming(struct fimc_control *ctrl,
					struct fimc_ctx *ctx);
extern int fimc_outdev_resume_dma(struct fimc_control *ctrl,
					struct fimc_ctx *ctx);
extern int fimc_outdev_start_camif(void *param);
extern int fimc_reqbufs_output(void *fh, struct v4l2_requestbuffers *b);
extern int fimc_querybuf_output(void *fh, struct v4l2_buffer *b);
extern int fimc_g_ctrl_output(void *fh, struct v4l2_control *c);
extern int fimc_s_ctrl_output(struct file *filp, void *fh,
					struct v4l2_control *c);
extern int fimc_cropcap_output(void *fh, struct v4l2_cropcap *a);
extern int fimc_g_crop_output(void *fh, struct v4l2_crop *a);
extern int fimc_s_crop_output(void *fh, struct v4l2_crop *a);
extern int fimc_streamon_output(void *fh);
extern int fimc_streamoff_output(void *fh);
extern int fimc_qbuf_output(void *fh, struct v4l2_buffer *b);
extern int fimc_dqbuf_output(void *fh, struct v4l2_buffer *b);
extern int fimc_g_fmt_vid_out(struct file *filp, void *fh,
					struct v4l2_format *f);
extern int fimc_s_fmt_vid_out(struct file *filp, void *fh,
					struct v4l2_format *f);
extern int fimc_try_fmt_vid_out(struct file *filp, void *fh,
					struct v4l2_format *f);
extern int fimc_output_set_dst_addr(struct fimc_control *ctrl,
					struct fimc_ctx *ctx, int idx);
extern int fimc_init_in_queue(struct fimc_control *ctrl, struct fimc_ctx *ctx);
extern int fimc_push_inq(struct fimc_control *ctrl,
					struct fimc_ctx *ctx, int idx);
extern int fimc_pop_inq(struct fimc_control *ctrl, int *ctx_num, int *idx);
extern int fimc_push_outq(struct fimc_control *ctrl,
					struct fimc_ctx *ctx, int idx);
extern int fimc_pop_outq(struct fimc_control *ctrl,
					struct fimc_ctx *ctx, int *idx);
extern int fimc_init_out_queue(struct fimc_control *ctrl, struct fimc_ctx *ctx);
extern void fimc_outdev_init_idxs(struct fimc_control *ctrl);

extern void fimc_dump_context(struct fimc_control *ctrl, struct fimc_ctx *ctx);
extern void fimc_print_signal(struct fimc_control *ctrl);

/* overlay device */
extern int fimc_try_fmt_overlay(struct file *filp, void *fh,
					struct v4l2_format *f);
extern int fimc_g_fmt_vid_overlay(struct file *filp, void *fh,
					struct v4l2_format *f);
extern int fimc_s_fmt_vid_overlay(struct file *filp, void *fh,
					struct v4l2_format *f);
extern int fimc_g_fbuf(struct file *filp, void *fh,
					struct v4l2_framebuffer *fb);
extern int fimc_s_fbuf(struct file *filp, void *fh,
					struct v4l2_framebuffer *fb);

/* Register access file */
extern void fimc_reset(struct fimc_control *ctrl);
extern int fimc_hwset_camera_source(struct fimc_control *ctrl);
extern int fimc_hwset_enable_irq(struct fimc_control *ctrl,
						int overflow, int level);
extern int fimc_hwset_disable_irq(struct fimc_control *ctrl);
extern int fimc_hwset_clear_irq(struct fimc_control *ctrl);
extern int fimc_hwset_reset(struct fimc_control *ctrl);
extern int fimc_hwset_sw_reset(struct fimc_control *ctrl);
extern int fimc_hwset_clksrc(struct fimc_control *ctrl, int src_clk);
extern int fimc_hwget_overflow_state(struct fimc_control *ctrl);
extern int fimc_hwset_camera_offset(struct fimc_control *ctrl);
extern int fimc_hwset_camera_polarity(struct fimc_control *ctrl);
extern int fimc_hwset_camera_type(struct fimc_control *ctrl);
extern int fimc_hwset_output_size(struct fimc_control *ctrl,
						int width, int height);
extern int fimc_hwset_output_colorspace(struct fimc_control *ctrl,
						u32 pixelformat);
extern int fimc_hwset_output_rot_flip(struct fimc_control *ctrl,
						u32 rot, u32 flip);
extern int fimc_hwset_output_area(struct fimc_control *ctrl,
						u32 width, u32 height);
extern int fimc_hwset_output_area_size(struct fimc_control *ctrl, u32 size);
extern int fimc_hwset_output_scan(struct fimc_control *ctrl,
						struct v4l2_pix_format *fmt);
extern int fimc_hwset_enable_lastirq(struct fimc_control *ctrl);
extern int fimc_hwset_disable_lastirq(struct fimc_control *ctrl);
extern int fimc_hwset_prescaler(struct fimc_control *ctrl,
						struct fimc_scaler *sc);
extern int fimc_hwset_output_yuv(struct fimc_control *ctrl, u32 pixelformat);
extern int fimc_hwset_output_address(struct fimc_control *ctrl,
						struct fimc_buf_set *bs,
						int id);
extern int fimc_hwset_input_rot(struct fimc_control *ctrl, u32 rot, u32 flip);
extern int fimc_hwset_scaler(struct fimc_control *ctrl, struct fimc_scaler *sc);
extern int fimc_hwset_scaler_bypass(struct fimc_control *ctrl);
extern int fimc_hwset_enable_lcdfifo(struct fimc_control *ctrl);
extern int fimc_hwset_disable_lcdfifo(struct fimc_control *ctrl);
extern int fimc_hwset_start_scaler(struct fimc_control *ctrl);
extern int fimc_hwset_stop_scaler(struct fimc_control *ctrl);
extern int fimc_hwset_input_rgb(struct fimc_control *ctrl, u32 pixelformat);
extern int fimc_hwset_intput_field(struct fimc_control *ctrl,
						enum v4l2_field field);
extern int fimc_hwset_output_rgb(struct fimc_control *ctrl, u32 pixelformat);
extern int fimc_hwset_ext_rgb(struct fimc_control *ctrl, int enable);
extern int fimc_hwset_enable_capture(struct fimc_control *ctrl,
						u32 bypass);
extern int fimc_hwset_disable_capture(struct fimc_control *ctrl);
extern void fimc_wait_disable_capture(struct fimc_control *ctrl);
extern int fimc_hwset_input_address(struct fimc_control *ctrl,
						dma_addr_t *base);
extern int fimc_hwset_enable_autoload(struct fimc_control *ctrl);
extern int fimc_hwset_disable_autoload(struct fimc_control *ctrl);
extern int fimc_hwset_real_input_size(struct fimc_control *ctrl,
						u32 width, u32 height);
extern int fimc_hwset_addr_change_enable(struct fimc_control *ctrl);
extern int fimc_hwset_addr_change_disable(struct fimc_control *ctrl);
extern int fimc_hwset_input_burst_cnt(struct fimc_control *ctrl, u32 cnt);
extern int fimc_hwset_input_colorspace(struct fimc_control *ctrl,
						u32 pixelformat);
extern int fimc_hwset_input_yuv(struct fimc_control *ctrl, u32 pixelformat);
extern int fimc_hwset_input_flip(struct fimc_control *ctrl, u32 rot, u32 flip);
extern int fimc_hwset_input_source(struct fimc_control *ctrl,
						enum fimc_input path);
extern int fimc_hwset_start_input_dma(struct fimc_control *ctrl);
extern int fimc_hwset_stop_input_dma(struct fimc_control *ctrl);
extern int fimc_hwset_output_offset(struct fimc_control *ctrl,
						u32 pixelformat,
						struct v4l2_rect *bound,
						struct v4l2_rect *crop);
extern int fimc_hwset_input_offset(struct fimc_control *ctrl,
						u32 pixelformat,
						struct v4l2_rect *bound,
						struct v4l2_rect *crop);
extern int fimc_hwset_org_input_size(struct fimc_control *ctrl,
						u32 width, u32 height);
extern int fimc_hwset_org_output_size(struct fimc_control *ctrl,
						u32 width, u32 height);
extern int fimc_hwset_ext_output_size(struct fimc_control *ctrl,
						u32 width, u32 height);
extern int fimc_hwset_input_addr_style(struct fimc_control *ctrl,
						u32 pixelformat);
extern int fimc_hwset_output_addr_style(struct fimc_control *ctrl,
						u32 pixelformat);
extern int fimc_hwset_jpeg_mode(struct fimc_control *ctrl, bool enable);
extern int fimc_hwget_frame_count(struct fimc_control *ctrl);
extern int fimc_hw_wait_winoff(struct fimc_control *ctrl);
extern int fimc_hw_wait_stop_input_dma(struct fimc_control *ctrl);
extern int fimc_hwset_input_lineskip(struct fimc_control *ctrl);
extern int fimc_hw_reset_camera(struct fimc_control *ctrl);
extern void fimc_hwset_stop_processing(struct fimc_control *ctrl);
extern int fimc_hwset_image_effect(struct fimc_control *ctrl);
extern int fimc_hwset_shadow_enable(struct fimc_control *ctrl);
extern int fimc_hwset_shadow_disable(struct fimc_control *ctrl);
extern void fimc_save_regs(struct fimc_control *ctrl);
extern void fimc_load_regs(struct fimc_control *ctrl);
extern void fimc_dump_regs(struct fimc_control *ctrl);

/*
 * D R I V E R  H E L P E R S
 *
*/
#define to_fimc_plat(d)		(to_platform_device(d)->dev.platform_data)

static inline struct fimc_global *get_fimc_dev(void)
{
	return fimc_dev;
}

static inline struct fimc_control *get_fimc_ctrl(int id)
{
	return &fimc_dev->ctrl[id];
}

#endif /* _FIMC_H */

