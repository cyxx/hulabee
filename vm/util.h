
#ifndef UTIL_H__
#define UTIL_H__

#include "intern.h"

enum {
	DBG_INFO     = 1 << 0,
	DBG_OPCODES  = 1 << 1,
	DBG_PAN      = 1 << 2,
	DBG_STACK    = 1 << 3,
	DBG_VM       = 1 << 4,
	DBG_SOB      = 1 << 5,
	DBG_INI      = 1 << 6,
	DBG_IMG      = 1 << 7,
	DBG_CAN      = 1 << 8,
	DBG_SYSCALLS = 1 << 9,
};

extern uint32_t g_debugMask;

void debug(uint32_t cm, const char *msg, ...);
void error(const char *msg, ...);
void warning(const char *msg, ...);

uint16_t fileRead16LE(FILE *fp);
uint32_t fileRead32LE(FILE *fp);

static inline uint16_t Read16(const uint8_t *buffer, int size, int *pos) {
	if (*pos + sizeof(uint16_t) > size) {
		error("Read16 out of bounds %d (%d)", *pos, size);
	}
	const uint16_t value = READ_LE_UINT16(buffer + *pos);
	*pos += sizeof(uint16_t);
	return value;
}

static inline uint32_t Read32(const uint8_t *buffer, int size, int *pos) {
	if (*pos + sizeof(uint32_t) > size) {
		error("Read32 out of bounds %d (%d)", *pos, size);
	}
	const uint32_t value = READ_LE_UINT32(buffer + *pos);
	*pos += sizeof(uint32_t);
	return value;
}


#ifdef NDEBUG
#define debug(x, ...)
#endif

#endif // UTIL_H__
