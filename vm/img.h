
#ifndef IMG_H__
#define IMG_H__

#include "intern.h"

enum {
	BMP_FMT_PAL256,
	BMP_FMT_RGB555,
	BMP_FMT_JPEG
};

typedef struct bitmap_t {
	int w, h;
	int texture;
} Bitmap;

void LoadImg(const uint8_t *data, int size, Bitmap *bmp);
void LoadJpg(const uint8_t *data, int size, Bitmap *bmp);

#endif /* IMG_H__ */
