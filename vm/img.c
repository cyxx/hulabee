
#include "host_sdl2.h"
#include "img.h"
#include "util.h"
#include <jpeglib.h>
#include <jerror.h>

struct SDL_Surface *LoadImg(const uint8_t *data, int size) {
	if (memcmp(data, "FFIMGAMI", 8) != 0) {
		warning("Unsupported IMG signature");
		return 0;
	}
	SDL_Color colors[256];
	int w = 0;
	int h = 0;
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
				w = READ_LE_UINT32(data + offset + 4);
				h = READ_LE_UINT32(data + offset + 8);
				const int b1 = READ_LE_UINT32(data + offset + 12);
				const int b2 = READ_LE_UINT32(data + offset + 16);
				debug(DBG_IMG, "IMG header compression:%d w:%d h:%d b1:0x%x b2:0x%x", compression, w, h, b1, b2);
				format = (b1 == 0x80) ? BMP_FMT_PAL256 : BMP_FMT_RGB555;
			}
			break;
		case 0x434c5554: /* CLUT */
			if (len != 1024) {
				warning("Unhandled IMG CLUT len:%d", len);
			} else {
				palette = data + offset;
				for (int i = 0; i < 256; ++i) {
					colors[i].r = palette[i * 4 + 1];
					colors[i].g = palette[i * 4 + 2];
					colors[i].b = palette[i * 4 + 3];
					colors[i].a = 255;
				}
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
			SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
			if (s) {
				SDL_LockSurface(s);
				uint8_t *p = s->pixels;
				for (int y = 0; y < h; ++y) {
					memcpy(p, bits, w);
					p += s->pitch;
					bits += w;
				}
				SDL_UnlockSurface(s);
				SDL_SetPaletteColors(s->format->palette, colors, 0, 256);
			}
			return s;
		}
		break;
	default:
		error("Unhandled IMG format:%d", format);
		break;
	}
	return 0;
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

struct SDL_Surface *LoadJpg(const uint8_t *data, int size) {
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
	const int w = cinfo.output_width;
	const int h = cinfo.output_height;
	debug(DBG_IMG, "JPEG w:%d h:%d depth:%d", w, h, cinfo.data_precision);

	SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, w, h, 32, SDL_PIXELFORMAT_XBGR8888);
	if (s) {
		SDL_LockSurface(s);
		uint8_t *dst = s->pixels;
		const int dst_pitch = s->pitch;
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&dst, 1);
			dst += dst_pitch;
		}
		SDL_UnlockSurface(s);
	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);
	return s;
}
