
#ifndef IMG_H__
#define IMG_H__

#include "intern.h"

enum {
	BMP_FMT_PAL256,
	BMP_FMT_RGB555,
	BMP_FMT_JPEG
};

struct SDL_Surface;

struct SDL_Surface *LoadImg(const uint8_t *data, int size);
struct SDL_Surface *LoadJpg(const uint8_t *data, int size);

#endif /* IMG_H__ */
