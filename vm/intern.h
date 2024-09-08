
#ifndef INTERN_H__
#define INTERN_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

static inline uint16_t READ_LE_UINT16(const void *ptr) {
	if (1 && (((uintptr_t)ptr) & 1) != 0) {
		uint16_t value;
		memcpy(&value, ptr, sizeof(uint16_t));
		return le16toh(value);
	} else {
		return le16toh(*(const uint16_t *)ptr);
	}
}

static inline uint32_t READ_LE_UINT32(const void *ptr) {
	if (1 && (((uintptr_t)ptr) & 3) != 0) {
		uint32_t value;
		memcpy(&value, ptr, sizeof(uint32_t));
		return le32toh(value);
	} else {
		return le32toh(*(const uint32_t *)ptr);
	}
}

static inline void WRITE_LE_UINT16(void *ptr, uint16_t v) {
	if (1 && (((uintptr_t)ptr) & 1) != 0) {
		const uint16_t value = htole16(v);
		memcpy(ptr, &value, sizeof(uint16_t));
	} else {
		*((uint16_t *)ptr) = htole16(v);
	}
}

static inline void WRITE_LE_UINT32(void *ptr, uint32_t v) {
	if (1 && (((uintptr_t)ptr) & 3) != 0) {
		const uint32_t value = htole32(v);
		memcpy(ptr, &value, sizeof(uint32_t));
	} else {
		*((uint32_t *)ptr) = htole32(v);
	}
}

enum GameID {
	GID_AUTORUN_AOL_MOOP_SONNY,
	GID_SONNY,
	GID_MOOP,
	GID_OLLO,
	GID_MONSTERS,
	GID_PIGLET,
	GID_MAHJONG,
	GID_FLIPOUT,
	GID_CASPER,
	GID_BUBBLEBLAST,
	GID_STITCH,
	GID_GRUBALICIOUS,
	GID_OFFCHAL,
	GID_TREASUREARCADE,
	GID_FOURHOUSES,
	GID_WORDSPIRAL,
	GID_REALMSGOLD,
};

enum Language {
	LANGUAGE_EN_US,
};

#endif // INTERN_H_
