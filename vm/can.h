
#ifndef CAN_H__
#define CAN_H__

#include "intern.h"

typedef struct {
	int bitmap_num;
	int x_pos;
	int y_pos;
} AnimationEntryFrame;

typedef struct {
	int num;
	uint32_t offset;
	int bounds_x1, bounds_y1, bounds_x2, bounds_y2;
	int frames_count;
	AnimationEntryFrame *frames;
} AnimationFrame;

typedef struct {
	uint32_t palette_offset;
	uint32_t bitmap_offset;
	int texture;
} AnimationBitmap;

typedef struct animation_t {
	const uint8_t *data;
	int size;
	int entries_count;
	AnimationFrame *entries;
	int bitmaps_count;
	AnimationBitmap *bitmaps;
} Animation;

void LoadCan(const uint8_t *data, int size, Animation *anim);
int FindAnimation(Animation *anim, int num);
int GetAnimationFramesCount(Animation *anim, int num);
int GetAnimationFrameBounds(Animation *anim, int num, int *x1, int *y1, int *x2, int *y2);
int GetAnimationBitmapBounds(Animation *anim, int num, int frame, int *x1, int *y1, int *x2, int *y2);
int GetCanBitmap(Animation *anim, int num, int frame_num, int *x, int *y);

#endif /* CAN_H__ */
