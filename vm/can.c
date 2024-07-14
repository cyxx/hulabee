
#include "can.h"
#include "host_sdl2.h"
#include "util.h"

void LoadCan(const uint8_t *data, int size, Animation *anim) {
	memset(anim, 0, sizeof(Animation));
	anim->data = data;
	anim->size = size;
	/* 'ACAN' */
	anim->entries_count = READ_LE_UINT32(data + 24);
	int offset = 28;
	anim->entries = (AnimationFrame *)calloc(anim->entries_count, sizeof(AnimationFrame));
	if (!anim->entries) {
		error("Unable to allocate %d AnimationFrame", anim->entries_count);
		return;
	}
	for (int i = 0; i < anim->entries_count; ++i) {
		AnimationFrame *frame = &anim->entries[i];
		frame->num = READ_LE_UINT32(data + offset); offset += 4;
		frame->offset = READ_LE_UINT32(data + offset); offset += 4;
	}
	const int palettes_count = READ_LE_UINT32(data + offset); offset += 4;
	const int palette_offset = READ_LE_UINT32(data + offset); offset += 4;
	for (int i = 1; i < palettes_count; ++i) {
		offset += 4;
	}
	debug(DBG_CAN, "CAN palettes:%d entries:%d", palettes_count, anim->entries_count);
	anim->bitmaps_count = READ_LE_UINT32(data + offset); offset += 4;
	anim->bitmaps = (AnimationBitmap *)calloc(anim->bitmaps_count, sizeof(AnimationBitmap));
	for (int i = 0; i < anim->bitmaps_count; ++i) {
		anim->bitmaps[i].palette_offset = palette_offset;
		anim->bitmaps[i].bitmap_offset = READ_LE_UINT32(data + offset); offset += 4;
	}
	for (int i = 0; i < anim->entries_count; ++i) {
		AnimationFrame *frame = &anim->entries[i];
		offset = frame->offset;
		const uint8_t *header = data + offset; offset += 0x14;
		frame->frames_count = READ_LE_UINT32(header);
		const int unk04 = READ_LE_UINT32(header + 4);
		frame->bounds_x1 = READ_LE_UINT32(data + offset); offset += 4;
		frame->bounds_y1 = READ_LE_UINT32(data + offset); offset += 4;
		frame->bounds_x2 = READ_LE_UINT32(data + offset); offset += 4;
		frame->bounds_y2 = READ_LE_UINT32(data + offset); offset += 4;
		const int unk24 = READ_LE_UINT32(data + offset); offset += 4;
		const int unk28 = READ_LE_UINT32(data + offset); offset += 4;
		assert(unk28 == 0);
		const int unk2c = READ_LE_UINT32(data + offset); offset += 4;
		debug(DBG_CAN, "CAN entry %d num:%d count:%d bounds:%d,%d,%d,%d unk24:%d", i, frame->num, unk2c, frame->bounds_x1, frame->bounds_y1, frame->bounds_x2, frame->bounds_y2, unk24);
		// assert(unk2c == anim->entries[i].frames_count);
		for (int j = 0; j < unk2c; ++j) {
			int len = READ_LE_UINT32(data + offset); offset += 4;
			// assert(len == 0);
			offset += 4 * len;
		}
		int count = unk04 * 4;
		if (count != 0) {
			offset += count * 2;
		}
		frame->frames = (AnimationEntryFrame *)calloc(unk2c, sizeof(AnimationEntryFrame));
		if (!frame->frames) {
			error("Unable to allocate %d AnimationEntryFrame", unk2c);
		} else {
			for (int j = 0; j < unk2c; ++j) {
				AnimationEntryFrame *aef = &frame->frames[j];
				aef->bitmap_num = READ_LE_UINT32(data + offset); offset += 4;
				aef->x_pos = READ_LE_UINT32(data + offset); offset += 4;
				aef->y_pos = READ_LE_UINT32(data + offset); offset += 4;
			}
		}
	}
}

int GetAnimationFramesCount(Animation *anim, int num) {
	return anim->entries[num].frames_count;
}

int FindAnimation(Animation *anim, int num) {
	for (int i = 0; i < anim->entries_count; ++i) {
		AnimationFrame *entry = &anim->entries[i];
		if (entry->num == num) {
			return i;
		}
	}
	warning("CAN fail to find entry num %d", num);
	return 0;
}

