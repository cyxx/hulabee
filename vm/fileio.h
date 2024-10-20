
#ifndef FILEIO_H__
#define FILEIO_H__

enum {
	FIO_TYPE_FILE,
	FIO_TYPE_DIRECTORY
};

enum {
	FIO_MODE_NEW = 1,
	FIO_MODE_READ,
	FIO_MODE_MODIFY,
};

void Fio_Init(const char *dataPath, const char *savePath);
void Fio_Fini();

int Fio_ResolvePath(const char *path, char *out, int len);

int Fio_Exists(const char *name);
int Fio_Eof(int fh);

int Fio_Open(const char *name, int mode);
void Fio_Close(int fh);
int Fio_Read(int fh, void *buf, int size);
void Fio_Write(int fh, const void *buf, int size);
int Fio_ReadInt(int fh, int size);
void Fio_WriteInt(int fh, int value, int size);

#endif
