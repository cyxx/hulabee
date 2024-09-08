
#include "can.h"
#include "img.h"
#include "host_sdl2.h"
#include "util.h"

SDL_Window *g_window;
SDL_Surface *g_background;

#define IMAGES_COUNT 32

static HostImage _images[IMAGES_COUNT];
static int _imagesCount;

int Host_ImageNew() {
	const int num = _imagesCount;
	assert(num < IMAGES_COUNT);
	HostImage *img = &_images[_imagesCount++];
	memset(img, 0, sizeof(HostImage));
	img->handle = num;
	return num;
}

void Host_ImageCreate(int handle, int w, int h, int depth) {
	HostImage *img = Host_ImageGet(handle);
	assert(!img->s);
	img->s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
}

void Host_ImageDelete(int handle) {
	HostImage *img = Host_ImageGet(handle);
	if (img->s) {
		SDL_FreeSurface(img->s);
	}
	memset(img, 0, sizeof(HostImage));
}

HostImage *Host_ImageGet(int handle) {
	assert(handle >= 0 && handle < IMAGES_COUNT);
	HostImage *img = &_images[handle];
	assert(img->handle == handle);
	return img;
}

#define CURSORS_COUNT 32

static HostCursor _cursors[CURSORS_COUNT];
static int _cursorsCount;

int Host_CursorNew() {
	const int num  = _cursorsCount;
	HostCursor *cur = &_cursors[_cursorsCount++];
	memset(cur, 0, sizeof(HostCursor));
	cur->handle = num;
	return num;
}

void Host_CursorCreate(int handle, HostImage *img) {
	HostCursor *cursor = Host_CursorGet(handle);
	assert(!cursor->c);
	cursor->c = SDL_CreateColorCursor(img->s, 1, 1);
}

void Host_CursorDelete(int handle) {
	HostCursor *cur = Host_CursorGet(handle);
	if (cur->c) {
		SDL_FreeCursor(cur->c);
	}
	memset(cur, 0, sizeof(HostCursor));
}

HostCursor *Host_CursorGet(int handle) {
	assert(handle >= 0 && handle < CURSORS_COUNT);
	HostCursor *cur = &_cursors[handle];
	assert(cur->handle == handle);
	return cur;
}

void Host_SetCursor(HostCursor *cursor) {
	SDL_SetCursor(cursor->c);
}

#define SPRITES_COUNT 128

static HostSprite _sprites[SPRITES_COUNT];
static int _spritesCount;

int Host_SpriteNew() {
	const int num = _spritesCount;
	assert(num < SPRITES_COUNT);
	++_spritesCount;
	HostSprite *spr = &_sprites[num];
	memset(spr, 0, sizeof(HostSprite));
	spr->handle = num;
	spr->rate = 1.;
	return num;
}

void Host_SpriteDelete(int handle) {
	HostSprite *spr = Host_SpriteGet(handle);
	if (spr->image) {
		SDL_FreeSurface(spr->image);
	}
	if (spr->animation_state) {
		free(spr->animation_state);
	}
	if (spr->animation_data) {
		UnloadCan(spr->animation_data);
	}
	memset(spr, 0, sizeof(HostSprite));
}

HostSprite *Host_SpriteGet(int handle) {
	assert(handle >= 0 && handle < SPRITES_COUNT);
	HostSprite *spr = &_sprites[handle];
	return spr;
}

int Host_GetSpriteAnim(int spr_num) {
	HostSprite *spr = Host_SpriteGet(spr_num);
	int current_anim = 0;
	if (spr->animation_state) {
		current_anim = spr->animation_state->current_animation;
	}
	return current_anim;
}

void Host_SetSpriteAnim(int spr_num, int anim) {
	HostSprite *spr = &_sprites[spr_num];
	if (!spr->animation_state) {
		spr->animation_state = (CanAnimationState *)malloc(sizeof(CanAnimationState));
	}
	if (spr->animation_state) {
		Can_Reset(spr->animation_data, spr->animation_state, SDL_GetTicks());
		spr->animation_state->current_animation = anim;
	}
}

