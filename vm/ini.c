
#include "ini.h"
#include "util.h"

#define INI_LINE_LEN 256

static inline int whitespace(char c) {
	return strchr(" \t", c) != 0;
}

static void copy(char *dst, const char *src, const char *end, const int line) {
	while (*src && whitespace(*src)) {
		++src;
	}
	--end;
	while (end > src && *end && whitespace(*end)) {
		--end;
	}
	int len = end - src + 1;
	if (len > INI_LINE_LEN) {
		warning("INI line length:%d", len);
		len = INI_LINE_LEN;
	}
	memcpy(dst, src, len);
	dst[len] = 0;
}

void LoadIni(const uint8_t *data, int size, IniProc proc) {
	int offset = 0;
	while (offset < size && data[offset] != '[') {
		uint32_t unk = READ_LE_UINT32(data + offset);
		debug(DBG_INI, "Skipping unknown bytes:0x%08x", unk);
		offset += 4;
	}
	const char *str = (const char *)data + offset;
	const char *end = str + size - offset;
	char buf[INI_LINE_LEN + 1];
	char *section = 0;
	for (int line = 0; str < end; ++line) {
		char *endline = strpbrk(str, "\r\n");
		copy(buf, str, endline ? endline : end, line);
		debug(DBG_INI, "INI line:%d '%s'", line, buf);
		if (!buf[0] || strncmp(buf, "//", 2) == 0 || buf[0] == ';') {
			/* empty or commented line */
		} else {
			if (buf[0] == '[') {
				char *p = strchr(&buf[1], ']');
				if (!p) {
					warning("Malformed INI section line %d", line);
				} else {
					*p = 0;
					free(section);
					section = strdup(&buf[1]);
					debug(DBG_INI, "INI section:'%s'", section);
				}
			} else {
				char *p = strchr(buf, '=');
				if (!p) {
					warning("Malformed INI key value line %d", line);
				} else {
					char *q = p - 1;
					while (q > buf && whitespace(*q)) {
						*q-- = 0;
					}
					*p++ = 0;
					while (*p && whitespace(*p)) {
						++p;
					}
					debug(DBG_INI, "key:'%s' value:'%s'", buf, p);
					if (*p) {
						proc(section, buf, p);
					}
				}
			}
		}
		if (!endline) {
			break;
		}
		str = endline + 1;
	}
}
