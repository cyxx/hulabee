
#ifndef HOST_SDL2_H__
#define HOST_SDL2_H__

#include <SDL.h>
#include "intern.h"

extern SDL_Window *g_window;

typedef struct host_image_t {
	uint32_t handle;
	SDL_Surface *s;
} HostImage;

int Host_CreateImage(SDL_Surface *s);
HostImage *HostImage_Get(int handle);

struct can_animation_state_t;

typedef struct host_sprite_t {
	uint32_t handle;
	int x, y;
	int order;
	int hidden;
	int asset;
	struct can_animation_state_t *animation_state;
	SDL_Surface *image;
} HostSprite;

int HostSprite_New();
void HostSprite_Delete(int handle);
HostSprite *HostSprite_Get(int handle);

int Host_CreateSprite();
int Host_GetSpriteAnim(int spr);
void Host_GetSpriteSize(int spr, int *w, int *h);
void Host_SetSpriteAnim(int spr, int anim);
void Host_SetSpritePos(int spr, int x, int y);
void Host_SetSpriteImage(int spr, SDL_Surface *s);

struct animation_t;
struct animation_t *Host_GetSpriteAnimationData(int spr);

void Host_SetWindowBackground(SDL_Surface *s);

int Host_GetLeftClick();
int Host_GetRightClick();

void Host_Init(const char *window_name, int window_w, int window_h);
void Host_Fini();

typedef void (*UpdateProc)(void *);
void Host_MainLoop(int interval, void (*update)(void *), void *userdata);

#endif /* HOST_SDL2_H__ */
