
#include <sys/param.h>
#include <sys/stat.h>
#include "pan.h"
#include "util.h"

static uint16_t rand16(uint16_t r) {
	return r * 0x6255 + 0x3619;
}

static uint8_t _shuffle[256];

void Pan_InitShuffleTable(const char *name) {
	uint8_t t[256];
	for (int i = 0; i < 256; ++i) {
		t[i] = i;
	}
	uint16_t hash = 0;
	for (int i = 0; name[i]; ++i) {
		uint8_t chr = name[i];
		if (chr >= 'A' && chr <= 'Z') {
			chr += 'a' - 'A';
		}
		hash += (chr << (i & 15)) + chr;
	}
	uint16_t r = rand16(hash);
	const int count = ((r * 10 + 0x8000) >> 16) + 10;
	for (int i = 0; i < count; ++i) {
		for (int j = 0; j < 256; ++j) {
			r = rand16(r);
			const int k = ((r << 8) - r + 0x8000) >> 16;
			assert(k >= 0 && k < 256);
			const uint8_t tmp = t[j];
			t[j] = t[k];
			t[k] = tmp;
		}
	}
	memset(_shuffle, 0, sizeof(_shuffle));
	for (int i = 0; i < 256; ++i) {
		_shuffle[t[i]] = i;
	}
}

#define PAN_FILES_COUNT 64

static PanAsset *_assets;
static int _assetsCount;
static FILE *_files[PAN_FILES_COUNT];
static int _filesCount;
static int _assetsHeapSize;

static int comparePanAsset(const void *a, const void *b) {
	const PanAsset *pa = (const PanAsset *)a;
	const PanAsset *pb = (const PanAsset *)b;
	return pa->id - pb->id;
}

int Pan_Open(const char *dataPath, const char *gameName, int num) {
	char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "%s/%s-%06d.pan", dataPath, gameName, num);
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		warning("Unable to open '%s'", path);
		return -1;
	}
	const uint32_t signature = fileRead32LE(fp);
	if (signature != 0x4150414E) { /* APAN */
		warning("Unexpected pan file signature:0x%x", signature);
		return -1;
	}
	const uint32_t size = fileRead32LE(fp);
	const uint32_t count = fileRead32LE(fp);
	const uint32_t version = fileRead32LE(fp);
	debug(DBG_PAN, "Open pan file '%s' size:%d count:%d version:%d", path, size, count, version);
	if (version == 3) {
	} else if (version == 5) {
		/* const uint32_t flags = */ fileRead32LE(fp);
	} else {
		warning("Unsupported pan file version:%d", version);
		return -1;
	}
	fseek(fp, 512, SEEK_CUR); /* RSA signature */
	fseek(fp, 512, SEEK_CUR); /* RSA public key */
	_assets = (PanAsset *)realloc(_assets, sizeof(PanAsset) * (count + _assetsCount));
	if (!_assets) {
		error("Failed to allocate _assets");
	} else {
		for (size_t i = 0; i < count; ++i) {
			PanAsset *asset = &_assets[_assetsCount + i];
			asset->id = fileRead32LE(fp);
			asset->type = fileRead32LE(fp);
			asset->offset = fileRead32LE(fp);
			asset->size = fileRead32LE(fp);
			fseek(fp, 16, SEEK_CUR); /* MD5 hash */
			if (asset->id == 1) {
				debug(DBG_PAN, "Asset ini size:%d id:%d", asset->size, asset->id);
				if (asset->type != PAN_ASSET_TYPE_INI) {
					warning("Unexpected asset type:%d for ID:1", asset->type);
				}
			}
			asset->name = 0;
			asset->f = _filesCount;
		}
		/* read filenames */
		for (size_t i = 0; i < count; ++i) {
			PanAsset *asset = &_assets[_assetsCount + i];
			char name[32];
			fseek(fp, asset->offset, SEEK_SET);
			int j = 0;
			while (1) {
				const char c = fgetc(fp);
				assert(j < 32);
				name[j++] = c;
				if (!c) {
					break;
				}
			}
			if (asset->type == PAN_ASSET_TYPE_SOB) {
				debug(DBG_PAN, "Asset bytecode '%s' id:%d", name, asset->id);
			}
			if (name[0]) {
				asset->name = strdup(name);
			}
			asset->offset += j;
		}
		_assetsCount += count;
		qsort(_assets, _assetsCount, sizeof(PanAsset), comparePanAsset);
	}
	assert(_filesCount < PAN_FILES_COUNT);
	_files[_filesCount++] = fp;
	return _filesCount;
}

static const uint32_t GG_TAG1 = 0x74648225;
static const uint32_t GG_TAG2 = 0x83547502;
static const uint32_t GG_HEADER_SIZE = 68;

