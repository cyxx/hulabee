
#ifndef INI_H__
#define INI_H__

#include "intern.h"

typedef void (*IniProc)(const char *section, const char *name, const char *value);

void LoadIni(const uint8_t *data, int size, IniProc proc);

#endif /* INI_H__ */
