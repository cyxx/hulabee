
#include "fileio.h"
#include "util.h"
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#define FILES_COUNT 32

typedef struct {
	uint32_t handle;
	FILE *fp;
	int mode;
} FioHandle;

static FioHandle _files[FILES_COUNT];
static int _filesCount = 1;

static const char *_dataPath;
static const char *_savePath;

static const char *MODES[] = { "", "wb", "rb", "ab" };

void Fio_Init(const char *dataPath, const char *savePath) {
	_dataPath = dataPath;
	_savePath = savePath;
}

void Fio_Fini() {
}

int Fio_ResolvePath(const char *path, char *out, int size) {
	if (path[0] == '$') {
		++path;
		const char *q = strpbrk(path, "/\\");
		const int len = q ? q - path : strlen(path);
		if (strncmp(path, "game", len) == 0 || strncmp(path, "system", len) == 0) {
			snprintf(out, size, "%s/%s", _dataPath, q ? q + 1 : "");
		} else if (strncmp(path, "user", len) == 0) {
			snprintf(out, size, "%s/%s", _savePath, q ? q + 1 : "");
		}
	} else {
		assert(strlen(path) < size);
		strcpy(out, path);
	}
	debug(DBG_FILEIO, "Fio_ResolvePath '%s'", out);
	return strlen(out);
}

int Fio_Exists(const char *name) {
	char path[MAXPATHLEN];
	Fio_ResolvePath(name, path, sizeof(path));
	struct stat st;
	if ((stat(path, &st) == 0) && S_ISREG(st.st_mode)) {
		return 1;
	}
	return 0;
}

static int fio_new_handle() {
	const int num = _filesCount;
	FioHandle *fh = 0;
	if (num < FILES_COUNT) {
		fh = &_files[num];
		++_filesCount;
	} else {
		for (int i = 1; i < FILES_COUNT; ++i) {
			if (_files[i].handle == 0) {
				fh = &_files[i];
				break;
			}
		}
		assert(fh);
	}
	memset(fh, 0, sizeof(FioHandle));
	fh->handle = fh - _files;
	return fh->handle;
}

static FioHandle *fio_get_handle(int handle) {
	assert(handle > 0 && handle < _filesCount);
	assert(_files[handle].fp);
	return &_files[handle];
}

int Fio_Eof(int fh) {
	FioHandle *f = fio_get_handle(fh);
	return feof(f->fp);
}

int Fio_Open(const char *name, int mode) {
	debug(DBG_FILEIO, "Fio_Open '%s' mode:%d", name, mode);
	char path[MAXPATHLEN];
	Fio_ResolvePath(name, path, sizeof(path));
	FILE *fp = fopen(path, MODES[mode]);
	if (fp) {
		const int handle = fio_new_handle();
		_files[handle].fp = fp;
		_files[handle].mode = mode;
		return handle;
	}
	return 0;
}

void Fio_Close(int fh) {
	FioHandle *f = fio_get_handle(fh);
	fclose(f->fp);
	memset(f, 0, sizeof(FioHandle));
}

int Fio_Read(int fh, void *buf, int size) {
	FioHandle *f = fio_get_handle(fh);
	return fread(buf, 1, size, f->fp);
}

void Fio_Write(int fh, const void *buf, int size) {
	FioHandle *f = fio_get_handle(fh);
	fwrite(buf, 1, size, f->fp);
}

int Fio_ReadInt(int fh, int size) {
	FioHandle *f = fio_get_handle(fh);
	if (size == 8) {
		return fgetc(f->fp);
	} else if (size == 16) {
		uint8_t buf[2];
		fread(buf, 1, sizeof(buf), f->fp);
		return READ_LE_UINT16(buf);
	} else if (size == 32) {
		uint8_t buf[4];
		fread(buf, 1, sizeof(buf), f->fp);
		return READ_LE_UINT32(buf);
	} else {
		return 0;
	}
}

void Fio_WriteInt(int fh, int value, int size) {
	FioHandle *f = fio_get_handle(fh);
	if (size == 8) {
		fputc(value, f->fp);
	} else if (size == 16) {
		uint8_t buf[2];
		WRITE_LE_UINT16(buf, value);
		fwrite(buf, 1, sizeof(buf), f->fp);
	} else if (size == 32) {
		uint8_t buf[4];
		WRITE_LE_UINT32(buf, value);
		fwrite(buf, 1, sizeof(buf), f->fp);
	}
}
