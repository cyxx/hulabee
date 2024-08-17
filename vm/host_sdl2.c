
#include "can.h"
#include "img.h"
#include "host_sdl2.h"
#include "util.h"

SDL_Window *g_window;

static SDL_Surface *_background;

#define IMAGES_COUNT 32

static HostImage _images[IMAGES_COUNT];
static int _imagesCount;

int Host_CreateImage(SDL_Surface *s) {
	const int num = _imagesCount;
	assert(num < IMAGES_COUNT);
	HostImage *img = &_images[_imagesCount++];
	memset(img, 0, sizeof(HostImage));
	img->handle = num;
	img->s = s;
	return num;
}

HostImage *HostImage_Get(int handle) {
	assert(handle >= 0 && handle < IMAGES_COUNT);
	HostImage *img = &_images[handle];
	assert(img->handle == handle);
	return img;
}

#define SPRITES_COUNT 64

typedef struct {
	CanData animation_data;
} Sprite;

static HostSprite _sprites2[SPRITES_COUNT];
static Sprite _sprites[SPRITES_COUNT];
static int _spritesCount;

int Host_CreateSprite() {
	const int num = _spritesCount;
	assert(num < SPRITES_COUNT);
	Sprite *spr = &_sprites[_spritesCount++];
	memset(spr, 0, sizeof(Sprite));
	HostSprite *spr2 = &_sprites2[num];
	spr2->handle = num;
	return num;
}

HostSprite *HostSprite_Get(int handle) {
	assert(handle >= 0 && handle < SPRITES_COUNT);
	HostSprite *spr = &_sprites2[handle];
	return spr;
}

static Sprite *getSprite(int num) {
	assert(num >= 0 && num < _spritesCount);
	return &_sprites[num];
}

int Host_GetSpriteAnim(int spr_num) {
	HostSprite *spr = HostSprite_Get(spr_num);
	int current_anim = 0;
	if (spr->animation_state) {
		current_anim = spr->animation_state->current_animation;
	}
	return current_anim;
}

void Host_SetSpriteAnim(int spr_num, int anim) {
	Sprite *spr = getSprite(spr_num);
	HostSprite *spr2 = &_sprites2[spr_num];
	GetAnimationPos(&spr->animation_data, anim, &spr2->x, &spr2->y);
	if (!spr2->animation_state) {
		spr2->animation_state = (CanAnimationState *)malloc(sizeof(CanAnimationState));
	}
	if (spr2->animation_state) {
		Can_Reset(&spr->animation_data, spr2->animation_state, SDL_GetTicks());
		spr2->animation_state->current_animation = anim;
	}
}

void Host_SetSpritePos(int spr_num, int x, int y) {
	HostSprite *spr = &_sprites2[spr_num];
	spr->x = x;
	spr->y = y;
}

void Host_SetSpriteImage(int spr_num, SDL_Surface *s) {
	HostSprite *spr2 = &_sprites2[spr_num];
	spr2->image = s;
}

CanData *Host_GetSpriteAnimationData(int spr_num) {
	Sprite *spr = getSprite(spr_num);
	return &spr->animation_data;
}

void Host_GetSpriteSize(int spr_num, int *w, int *h) {
	Sprite *spr = getSprite(spr_num);
	HostSprite *spr2 = &_sprites2[spr_num];
	if (spr2->image) {
		*w = spr2->image->w;
		*h = spr2->image->h;
	} else {
		HostSprite *spr2 = &_sprites2[spr_num];
		int current_anim = 0;
		if (spr2->animation_state) {
			current_anim = spr2->animation_state->current_animation;
		}
		int x1, y1, x2, y2;
		GetAnimationBounds(&spr->animation_data, current_anim, &x1, &y1, &x2, &y2);
		if (w) {
			*w = x2 - x1 + 1;
		}
		if (h) {
			*h = y2 - y1 + 1;
		}
	}
}

void Host_SetWindowBackground(SDL_Surface *s) {
	_background = s;
}

static int _prevButtons, _currentButtons;

int Host_GetLeftClick() {
	return (_prevButtons & SDL_BUTTON_LEFT) != 0 && (_currentButtons & SDL_BUTTON_LEFT) == 0;
}

int Host_GetRightClick() {
	return (_prevButtons & SDL_BUTTON_RIGHT) != 0 && (_currentButtons & SDL_BUTTON_RIGHT) == 0;
}

void Host_Init(const char *window_name, int window_w, int window_h) {
	SDL_Init(SDL_INIT_VIDEO);
	g_window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, 0 /* flags */);
}

void Host_Fini() {
	SDL_DestroyWindow(g_window);
	SDL_Quit();
}

static void animate_sprites() {
	for (int i = 0; i < _spritesCount; ++i) {
		HostSprite *spr2 = &_sprites2[i];
		if (spr2->animation_state) {
			Sprite *spr = &_sprites[i];
			Can_Update(&spr->animation_data, spr2->animation_state, SDL_GetTicks());
		}
	}
}

static int compareSpriteOrder(const void *a, const void *b) {
	const HostSprite *spr1 = (const HostSprite *)a;
	const HostSprite *spr2 = (const HostSprite *)b;
	return spr1->order - spr2->order;
}

static void draw() {
	SDL_Surface *screen = SDL_GetWindowSurface(g_window);
	SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
	if (_background) {
		SDL_BlitSurface(_background, 0, screen, 0);
	}
	HostSprite sprites[SPRITES_COUNT];
	memcpy(sprites, _sprites2, _spritesCount * sizeof(HostSprite));
	qsort(sprites, _spritesCount, sizeof(HostSprite), compareSpriteOrder);
	for (int i = 0; i < _spritesCount; ++i) {
		HostSprite *spr2 = &sprites[i];
		Sprite *spr = &_sprites[spr2->handle];
		if (!spr2->hidden) {
			SDL_Surface *s = spr2->image;
			if (s) {
				SDL_Rect dst;
				dst.x = spr2->x;
				dst.y = spr2->y;
				dst.w = s->w;
				dst.h = s->h;
				SDL_BlitSurface(s, 0, screen, &dst);
			} else {
				int current_anim = 0;
				int current_frame = 0;
				if (spr2->animation_state) {
					current_anim = spr2->animation_state->current_animation;
					current_frame = spr2->animation_state->current_frame;
				}
				Can_Draw(&spr->animation_data, current_anim, current_frame, screen, spr2->x, spr2->y, 0 /* flags */);
			}
		}
	}
	SDL_UpdateWindowSurface(g_window);
}

void Host_MainLoop(int interval, void (*update)(void *), void *userdata) {
	int quit = 0;
	while (!quit) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quit = 1;
				break;
			}
		}
		_prevButtons = _currentButtons;
		_currentButtons = SDL_GetMouseState(0, 0);
		update(userdata);
		animate_sprites();
		draw();
		SDL_Delay(interval);
	}
}
