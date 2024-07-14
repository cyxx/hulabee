
#ifndef HOST_SDL2_H__
#define HOST_SDL2_H__

#include <SDL.h>
#include "intern.h"

int CreateTexture(int w, int h, int fmt, void (*decode)(void *, uint32_t *, int), void *userdata);
SDL_Texture *GetTexture(int num);

extern SDL_Window *g_window;

struct bitmap_t;
int Host_CreateImage(struct bitmap_t *);

int Host_CreateSprite();
int Host_GetSpriteAnim(int spr);
void Host_GetSpriteSize(int spr, int *w, int *h);
void Host_SetSpriteAnim(int spr, int anim);
void Host_SetSpritePos(int spr, int x, int y);
void Host_SetSpriteTexture(int spr, int texture);
void Host_GetSpritePos(int spr, int *x, int *y);
void Host_SetSpriteOrder(int spr, int order);
void Host_ShowSprite(int spr, int show);
int Host_IsSpriteHidden(int spr);
void Host_SetSpriteImage(int spr, struct bitmap_t *);

struct animation_t;
struct animation_t *Host_GetSpriteAnimationData(int spr);

void Host_SetWindowBackgroundTexture(int texture);

int Host_GetLeftClick();
int Host_GetRightClick();

void Host_Init(const char *window_name, int window_w, int window_h);
void Host_Fini();

typedef void (*UpdateProc)(void *);
void Host_MainLoop(int interval, void (*update)(void *), void *userdata);

#endif /* HOST_SDL2_H__ */
