
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "host_sdl2.h"
#include "ini.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

#define GAMENAME_LEN 32

typedef struct {
	const char *name;
	uint32_t pan000000_size;
	int gid;
	int language;
} GameVersion;

static const GameVersion _gameVersions[] = {
	{ "autorun",        787550, GID_AUTORUN_AOL_MOOP_SONNY, LANGUAGE_EN_US },
	{ "sonnyrace",    19492068, GID_SONNY,                  LANGUAGE_EN_US },
	{ "mooptreasure", 17634930, GID_MOOP,                   LANGUAGE_EN_US },
	{ "ollofair",     21325685, GID_OLLO,                   LANGUAGE_EN_US },
	{ 0, 0, -1, -1 },
};

static const GameVersion *LoadGame(DIR *d, const char *dataPath, char *gameName) {
	const GameVersion *gameVersion = 0;
	struct dirent *de;
	while ((de = readdir(d)) != 0) {
		if (de->d_name[0] == '.') {
			continue;
		}
		const char *sep = strchr(de->d_name, '-');
		if (!sep) {
			continue;
		}
		int num;
		if (sscanf(sep + 1, "%06d.pan", &num) != 1) {
			continue;
		}
		debug(DBG_PAN, "Found pan id:%06d", num);
		char buffer[GAMENAME_LEN];
		const size_t len = sep - de->d_name;
		assert(len < sizeof(buffer));
		memcpy(buffer, de->d_name, len);
		buffer[len] = 0;
		if (!gameName[0]) {
			strcpy(gameName, buffer);
			debug(DBG_PAN, "Found game %s", gameName);
		} else if (strcmp(gameName, buffer) != 0) {
			continue;
		}
		Pan_Open(dataPath, gameName, num);
		if (num == 0) {
			for (int i = 0; _gameVersions[i].name; ++i) {
				if (strcasecmp(gameName, _gameVersions[i].name) != 0) {
					continue;
				}
				char path[MAXPATHLEN];
				snprintf(path, sizeof(path), "%s/%s", dataPath, de->d_name);
				struct stat st;
				if (stat(path, &st) == 0 && st.st_size == _gameVersions[i].pan000000_size) {
					gameVersion = &_gameVersions[i];
					break;
				}
			}
		}
	}
	return gameVersion;
}

static int _windowW = 640;
static int _windowH = 480;

static void HandleGameIni(const char *section, const char *key, const char *value) {
	// fprintf(stdout, "INI section:%s %s=%s\n", section, key, value);
	if (strcmp(section, "Video") == 0) {
		if (strcmp(key, "DisplayWidth") == 0) {
			_windowW = atoi(value);
		} else if (strcmp(key, "DisplayHeight") == 0) {
			_windowH = atoi(value);
		}
	}
}

static void ParseGameIni() {
	PanBuffer pb;
	if (Pan_LoadAssetById(1, &pb)) {
		LoadIni(pb.buffer, pb.size, HandleGameIni);
		Pan_UnloadAsset(&pb);
	}
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		g_debugMask = DBG_INFO | DBG_OPCODES | DBG_PAN | DBG_STACK | DBG_VM | DBG_SOB | DBG_INI | DBG_IMG | DBG_CAN | DBG_SYSCALLS;
		const char *dataPath = argv[1];
		DIR *d = opendir(dataPath);
		if (!d) {
			warning("Unable to open '%s'", dataPath);
		} else {
			char gameName[GAMENAME_LEN + 1];
			gameName[0] = 0;
			const GameVersion *version = LoadGame(d, dataPath, gameName);
			closedir(d);
			if (!gameName[0]) {
				warning("No SAUCE game found in '%s'", dataPath);
			} else {
				Pan_InitShuffleTable(gameName);
				ParseGameIni();
				VMContext *c = VM_NewContext();
				if (version) {
					debug(DBG_INFO, "Found game ID '%s'", version->name);
					VM_SetGameID(c, version->gid);
				}
				VM_InitOpcodes();
				VM_InitSyscalls(c);
				Host_Init(version ? version->name : "", _windowW, _windowH);
				VM_RunMainBoot(c, gameName, "");
				Host_MainLoop(50, (UpdateProc)VM_RunThreads, c);
				Host_Fini();
				VM_FreeContext(c);
				SDL_Quit();
			}
		}
	}
	return 0;
}
