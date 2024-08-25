
#include "can.h"
#include "host_sdl2.h"
#include "util.h"

CanData *LoadCan(const uint8_t *data, int size) {
	CanData *anim = (CanData *)malloc(sizeof(CanData));
	memset(anim, 0, sizeof(CanData));
	anim->data = data;
	anim->size = size;
	if (READ_LE_UINT32(data) != 0x4143414e) { /* 'ACAN' */
		warning("Unsupported CAN signature");
		return 0;
	}
	const int version = READ_LE_UINT32(data + 4);
	if (version != 5) {
		warning("Unsupported CAN version %d", version);
		return 0;
	}
	anim->entries_count = READ_LE_UINT32(data + 24);
	int offset = 28;
	anim->entries = (CanAnimation *)calloc(anim->entries_count, sizeof(CanAnimation));
	if (!anim->entries) {
		error("Unable to allocate %d CanAnimation", anim->entries_count);
		return 0;
	}
	for (int i = 0; i < anim->entries_count; ++i) {
		CanAnimation *frame = &anim->entries[i];
		frame->num = READ_LE_UINT32(data + offset); offset += 4;
		frame->offset = READ_LE_UINT32(data + offset); offset += 4;
	}
	anim->palettes_count = READ_LE_UINT32(data + offset); offset += 4;
	assert(anim->palettes_count > 0);
	anim->palettes = (uint32_t *)calloc(anim->palettes_count, sizeof(uint32_t));
	if (!anim->palettes) {
		error("Unable to allocate %d palette offsets", anim->palettes_count);
		return 0;
	}
	for (int i = 0; i < anim->palettes_count; ++i) {
		anim->palettes[i] = READ_LE_UINT32(data + offset); offset += 4;
	}
	debug(DBG_CAN, "CAN palettes:%d entries:%d", anim->palettes_count, anim->entries_count);
	anim->bitmaps_count = READ_LE_UINT32(data + offset); offset += 4;
	assert(anim->bitmaps_count > 0);
	anim->bitmaps = (CanBitmap *)calloc(anim->bitmaps_count, sizeof(CanBitmap));
	for (int i = 0; i < anim->bitmaps_count; ++i) {
		anim->bitmaps[i].palette_num = -1;
		anim->bitmaps[i].bitmap_offset = READ_LE_UINT32(data + offset); offset += 4;
	}
	for (int i = 0; i < anim->entries_count; ++i) {
		CanAnimation *frame = &anim->entries[i];
		offset = frame->offset;
		const uint8_t *header = data + offset; offset += 20;
		frame->frames_count = READ_LE_UINT32(header);
		const int frames_count = frame->frames_count;
		const int layers = READ_LE_UINT32(header + 4);
		frame->layers_count = layers;
		frame->rate = READ_LE_UINT32(header + 8);
		frame->pos_x = READ_LE_UINT32(header + 12);
		frame->pos_y = READ_LE_UINT32(header + 16);
		frame->bounds_x1 = READ_LE_UINT32(data + offset); offset += 4;
		frame->bounds_y1 = READ_LE_UINT32(data + offset); offset += 4;
		frame->bounds_x2 = READ_LE_UINT32(data + offset); offset += 4;
		frame->bounds_y2 = READ_LE_UINT32(data + offset); offset += 4;
		frame->default_palette_num = READ_LE_UINT32(data + offset); offset += 4;
		const int unk28 = READ_LE_UINT32(data + offset); offset += 4;
		assert(unk28 == 0);
		const int unk2c = READ_LE_UINT32(data + offset); offset += 4;
		debug(DBG_CAN, "CAN entry %d num:%d unk2c:%d bounds:%d,%d,%d,%d", i, frame->num, unk2c, frame->bounds_x1, frame->bounds_y1, frame->bounds_x2, frame->bounds_y2);
		AnimationFrameTrigger *triggers = (AnimationFrameTrigger *)calloc(frames_count, sizeof(AnimationFrameTrigger));
		for (int j = 0; j < frames_count; ++j) {
			const int len = READ_LE_UINT32(data + offset); offset += 4;
			triggers[j].count = len;
			triggers[j].offset = offset;
			for (int k = 0; k < len; ++k) {
				const int id = READ_LE_UINT32(data + offset); offset += 4;
				debug(DBG_CAN, "CAN frame %d trigger %d=%d", j, k, id);
			}
		}
		frame->triggers = triggers;
		if (layers != 0) {
			frame->offset_layers_id = offset;
			for (int j = 0; j < layers; ++j) {
				const int a = READ_LE_UINT32(data + offset); offset += 4;
				debug(DBG_CAN, "CAN layer %d ID %d", j, a);
			}
			frame->offset_layers_palette = offset;
			for (int j = 0; j < layers; ++j) {
				const int b = READ_LE_UINT32(data + offset); offset += 4;
				debug(DBG_CAN, "CAN layer %d palette %d", j, b);
			}
		}
		frame->frames = (AnimationFrameLayer *)calloc(frames_count * layers, sizeof(AnimationFrameLayer));
		if (!frame->frames) {
			error("Unable to allocate %d AnimationFrameLayer", frames_count * layers);
		} else {
			assert(layers > 0);
			for (int j = 0; j < frames_count; ++j) {
				AnimationFrameLayer *afl = &frame->frames[j * layers];
				for (int k = 0; k < layers; ++k) {
					afl[k].bitmap_num = READ_LE_UINT32(data + offset); offset += 4;
					assert(afl[k].bitmap_num < anim->bitmaps_count);
				}
				for (int k = 0; k < layers; ++k) {
					afl[k].x_pos = READ_LE_UINT32(data + offset); offset += 4;
					afl[k].y_pos = READ_LE_UINT32(data + offset); offset += 4;
				}
			}
		}
	}
	return anim;
}

