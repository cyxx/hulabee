
#include <dirent.h>
#include <getopt.h>
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
} GameVersion;

static const GameVersion _gameVersions[] = {
	{ "autorun",         787550, GID_AUTORUN_AOL_MOOP_SONNY },
	{ "sonnyrace",     19492068, GID_SONNY,                 },
	{ "mooptreasure",         0, GID_MOOP,                  },
	{ "ollofair",             0, GID_OLLO,                  },
	{ "monsters1",            0, GID_MONSTERS,              },
	{ "piglet1",              0, GID_PIGLET,                },
	{ "realmahjong",    4769563, GID_MAHJONG,               },
	{ "flipoutjr",      4682806, GID_FLIPOUT                },
	{ "casper2",       10725524, GID_CASPER                 },
	{ "bubbleblast",   21444457, GID_BUBBLEBLAST            },
	{ "stitch2",        9583009, GID_STITCH                 },
	{ "grubalicious2", 13907010, GID_GRUBALICIOUS           },
	{ "fourhouses",    16663362, GID_FOURHOUSES             },
	{ "wordspiral",    17873745, GID_WORDSPIRAL,            },
	{ "realmsofgold",  17540639, GID_REALMSGOLD,            },
	{ 0, 0, -1 },
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
		int num, gg = 0;
		if (strcmp(sep + 1, "000000.gg") == 0) {
			gg = 1;
			num = 0;
		} else if (sscanf(sep + 1, "%06d.pan", &num) != 1) {
			continue;
		}
		debug(DBG_PAN, "Found pan id:%06d gg:%d", num, gg);
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
		if (gg) {
			Gg_Open(dataPath, gameName);
		} else {
			Pan_Open(dataPath, gameName, num);
		}
		if (num == 0) {
			for (int i = 0; _gameVersions[i].name; ++i) {
				if (strcasecmp(gameName, _gameVersions[i].name) != 0) {
					continue;
				}
				if (_gameVersions[i].pan000000_size == 0) {
					gameVersion = &_gameVersions[i];
					break;
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
static char *_bootClass = 0;

static void HandleGameIni(const char *section, const char *key, const char *value) {
	// fprintf(stdout, "INI section:%s %s=%s\n", section, key, value);
	if (strcmp(section, "Video") == 0) {
		if (strcmp(key, "DisplayWidth") == 0) {
			_windowW = atoi(value);
		} else if (strcmp(key, "DisplayHeight") == 0) {
			_windowH = atoi(value);
		}
	} else if (strcmp(section, "General") == 0) {
		if (strcmp(key, "BootClass") == 0) {
			_bootClass = strdup(value);
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
	g_debugMask = DBG_INFO | DBG_OPCODES | DBG_PAN | DBG_STACK | DBG_VM | DBG_SOB | DBG_INI | DBG_IMG | DBG_CAN | DBG_SYSCALLS;
	char *dataPath = 0;
	if (argc == 2) {
		// data path as the only command line argument
		struct stat st;
		if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
			dataPath = strdup(argv[1]);
		}
	} else {
		while (1) {
			static struct option options[] = {
				{ "datapath",   required_argument, 0, 1 },
				{ "debug",      required_argument, 0, 2 },
				{ 0, 0, 0, 0 },
			};
			int index;
			const int c = getopt_long(argc, argv, "", options, &index);
			if (c == -1) {
				break;
			}
			switch (c) {
			case 1:
				dataPath = strdup(optarg);
				break;
			case 2:
				g_debugMask = DBG_INFO | atoi(optarg);
				break;
                        }
		}
	}
	if (!dataPath) {
		return -1;
	}
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
			VM_RunMainBoot(c, _bootClass ? _bootClass : gameName, "");
			Host_MainLoop(50, (UpdateProc)VM_RunThreads, c);
			Host_Fini();
			VM_FreeContext(c);
			SDL_Quit();
		}
	}
	return 0;
}
