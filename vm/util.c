
#include <stdarg.h>
#include "util.h"

uint32_t g_debugMask;

void debug(uint32_t cm, const char *msg, ...) {
	char buf[1024];
	if (cm & g_debugMask) {
		va_list va;
		va_start(va, msg);
		vsprintf(buf, msg, va);
		va_end(va);
		fprintf(stdout, "%s\n", buf);
		fflush(stdout);
	}
}

void error(const char *msg, ...) {
	char buf[1024];
	va_list va;
	va_start(va, msg);
	vsnprintf(buf, sizeof(buf), msg, va);
	va_end(va);
	fprintf(stderr, "ERROR: %s!\n", buf);
	assert(0);
	exit(-1);
}

void warning(const char *msg, ...) {
	char buf[1024];
	va_list va;
	va_start(va, msg);
	vsnprintf(buf, sizeof(buf), msg, va);
	va_end(va);
	fprintf(stdout, "WARNING: %s!\n", buf);
}

int fileRead(FILE *fp, void *buf, int size) {
	const int count = fread(buf, 1, size, fp);
	if (count != size) {
		error("I/O error on reading %d bytes, ret %d", size, count);
	}
	return count;
}

uint8_t fileReadByte(FILE *fp) {
	uint8_t b;
	fileRead(fp, &b, 1);
	return b;
}

uint16_t fileRead16LE(FILE *fp) {
	uint8_t buf[2];
	fileRead(fp, buf, 2);
	return READ_LE_UINT16(buf);
}

uint32_t fileRead32LE(FILE *fp) {
	uint8_t buf[4];
	fileRead(fp, buf, 4);
	return READ_LE_UINT32(buf);
}