void UnloadCan(CanData *anim) {
	for (int i = 0; i < anim->entries_count; ++i) {
		CanAnimation *frame = &anim->entries[i];
		free(frame->frames);
	}
	free(anim->entries);
	free(anim->palettes);
	free(anim->bitmaps);
	free(anim);
}

int GetAnimationFramesCount(CanData *anim, int num) {
	assert(num >= 0 && num < anim->entries_count);
	return anim->entries[num].frames_count;
}

int FindAnimation(CanData *anim, int num) {
	for (int i = 0; i < anim->entries_count; ++i) {
		CanAnimation *entry = &anim->entries[i];
		debug(DBG_CAN, "Animation #%d ID %d", i, entry->num);
		if (entry->num == num) {
			return i;
		}
	}
	debug(DBG_CAN, "CAN fail to find entry num %d (count %d)", num, anim->entries_count);
	return -1;
}

void GetAnimationPos(CanData *anim, int num, int *x, int *y) {
	assert(num >= 0 && num < anim->entries_count);
	CanAnimation *entry = &anim->entries[num];
	*x = entry->pos_x;
	*y = entry->pos_y;
}

void GetAnimationBounds(CanData *anim, int num, int *x1, int *y1, int *x2, int *y2) {
	assert(num >= 0 && num < anim->entries_count);
	CanAnimation *entry = &anim->entries[num];
	*x1 = entry->bounds_x1;
	*y1 = entry->bounds_y1;
	*x2 = entry->bounds_x2;
	*y2 = entry->bounds_y2;
}

void GetCanBitmapBounds(CanData *anim, int num, int frame, int *x1, int *y1, int *x2, int *y2) {
	assert(num >= 0 && num < anim->entries_count);
	CanAnimation *entry = &anim->entries[num];
	--frame;
	assert(frame >= 0 && frame < entry->frames_count);
	AnimationFrameLayer *afl = &entry->frames[frame * entry->layers_count];
	*x1 = *y1 = INT32_MAX;
	*x2 = *y2 = INT32_MIN;
	for (int i = 0; i < entry->layers_count; ++i) {
		const int bitmap_num = afl[i].bitmap_num;
		if (bitmap_num == 0) {
			continue;
		}
		if (*x1 > afl[i].x_pos) {
			*x1 = afl[i].x_pos;
		}
		if (*y1 > afl[i].y_pos) {
			*y1 = afl[i].y_pos;
		}
		uint32_t offset = anim->bitmaps[bitmap_num].bitmap_offset;
		const int w = READ_LE_UINT32(anim->data + offset); offset += 4;
		const int h = READ_LE_UINT32(anim->data + offset); offset += 4;
		if (*x2 < *x1 + w) {
			*x2 = *x1 + w;
		}
		if (*y2 < *y1 + h) {
			*y2 = *y1 + h;
		}
	}
}

static void decodeHelper(const uint8_t *data, const uint8_t *offsets, int w, int h, uint8_t *dst8, int dst_pitch) {
	for (int y = 0; y < h; ++y, offsets += 4) {
		const uint8_t *line = data + READ_LE_UINT32(offsets);
		for (int x = 0; x < w; ) {
			const int code = *line++;
			if (code == 0) {
				const int count = *line++;
				x += count;
			} else if (code == 1) {
				const int count = *line++;
				const int color = *line++;
				memset(dst8 + x, color, count);
				x += count;
			} else if (code == 2 || code == 4) {
				const int count = *line++;
				memcpy(dst8 + x, line, count);
				x += count;
				line += count;
			}
		}
		dst8 += dst_pitch;
	}
}

