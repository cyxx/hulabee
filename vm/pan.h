
#ifndef PAN_H__
#define PAN_H__

#include "intern.h"

enum {
	PAN_ASSET_TYPE_INI = 0,
	PAN_ASSET_TYPE_IMG = 1,
	PAN_ASSET_TYPE_CAN = 2,
	PAN_ASSET_TYPE_SOB = 3,
	PAN_ASSET_TYPE_WAV = 4,
	PAN_ASSET_TYPE_MP3 = 7,
	PAN_ASSET_TYPE_TXT = 8,
	PAN_ASSET_TYPE_JPG = 9,
};

typedef struct {
	uint32_t id;
	uint32_t type;
	uint32_t offset;
	uint32_t size;
	char *name;
	uint8_t f;
} PanAsset;

typedef struct {
	const uint8_t *buffer;
	int size;
	uint16_t index;
} PanBuffer;

void Pan_InitShuffleTable(const char *name);
void Pan_InitHeap(int size);
int Pan_Open(const char *filePath, uint32_t offset);
int Gg_Open(const char *filePath);
int Pan_HasAsset(uint32_t id);
int Pan_GetAssetType(uint32_t id);
int Pan_LoadAssetById(uint32_t id, PanBuffer *buffer);
int Pan_LoadAssetByName(const char *name, PanBuffer *buffer);
void Pan_UnloadAsset(PanBuffer *buffer);

#endif /* PAN_H__ */
