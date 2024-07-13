
#include "can.h"
#include "img.h"
#include "host_sdl2.h"
#include "util.h"

#define TEXTURES_COUNT 256

static SDL_Renderer *_renderer;
SDL_Window *g_window;

static SDL_Texture *_textures[TEXTURES_COUNT];
static int _texturesCount;

static int _backgroundTexture = -1;

int CreateTexture(int w, int h, void (*decode)(void *, uint32_t *, int), void *userdata) {
	SDL_Texture *texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	void *dst = 0;
	int pitch = 0;
	if (SDL_LockTexture(texture, 0, &dst, &pitch) == 0) {
		assert((pitch & 3) == 0);
		decode(userdata, (uint32_t *)dst, pitch / sizeof(uint32_t));
		SDL_UnlockTexture(texture);
	}
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	const int num = _texturesCount;
	assert(num < TEXTURES_COUNT);
	_textures[_texturesCount++] = texture;
	return num;
}

SDL_Texture *GetTexture(int num) {
	assert(num >= 0 && num < _texturesCount);
	return _textures[num];
}

#define IMAGES_COUNT 32

typedef struct {
	int texture;
} Image;

static Image _images[IMAGES_COUNT];
static int _imagesCount;

int Host_CreateImage(struct bitmap_t *b) {
	const int num = _imagesCount;
	assert(num < IMAGES_COUNT);
	Image *img = &_images[_imagesCount++];
	memset(img, 0, sizeof(Image));
	img->texture = b->texture;
	return num;
}

#define SPRITES_COUNT 64

typedef struct {
	Animation animation_data;
	int current_anim;
	int current_frame;
	int x, y;
	int texture;
	int order;
	int hidden;
} Sprite;

static Sprite _sprites[SPRITES_COUNT];
static int _spritesCount;

int Host_CreateSprite() {
	const int num = _spritesCount;
	assert(num < SPRITES_COUNT);
	Sprite *spr = &_sprites[_spritesCount++];
	memset(spr, 0, sizeof(Sprite));
	spr->texture = -1;
	return num;
}

static Sprite *getSprite(int num) {
	assert(num >= 0 && num < _spritesCount);
	return &_sprites[num];
}

int Host_GetSpriteAnim(int spr_num) {
	Sprite *spr = getSprite(spr_num);
	return spr->current_anim;
}

void Host_SetSpriteAnim(int spr_num, int anim) {
	Sprite *spr = getSprite(spr_num);
	spr->current_anim = anim;
	spr->current_frame = 0;
	int x2, y2;
	GetAnimationFrameBounds(&spr->animation_data, anim, &spr->x, &spr->y, &x2, &y2);
	int x1, y1;
	spr->texture = GetCanBitmap(&spr->animation_data, anim, 0, &x1, &y1);
}

void Host_SetSpritePos(int spr_num, int x, int y) {
	Sprite *spr = getSprite(spr_num);
	spr->x = x;
	spr->y = y;
}

void Host_SetSpriteTexture(int spr_num, int texture) {
	Sprite *spr = getSprite(spr_num);
	spr->texture = texture;
}

void Host_GetSpritePos(int spr_num, int *x, int *y) {
	Sprite *spr = getSprite(spr_num);
	if (x) {
		*x = spr->x;
	}
	if (y) {
		*y = spr->y;
	}
}

void Host_SetSpriteOrder(int spr_num, int order) {
	Sprite *spr = getSprite(spr_num);
	spr->order = order;
}

void Host_ShowSprite(int spr_num, int show) {
	Sprite *spr = getSprite(spr_num);
	spr->hidden = !show;
}

int Host_IsSpriteHidden(int spr_num) {
	Sprite *spr = getSprite(spr_num);
	return spr->hidden;
}

void Host_SetSpriteImage(int spr_num, struct bitmap_t *b) {
	Sprite *spr = getSprite(spr_num);
	spr->texture = b->texture;
}

Animation *Host_GetSpriteAnimationData(int spr_num) {
	Sprite *spr = getSprite(spr_num);
	return &spr->animation_data;
}

void Host_SetWindowBackgroundTexture(int texture) {
	_backgroundTexture = texture;
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
	_renderer = SDL_CreateRenderer(g_window, -1, 0);
}

void Host_Fini() {
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
}

static void draw() {
	SDL_RenderClear(_renderer);
	if (_backgroundTexture != -1) {
		SDL_Texture *background = GetTexture(_backgroundTexture);
		if (background) {
			SDL_RenderCopy(_renderer, background, 0, 0);
		}
	}
	for (int i = 0; i < _spritesCount; ++i) {
		Sprite *spr = &_sprites[i];
		if (!spr->hidden && !(spr->texture < 0)) {
			SDL_Rect dst;
			dst.x = spr->x;
			dst.y = spr->y;
			SDL_Texture *texture = GetTexture(spr->texture);
			SDL_QueryTexture(texture, 0, 0, &dst.w, &dst.h);
			SDL_RenderCopy(_renderer, texture, 0, &dst);
		}
	}
	SDL_RenderPresent(_renderer);
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
		draw();
		SDL_Delay(interval);
	}
}
