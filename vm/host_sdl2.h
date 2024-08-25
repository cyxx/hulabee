
#ifndef HOST_SDL2_H__
#define HOST_SDL2_H__

#include <SDL.h>
#include "intern.h"

extern SDL_Window *g_window;
extern SDL_Surface *g_background;

typedef struct host_image_t {
	uint32_t handle;
	SDL_Surface *s;
} HostImage;

int Host_ImageNew();
void Host_ImageCreate(int img, int w, int h, int depth);
HostImage *Host_ImageGet(int handle);

typedef struct host_cursor_t {
	uint32_t handle;
	SDL_Cursor *c;
} HostCursor;

int Host_CursorNew();
void Host_CursorCreate(int num, HostImage *);
HostCursor *Host_CursorGet(int handle);
void Host_SetCursor(HostCursor *cursor);

struct can_animation_data_t;
struct can_animation_state_t;

typedef struct host_sprite_t {
	uint32_t handle;
	int x, y;
	int order;
	int hidden;
	int asset;
	struct can_animation_data_t *animation_data;
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

void Host_SetWindowBackground(SDL_Surface *s);

int Host_GetLeftClick();
int Host_GetRightClick();

int Host_GetLastKey();
void Host_ResetKey();

void Host_Init(const char *window_name, int window_w, int window_h);
void Host_Fini();

typedef void (*UpdateProc)(void *);
void Host_MainLoop(int interval, void (*update)(void *), void *userdata);

#endif /* HOST_SDL2_H__ */