int Gg_Open(const char *dataPath, const char *gameName) {
	assert(_filesCount == 0);
	char path[MAXPATHLEN];
	snprintf(path, sizeof(path), "%s/%s-000000.gg", dataPath, gameName);
	struct stat st;
	if (stat(path, &st) != 0) {
		warning("Failed to stat '%s'", path);
		return 1;
	}
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		warning("Unable to open '%s'", path);
		return -1;
	}
	int count = 0;
	while (ftell(fp) < st.st_size) {
		const uint32_t tag1 = fileRead32LE(fp);
		const uint32_t tag2 = fileRead32LE(fp);
		if (tag1 != GG_TAG1 || tag2 != GG_TAG2) {
			warning("Unhandled GG tags 0x%x 0x%x", tag1, tag2);
			break;
		}
		const uint32_t size = fileRead32LE(fp);
		++count;
		fseek(fp, size, SEEK_CUR);
	}
	debug(DBG_PAN, "Found %d assets in '%s'", count, path);
	_assets = (PanAsset *)realloc(_assets, sizeof(PanAsset) * (count + _assetsCount));
	if (!_assets) {
		error("Failed to allocate _assets");
	} else {
		int rd;
		fseek(fp, 0, SEEK_SET);
		for (PanAsset *asset = _assets; ftell(fp) < st.st_size; ++asset) {
			const uint32_t tag1 = fileRead32LE(fp);
			const uint32_t tag2 = fileRead32LE(fp);
			if (tag1 != GG_TAG1 || tag2 != GG_TAG2) {
				warning("Unhandled GG tags 0x%x 0x%x", tag1, tag2);
				break;
			}
			uint8_t header[GG_HEADER_SIZE];
			rd = fread(header, 1, GG_HEADER_SIZE, fp);
			if (rd != GG_HEADER_SIZE) {
				error("Failed to read %d bytes, ret %d", GG_HEADER_SIZE, rd);
			}
			const uint32_t size = READ_LE_UINT32(header);
			asset->id = READ_LE_UINT32(header + 4);
			asset->type = READ_LE_UINT32(header + 8);
			asset->size = READ_LE_UINT32(header + 12);

			char name[64];
			const int namelen = 4 + size - asset->size - GG_HEADER_SIZE;
			assert(namelen < 64);
			rd = fread(name, 1, namelen, fp);
			if (rd != namelen) {
				error("Failed to read %d bytes, ret %d", namelen, rd);
			}
			for (int i = 0; name[i]; ++i) {
				name[i] ^= 0x87;
			}
			asset->name = strdup(name);
			debug(DBG_PAN, "Asset id:%d type:%d name:'%s'", asset->id, asset->type, name);

			asset->offset = ftell(fp);
			fseek(fp, asset->size, SEEK_CUR);
			asset->f = _filesCount;
		}
		_assetsCount += count;
		qsort(_assets, _assetsCount, sizeof(PanAsset), comparePanAsset);
	}
	assert(_filesCount < PAN_FILES_COUNT);
	_files[_filesCount++] = fp;
	return _filesCount;
}

static int comparePanAssetById(const void *a, const void *b) {
	const uint32_t id = *(const uint32_t *)a;
	const PanAsset *asset = (const PanAsset *)b;
	return id - asset->id;
}

int Pan_HasAsset(uint32_t id) {
	const PanAsset *asset = (const PanAsset *)bsearch(&id, _assets, _assetsCount, sizeof(PanAsset), comparePanAssetById);
	return asset != 0;
}

int Pan_GetAssetType(uint32_t id) {
	const PanAsset *asset = (const PanAsset *)bsearch(&id, _assets, _assetsCount, sizeof(PanAsset), comparePanAssetById);
	if (asset) {
		return asset->type;
	}
	return -1;
}

static int load(const PanAsset *asset, PanBuffer *pb) {
	FILE *fp = _files[asset->f];
	fseek(fp, asset->offset, SEEK_SET);
	const int size = asset->size;
	pb->buffer = (uint8_t *)malloc(size);
	if (!pb->buffer) {
		error("Failed to allocate %d bytes", size);
	} else {
		uint8_t *buffer = pb->buffer;
		const int count = fread(buffer, 1, size, fp);
		if (count != asset->size) {
			error("Failed to read %d bytes, ret %d", size, count);
		}
		for (int i = 0; i < count; ++i) {
			buffer[i] = _shuffle[buffer[i]];
		}
		pb->size = size;
		_assetsHeapSize += size;
		debug(DBG_PAN, "Loaded assets HeapSize:%d", _assetsHeapSize);
	}
	return size;
}

int Pan_LoadAssetById(uint32_t id, PanBuffer *buffer) {
	debug(DBG_PAN, "LoadAssetById %d", id);
	const PanAsset *asset = (const PanAsset *)bsearch(&id, _assets, _assetsCount, sizeof(PanAsset), comparePanAssetById);
	if (asset) {
		assert(asset->id == id);
		return load(asset, buffer);
	}
	warning("Asset %d not found", id);
	return 0;
}

int Pan_LoadAssetByName(const char *name, PanBuffer *buffer) {
	debug(DBG_PAN, "LoadAssetByName '%s'", name);
	for (int i = 0; i < _assetsCount; ++i) {
		PanAsset *asset = &_assets[i];
		if (asset->name && strcasecmp(asset->name, name) == 0) {
			return load(asset, buffer);
		}
	}
	warning("Asset '%s' not found", name);
	return 0;
}

void Pan_UnloadAsset(PanBuffer *buffer) {
	free(buffer->buffer);
	_assetsHeapSize -= buffer->size;
	memset(buffer, 0, sizeof(PanBuffer));
}
