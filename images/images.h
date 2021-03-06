/*
 *	Image Library -- Main header file
 *
 *	(c) 2006 Pavel Charvat <pchar@ucw.cz>
 *
 *	This software may be freely distributed and used according to the terms
 *	of the GNU Lesser General Public License.
 */

#ifndef _IMAGES_IMAGES_H
#define _IMAGES_IMAGES_H

#include <ucw/bbuf.h>

#ifdef CONFIG_UCW_CLEAN_ABI
#define image_channels_format_to_name ucw_image_channels_format_to_name
#define image_clear ucw_image_clear
#define image_clone ucw_image_clone
#define image_context_cleanup ucw_image_context_cleanup
#define image_context_init ucw_image_context_init
#define image_context_msg ucw_image_context_msg
#define image_context_msg_default ucw_image_context_msg_default
#define image_context_msg_silent ucw_image_context_msg_silent
#define image_context_vmsg ucw_image_context_vmsg
#define image_destroy ucw_image_destroy
#define image_dimensions_fit_to_box ucw_image_dimensions_fit_to_box
#define image_extension_to_format ucw_image_extension_to_format
#define image_file_name_to_format ucw_image_file_name_to_format
#define image_format_to_extension ucw_image_format_to_extension
#define image_init_matrix ucw_image_init_matrix
#define image_init_subimage ucw_image_init_subimage
#define image_io_cleanup ucw_image_io_cleanup
#define image_io_init ucw_image_io_init
#define image_io_read ucw_image_io_read
#define image_io_read_data ucw_image_io_read_data
#define image_io_read_header ucw_image_io_read_header
#define image_io_reset ucw_image_io_reset
#define image_io_write ucw_image_io_write
#define image_max_bytes ucw_image_max_bytes
#define image_max_dim ucw_image_max_dim
#define image_name_to_channels_format ucw_image_name_to_channels_format
#define image_new ucw_image_new
#define image_scale ucw_image_scale
#endif

struct mempool;
struct fastbuf;


/* context.c
 * - contexts with error/message handling
 * - imagelib is thread-safe as long as threads work in different contexts */

struct image_context {
  byte *msg;				/* last message */
  uint msg_code;			/* last message code (see images/error.h for details) */
  bb_t msg_buf;				/* message buffer */
  void (*msg_callback)(struct image_context *ctx); /* called for each message (in msg_{str,code}) */
  uint tracing_level;			/* tracing level (zero to disable) */
};

/* initialization/cleanup */
void image_context_init(struct image_context *ctx);
void image_context_cleanup(struct image_context *ctx);

/* message handling, see images/error.h for useful macros */
void image_context_msg(struct image_context *ctx, uint code, char *msg, ...);
void image_context_vmsg(struct image_context *ctx, uint code, char *msg, va_list args);

/* default callback, displays messages with standard libucw's log() routine */
void image_context_msg_default(struct image_context *ctx);

/* empty callback */
void image_context_msg_silent(struct image_context *ctx);


/* image.c
 * - basic manipulation with images
 * - image structure is not directly connected to a single context
 *   but manipulation routines are (user must synchronize the access himself)! */

extern uint image_max_dim;		/* ImageLib.ImageMaxDim */
extern uint image_max_bytes;		/* ImageLib.ImageMaxBytes */

/* SSE aligning size, see IMAGE_SSE_ALIGNED */
#define IMAGE_SSE_ALIGN_SIZE 16

enum image_flag {
  IMAGE_COLOR_SPACE = 0xf,		/* mask for enum color_space */
  IMAGE_ALPHA = 0x10,			/* alpha channel */
  IMAGE_PIXELS_ALIGNED = 0x20,		/* align pixel size to the nearest power of two  */
  IMAGE_SSE_ALIGNED = 0x40,		/* align scanlines to multiples of 16 bytes (both start and size) */
  IMAGE_NEED_DESTROY = 0x80,		/* image is allocated with xmalloc */
  IMAGE_GAPS_PROTECTED = 0x100,		/* cannot access gaps between rows */
  IMAGE_CHANNELS_FORMAT = IMAGE_COLOR_SPACE | IMAGE_ALPHA,
  IMAGE_PIXEL_FORMAT = IMAGE_CHANNELS_FORMAT | IMAGE_PIXELS_ALIGNED,
  IMAGE_ALIGNED = IMAGE_PIXELS_ALIGNED | IMAGE_SSE_ALIGNED,
  IMAGE_NEW_FLAGS = IMAGE_PIXEL_FORMAT | IMAGE_SSE_ALIGNED,
  IMAGE_INTERNAL_FLAGS = IMAGE_NEED_DESTROY | IMAGE_GAPS_PROTECTED,
};

