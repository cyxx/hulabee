
#include "host_sdl2.h"
#include "img.h"
#include "util.h"
#include <jpeglib.h>
#include <jerror.h>

struct decode_img_t {
	const uint8_t *data;
	const uint8_t *palette;
	int w, h;
};

static void decodeImgHelper(void *userdata, uint32_t *dst, int dst_pitch) {
	struct decode_img_t *d = (struct decode_img_t *)userdata;
	const uint8_t *palette = d->palette;
	uint32_t pal[256];
	for (int color = 0; color < 256; ++color) {
		const uint8_t r = palette[color * 4 + 1];
		const uint8_t g = palette[color * 4 + 2];
		const uint8_t b = palette[color * 4 + 3];
		pal[color] = 0xFF000000 | (r << 16) | (g << 8) | b;
	}
	const uint8_t *p = d->data;
	for (int y = 0; y < d->h; ++y) {
		for (int x = 0; x < d->w; ++x) {
			dst[x] = pal[*p++];
		}
		dst += dst_pitch;
	}
}

void LoadImg(const uint8_t *data, int size, Bitmap *bmp) {
	memset(bmp, 0, sizeof(Bitmap));
	bmp->texture = -1;
	if (memcmp(data, "FFIMGAMI", 8) != 0) {
		warning("Unsupported IMG signature");
		return;
	}
	int format = 0;
	const uint8_t *bits = 0;
	const uint8_t *palette = 0;
	for (int offset = 12; offset + 8 < size; ) {
		const int tag = Read32(data, size, &offset);
		const int len = Read32(data, size, &offset);
		debug(DBG_IMG, "IMG tag:0x%x len:%d", tag, len);
		if (offset + len > size) {
			break;
		}
		switch (tag) {
		case 0x48454144: /* HEAD */
			if (len != 24) {
				warning("Unhandled IMG HEAD len:%d", len);
			} else {
				const int compression = READ_LE_UINT32(data + offset);
				bmp->w = READ_LE_UINT32(data + offset + 4);
				bmp->h = READ_LE_UINT32(data + offset + 8);
				const int b1 = READ_LE_UINT32(data + offset + 12);
				const int b2 = READ_LE_UINT32(data + offset + 16);
				debug(DBG_IMG, "IMG header compression:%d w:%d h:%d b1:0x%x b2:0x%x", compression, bmp->w, bmp->h, b1, b2);
				format = (b1 == 0x80) ? BMP_FMT_PAL256 : BMP_FMT_RGB555;
			}
			break;
		case 0x434c5554: /* CLUT */
			if (len != 1024) {
				warning("Unhandled IMG CLUT len:%d", len);
			} else {
				palette = data + offset;
			}
			break;
		case 0x44415441: /* DATA */
			bits = data + offset;
			break;
		default:
			warning("Unhandled IMG tag:0x%x len:%d", tag, len);
			break;
		}
		offset += len;
	}
	switch (format) {
	case BMP_FMT_PAL256:
		if (bits && palette) {
			struct decode_img_t d;
			memset(&d, 0, sizeof(struct decode_img_t));
			d.data = bits;
			d.palette = palette;
			d.w = bmp->w;
			d.h = bmp->h;
			bmp->texture = CreateTexture(bmp->w, bmp->h, SDL_PIXELFORMAT_ARGB8888, decodeImgHelper, &d);
		}
		break;
	default:
		error("Unhandled IMG format:%d", format);
		break;
	}
}

static void decodeJpgHelper(void *userdata, uint32_t *dst, int dst_pitch) {
	struct jpeg_decompress_struct *cinfo = (struct jpeg_decompress_struct *)userdata;
	while (cinfo->output_scanline < cinfo->output_height) {
		jpeg_read_scanlines(cinfo, (JSAMPARRAY)&dst, 1);
		dst += dst_pitch;
	}
}

static void error_exit(j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message)(cinfo, buffer);
	warning("libjpeg: %s", buffer);
}

static void output_message(j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message)(cinfo, buffer);
	debug(DBG_IMG, "libjpeg: %s", buffer);
}

void LoadJpg(const uint8_t *data, int size, Bitmap *bmp) {
	debug(DBG_IMG, "LoadJpg size:%d", size);
	struct jpeg_decompress_struct cinfo;
	memset(&cinfo, 0, sizeof(cinfo));

	struct jpeg_error_mgr jerr;
	memset(&jerr, 0, sizeof(jerr));
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = error_exit;
	cinfo.err->output_message = output_message;

	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, data, size);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_EXT_RGBX;

	jpeg_start_decompress(&cinfo);
	bmp->w = cinfo.output_width;
	bmp->h = cinfo.output_height;
	debug(DBG_IMG, "JPEG w:%d h:%d depth:%d", bmp->w, bmp->h, cinfo.data_precision);
	bmp->texture = CreateTexture(bmp->w, bmp->h, SDL_PIXELFORMAT_XBGR8888, decodeJpgHelper, &cinfo);
	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);
}