void Host_SetSpritePos(int spr_num, int x, int y) {
	HostSprite *spr = &_sprites[spr_num];
	spr->x = x;
	spr->y = y;
}

void Host_SetSpriteImage(int spr_num, SDL_Surface *s) {
	HostSprite *spr = &_sprites[spr_num];
	spr->image = s;
}

void Host_GetSpriteSize(int spr_num, int *w, int *h) {
	HostSprite *spr = &_sprites[spr_num];
	if (spr->image) {
		*w = spr->image->w;
		*h = spr->image->h;
	} else {
		HostSprite *spr = &_sprites[spr_num];
		int current_anim = 0;
		if (spr->animation_state) {
			current_anim = spr->animation_state->current_animation;
		}
		int x1, y1, x2, y2;
		GetAnimationBounds(spr->animation_data, current_anim, &x1, &y1, &x2, &y2);
		if (w) {
			*w = x2 - x1 + 1;
		}
		if (h) {
			*h = y2 - y1 + 1;
		}
	}
}

void Host_SetWindowBackground(SDL_Surface *s) {
	if (g_background) {
		SDL_FreeSurface(g_background);
	}
	g_background = SDL_DuplicateSurface(s);
}

uint32_t Host_GetTimer() {
	if (0) {
		return SDL_GetPerformanceCounter() * 1000 / SDL_GetPerformanceFrequency();
	}
	return SDL_GetTicks();
}

static int _prevButtons, _currentButtons;

int Host_GetLeftClick() {
	return (_prevButtons & SDL_BUTTON_LEFT) != 0 && (_currentButtons & SDL_BUTTON_LEFT) == 0;
}

int Host_GetRightClick() {
	return (_prevButtons & SDL_BUTTON_RIGHT) != 0 && (_currentButtons & SDL_BUTTON_RIGHT) == 0;
}

static int _key;

int Host_GetLastKey() {
	return _key;
}

void Host_ResetKey() {
	_key = 0;
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
		HostSprite *spr = &_sprites[i];
		if (spr->animation_state) {
			Can_Update(spr->animation_data, spr->animation_state, SDL_GetTicks(), spr->rate);
		}
	}
}

static int compareSpriteOrder(const void *a, const void *b) {
	const HostSprite *spr1 = (const HostSprite *)a;
	const HostSprite *spr = (const HostSprite *)b;
	return spr1->order - spr->order;
}

static void draw() {
	SDL_Surface *screen = SDL_GetWindowSurface(g_window);
	SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
	if (g_background) {
		SDL_BlitSurface(g_background, 0, screen, 0);
	}
	HostSprite sprites[SPRITES_COUNT];
	memcpy(sprites, _sprites, _spritesCount * sizeof(HostSprite));
	qsort(sprites, _spritesCount, sizeof(HostSprite), compareSpriteOrder);
	for (int i = 0; i < _spritesCount; ++i) {
		HostSprite *spr = &sprites[i];
		if (!spr->hidden) {
			SDL_Surface *s = spr->image;
			if (s) {
				SDL_Rect dst;
				dst.x = spr->x;
				dst.y = spr->y;
				dst.w = s->w;
				dst.h = s->h;
				SDL_BlitSurface(s, 0, screen, &dst);
			} else if (spr->animation_data) {
				int current_anim = 0;
				int current_frame = 0;
				if (spr->animation_state) {
					current_anim = spr->animation_state->current_animation;
					current_frame = spr->animation_state->current_frame;
				}
				Can_Draw(spr->animation_data, current_anim, current_frame, screen, spr->x, spr->y, 0 /* flags */);
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
			case SDL_KEYDOWN:
				_key = ev.key.keysym.sym;
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