int GetAnimationFrameBounds(Animation *anim, int num, int *x1, int *y1, int *x2, int *y2) {
	assert(num >= 0 && num < anim->entries_count);
	AnimationFrame *entry = &anim->entries[num];
	*x1 = entry->bounds_x1;
	*y1 = entry->bounds_y1;
	*x2 = entry->bounds_x2;
	*y2 = entry->bounds_y2;
	return 0;
}

int GetAnimationBitmapBounds(Animation *anim, int num, int frame, int *x1, int *y1, int *x2, int *y2) {
	assert(frame != 0);
	--frame;
	if (frame < anim->entries[num].frames_count) {
		*x1 = anim->entries[num].frames[frame].x_pos;
		if (*x1 == 0) {
			*x1 = anim->entries[num].bounds_x1;
		}
		*y1 = anim->entries[num].frames[frame].y_pos;
		if (*y1 == 0) {
			*y1 = anim->entries[num].bounds_y1;
		}
		const int bitmap_num = anim->entries[num].frames[frame].bitmap_num;
		uint32_t offset = anim->bitmaps[bitmap_num].bitmap_offset;
		const int w = READ_LE_UINT32(anim->data + offset); offset += 4;
		const int h = READ_LE_UINT32(anim->data + offset); offset += 4;
		*x2 = *x1 + w;
		*y2 = *y1 + h;
		return 0;
	}
	return -1;
}

struct decode_t {
	const uint8_t *data;
	const uint8_t *offsets;
	const uint32_t *palette;
	int size;
	int w, h;
};

static void decodeHelper(void *userdata, uint32_t *dst, int dst_pitch) {
	struct decode_t *d = (struct decode_t *)userdata;
	const uint8_t *offsets = d->offsets;
	for (int y = 0; y < d->h; ++y, offsets += 4) {
		const uint8_t *line = d->data + READ_LE_UINT32(offsets);
		for (int x = 0; x < d->w; ) {
			const int code = *line++;
			if (code == 0) {
				const int count = *line++;
				x += count;
			} else if (code == 1) {
				const int count = *line++;
				const int color = *line++;
				const uint32_t rgb = d->palette[color];
				if ((rgb & 0xFFFFFF) != 0x800080) {
					for (int i = 0; i < count; ++i) {
						dst[x] = rgb;
						++x;
					}
				} else {
					x += count;
				}
			} else if (code == 2 || code == 4) {
				const int count = *line++;
				for (int i = 0; i < count; ++i) {
					const int color = *line++;
					const uint32_t rgb = d->palette[color];
					if ((rgb & 0xFEFFFE) != 0x800080) {
						dst[x] = rgb;
					}
					++x;
				}
			}
		}
		dst += dst_pitch;
	}
}

int GetCanBitmap(Animation *anim, int num, int anim_num, int *x_pos, int *y_pos) {
	assert(num >= 0 && num < anim->entries_count);
	AnimationFrame *entry = &anim->entries[num];
	const int frame = entry->frames[anim_num].bitmap_num;
	AnimationBitmap *b = &anim->bitmaps[frame];
	*x_pos = entry->frames[anim_num].x_pos;
	*y_pos = entry->frames[anim_num].y_pos;
	if (b->texture != -1) {
		//return b->texture;
	}
	uint32_t offset = b->bitmap_offset;
	const int w = READ_LE_UINT32(anim->data + offset); offset += 4;
	if (w == 0) {
		return -1;
	}
	const int h = READ_LE_UINT32(anim->data + offset); offset += 4;
	if (h == 0) {
		return -1;
	}
	const int c = READ_LE_UINT32(anim->data + offset); offset += 4;
	debug(DBG_CAN, "CAN compression 0x%x w:%d h:%d offset:0x%x", c, w, h, offset);
	if (c != 0x58524c38) {
		error("Unsupported compression:0x%x", c);
		return -1;
	}
	const uint8_t *palette = anim->data + b->palette_offset;
	uint32_t pal[256];
	for (int color = 0; color < 256; ++color) {
		const uint8_t b = palette[color * 4];
		const uint8_t g = palette[color * 4 + 1];
		const uint8_t r = palette[color * 4 + 2];
		pal[color] = 0xFF000000 | (r << 16) | (g << 8) | b;
	}
	const int size = READ_LE_UINT32(anim->data + offset); offset += 4;
	struct decode_t d;
	memset(&d, 0, sizeof(struct decode_t));
	d.data = anim->data + offset;
	d.offsets = anim->data + offset;
	d.palette = pal;
	d.size = size;
	d.w = w;
	d.h = h;
	b->texture = CreateTexture(w, h, SDL_PIXELFORMAT_ARGB8888, decodeHelper, &d);
	return b->texture;
}
