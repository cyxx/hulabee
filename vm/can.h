
#ifndef CAN_H__
#define CAN_H__

#include "intern.h"

struct SDL_SUrface;

typedef struct {
	int bitmap_num;
	int x_pos;
	int y_pos;
} AnimationFrameLayer;

typedef struct {
	int count;
	uint32_t offset;
} AnimationFrameTrigger;

typedef struct {
	int num; /* animation ID */
	uint32_t offset;
	int rate;
	int pos_x, pos_y;
	int bounds_x1, bounds_y1, bounds_x2, bounds_y2;
	int default_palette_num;
	int frames_count;
	int layers_count;
	AnimationFrameLayer *frames;
	AnimationFrameTrigger *triggers;
	uint32_t offset_layers_id;
	uint32_t offset_layers_palette;
} CanAnimation;

typedef struct {
	uint32_t bitmap_offset;
	int palette_num;
	struct SDL_Surface *s;
} CanBitmap;

typedef struct can_animation_data_t {
	const uint8_t *data;
	int size;
	int entries_count;
	CanAnimation *entries;
	int palettes_count;
	uint32_t *palettes;
	int bitmaps_count;
	CanBitmap *bitmaps;
} CanData;

typedef struct can_animation_state_t {
	int current_animation;
	int current_frame;
	int loop;
	int timestamp;
} CanAnimationState;

CanData *LoadCan(const uint8_t *data, int size);
void UnloadCan(CanData *);

int FindAnimation(CanData *anim, int num);
int GetAnimationFramesCount(CanData *anim, int num);
void GetAnimationPos(CanData *anim, int num, int *x, int *y);
void GetAnimationBounds(CanData *anim, int num, int *x1, int *y1, int *x2, int *y2);
void GetCanBitmapBounds(CanData *anim, int num, int frame, int *x1, int *y1, int *x2, int *y2);

void Can_Draw(CanData *anim, int num, int frame, struct SDL_Surface *, int x, int y, int flags);
void Can_Reset(CanData *anim, CanAnimationState *state, int timestamp);
void Can_Update(CanData *anim, CanAnimationState *state, int timestamp);
bool Can_HasTrigger(CanData *anim, CanAnimationState *state, int frame, int trigger);
int Can_GetTriggersCount(CanData *anim, CanAnimationState *state, int frame);
bool Can_Done(CanData *anim, CanAnimationState *state);

#endif /* CAN_H__ */