static void read_palette(CanData *anim, int palette_num, SDL_Color *colors) {
	const uint8_t *p = anim->data + anim->palettes[palette_num];
	for (int color = 0; color < 256; ++color, p += 4) {
		colors[color].b = p[0];
		colors[color].g = p[1];
		colors[color].r = p[2];
		colors[color].a = 0xFF;
	}
}

static SDL_Surface *create_surface(CanData *anim, uint32_t offset) {
	const int w = READ_LE_UINT32(anim->data + offset); offset += 4;
	if (w == 0) {
		return 0;
	}
	const int h = READ_LE_UINT32(anim->data + offset); offset += 4;
	if (h == 0) {
		return 0;
	}
	const int c = READ_LE_UINT32(anim->data + offset); offset += 4;
	debug(DBG_CAN, "CAN compression 0x%x w:%d h:%d offset:0x%x", c, w, h, offset);
	if (c != 0x58524c38) { /* XRL8 */
		error("Unsupported compression:0x%x", c);
		return 0;
	}
	/* const int size = READ_LE_UINT32(anim->data + offset); */ offset += 4;

	SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (s) {
		SDL_LockSurface(s);
		decodeHelper(anim->data + offset, anim->data + offset, w, h, s->pixels, s->pitch);
		SDL_UnlockSurface(s);
		SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);
		SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, 0x80, 0x00, 0x80));
	}
	return s;
}

void Can_Draw(CanData *anim, int num, int frame, struct SDL_Surface *target, int x, int y, int flags) {
	if (!(num >= 0 && num < anim->entries_count)) {
		return;
	}
	assert(num >= 0 && num < anim->entries_count);
	CanAnimation *entry = &anim->entries[num];
	if (!(frame >= 0 && frame < entry->frames_count)) {
		return;
	}
	assert(frame >= 0 && frame < entry->frames_count);
	AnimationFrameLayer *afl = &entry->frames[frame * entry->layers_count];
	const uint8_t *layer_palette = anim->data + entry->offset_layers_palette;
	for (int i = 0; i < entry->layers_count; ++i, ++afl) {
		int palette_num = READ_LE_UINT32(layer_palette); layer_palette += 4;
		if (afl->bitmap_num != 0) {
			CanBitmap *b = &anim->bitmaps[afl->bitmap_num];
			if (!b->s) {
				b->s = create_surface(anim, b->bitmap_offset);
				if (!b->s) {
					continue;
				}
			}
			if (palette_num == -1) {
			       palette_num = entry->default_palette_num;
			}
			if (b->palette_num != palette_num) {
				SDL_Color colors[256];
				read_palette(anim, palette_num, colors);
				SDL_SetPaletteColors(b->s->format->palette, colors, 0, 256);
				b->palette_num = palette_num;
			}
			SDL_Rect dst;
			dst.x = afl->x_pos;
			dst.y = afl->y_pos;
			dst.w = b->s->w;
			dst.h = b->s->h;
			SDL_BlitSurface(b->s, 0, target, &dst);
		}
	}
}

void Can_Reset(CanData *anim, CanAnimationState *state, int timestamp) {
	memset(state, 0, sizeof(CanAnimationState));
	state->timestamp = timestamp;
}

void Can_Update(CanData *anim, CanAnimationState *state, int timestamp) {
	CanAnimation *entry = &anim->entries[state->current_animation];
	if (entry->rate == 0) {
		return;
	}
	const int count = (timestamp - state->timestamp) / entry->rate;
	if (count == 0) {
		return;
	}
	state->current_frame += count;
	if (state->current_frame >= entry->frames_count) {
		if (state->loop) {
			state->current_frame %= entry->frames_count;
		} else {
			state->current_frame = entry->frames_count - 1;
		}
	}
	state->timestamp = timestamp;
}

bool Can_HasTrigger(CanData *anim, CanAnimationState *state, int frame, int trigger) {
	CanAnimation *entry = &anim->entries[state->current_animation];
	const int triggers_count = entry->triggers[frame].count;
	if (triggers_count != 0) {
		const uint8_t *p = anim->data + entry->triggers[state->current_frame].offset;
		for (int i = 0; i < triggers_count; ++i, p += 4) {
			if (READ_LE_UINT32(p) == trigger) {
				return true;
			}
		}
	}
	return false;
}

int Can_GetTriggersCount(CanData *anim, CanAnimationState *state, int frame) {
	CanAnimation *entry = &anim->entries[state->current_animation];
	const int triggers_count = entry->triggers[frame].count;
	return triggers_count;
}

bool Can_Done(CanData *anim, CanAnimationState *state) {
	CanAnimation *entry = &anim->entries[state->current_animation];
	debug(DBG_CAN, "Can_Done loop:%d frame %d/%d", state->loop, state->current_frame, entry->frames_count);
	return state->loop == 0 && state->current_frame == (entry->frames_count - 1);
}