#define IMAGE_MAX_CHANNELS 4
#define IMAGE_CHANNELS_FORMAT_MAX_SIZE 128
byte *image_channels_format_to_name(uint format, byte *buf);
uint image_name_to_channels_format(byte *name);

struct color {
  byte c[IMAGE_MAX_CHANNELS];
  byte color_space;
};

struct image {
  byte *pixels;			/* aligned top left pixel, there are at least sizeof(uint)
				   unused bytes after the buffer (possible optimizations) */
  uint cols;			/* number of columns */
  uint rows;			/* number of rows */
  uint channels;		/* number of color channels including the alpha channel */
  uint pixel_size;		/* size of pixel in bytes (1, 2, 3 or 4) */
  uint row_size;		/* scanline size in bytes */
  uint row_pixels_size;		/* scanline size in bytes excluding rows gaps */
  uint image_size;		/* rows * row_size */
  uint flags;			/* enum image_flag */
};

struct image *image_new(struct image_context *ctx, uint cols, uint rows, uint flags, struct mempool *pool);
struct image *image_clone(struct image_context *ctx, struct image *src, uint flags, struct mempool *pool);
void image_destroy(struct image *img);
void image_clear(struct image_context *ctx, struct image *img);
struct image *image_init_matrix(struct image_context *ctx, struct image *img, byte *pixels, uint cols, uint rows, uint row_size, uint flags);
struct image *image_init_subimage(struct image_context *ctx, struct image *img, struct image *src, uint left, uint top, uint cols, uint rows);

static inline int image_dimensions_valid(uint cols, uint rows)
{
  return cols && rows && cols <= image_max_dim && rows <= image_max_dim;
}
/* scale.c */

int image_scale(struct image_context *ctx, struct image *dest, struct image *src);
void image_dimensions_fit_to_box(uint *cols, uint *rows, uint max_cols, uint max_rows, uint upsample);

/* image-io.c */

enum image_format {
  IMAGE_FORMAT_UNDEFINED,
  IMAGE_FORMAT_JPEG,
  IMAGE_FORMAT_PNG,
  IMAGE_FORMAT_GIF,
  IMAGE_FORMAT_MAX
};

struct image_io {
					/*  R - read_header input */
					/*   H - read_header output */
					/*    I - read_data input */
					/*     O - read_data output */
					/*      W - write input */

  struct image *image;			/* [   OW] - image data */
  enum image_format format;		/* [R   W] - file format (IMAGE_FORMAT_x) */
  struct fastbuf *fastbuf;		/* [R   W] - source/destination stream */
  struct mempool *pool;			/* [  I  ] - parameter to image_new */
  uint cols;				/* [ HI  ] - number of columns, parameter to image_new */
  uint rows;				/* [ HI  ] - number of rows, parameter to image_new */
  uint flags;				/* [ HI  ] - see enum image_io_flags */
  uint jpeg_quality;			/* [    W] - JPEG compression quality (1..100) */
  uint number_of_colors;		/* [ H   ] - number of image colors */
  struct color background_color;	/* [ HI  ] - background color, zero if undefined */
  uint exif_size;			/* [ H  W] - EXIF size in bytes (zero if not present) */
  byte *exif_data;			/* [ H  W] - EXIF data */

  /* internals */
  struct image_context *context;
  struct mempool *internal_pool;
  void *read_data;
  void (*read_cancel)(struct image_io *io);
};

enum image_io_flags {
  IMAGE_IO_IMAGE_FLAGS = 0xffff,	/* [ HI  ] - mask of parameters to image new, read_header fills IMAGE_CHANNELS_FORMAT */
  IMAGE_IO_NEED_DESTROY = 0x10000,	/* [   O ] - enables automatic call of image_destroy */
  IMAGE_IO_HAS_PALETTE = 0x20000,	/* [ H   ] - true for image with indexed colors */
  IMAGE_IO_USE_BACKGROUND = 0x40000,	/* [  I  ] - merge transparent pixels with background_color */
  IMAGE_IO_WANT_EXIF = 0x80000,		/* [R    ] - read EXIF data if present */
};

int image_io_init(struct image_context *ctx, struct image_io *io);
void image_io_cleanup(struct image_io *io);
void image_io_reset(struct image_io *io);

int image_io_read_header(struct image_io *io);
struct image *image_io_read_data(struct image_io *io, int ref);
struct image *image_io_read(struct image_io *io, int ref);

int image_io_write(struct image_io *io);

byte *image_format_to_extension(enum image_format format);
enum image_format image_extension_to_format(byte *extension);
enum image_format image_file_name_to_format(byte *file_name);

#endif
