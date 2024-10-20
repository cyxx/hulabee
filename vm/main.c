
#include <dirent.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "fileio.h"
#include "host_sdl2.h"
#include "ini.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static const int PAN_HEAP_SIZE = 8 * 1024 * 1024;

#define GAMENAME_LEN 32

typedef struct {
	const char *name;
	int gid;
} GameVersion;

static const GameVersion _gameVersions[] = {
	{ "autorun",        GID_AUTORUN_AOL_MOOP_SONNY },
	{ "sonnyrace",      GID_SONNY,                 },
	{ "mooptreasure",   GID_MOOP,                  },
	{ "ollofair",       GID_OLLO,                  },
	{ "monsters1",      GID_MONSTERS,              },
	{ "piglet1",        GID_PIGLET,                },
	{ "realmahjong",    GID_MAHJONG,               },
	{ "flipoutjr",      GID_FLIPOUT                },
	{ "casper2",        GID_CASPER                 },
	{ "bubbleblast",    GID_BUBBLEBLAST            },
	{ "stitch2",        GID_STITCH                 },
	{ "grubalicious2",  GID_GRUBALICIOUS           },
	{ "offchal",        GID_OFFCHAL                },
	{ "treasurearcade", GID_TREASUREARCADE         },
	{ "fourhouses",     GID_FOURHOUSES             },
	{ "wordspiral",     GID_WORDSPIRAL,            },
	{ "realmsofgold",   GID_REALMSGOLD,            },
	{ 0, -1 },
};

typedef struct {
	const char *filename;
	uint32_t size;
	uint32_t pan_offset;
	const GameVersion version;
} Executable;

static const Executable _executables[] = {
	{ "autorun.exe", 599708, 0x4E200, { "autorun", GID_MOOP } },
	{ "autorun.exe", 554328, 0x4E200, { "autorun", GID_OLLO } },
	{ 0, 0, 0, { 0, -1 } },
};

static const Executable *FindExecutable(const char *filePath) {
	struct stat st;
	if (stat(filePath, &st) == 0 && S_ISREG(st.st_mode)) {
		for (int i = 0; _executables[i].filename; ++i) {
			if (st.st_size == _executables[i].size) {
				FILE *fp = fopen(filePath, "rb");
				if (fp) {
					fseek(fp, _executables[i].pan_offset, SEEK_SET);
					const uint32_t apan = fileRead32LE(fp);
					fclose(fp);
					if (apan == 0x4150414E) {
						return &_executables[i];
					}
				}
			}
		}
	}
	return 0;
}

static const GameVersion *FindGame(DIR *d, const char *dataPath, char *gameName) {
	const GameVersion *gameVersion = 0;
	struct dirent *de;
	while ((de = readdir(d)) != 0) {
		if (de->d_name[0] == '.') {
			continue;
		}
		const char *ext = strrchr(de->d_name, '.');
		if (!ext) {
			continue;
		}
		char path[MAXPATHLEN];
		snprintf(path, sizeof(path), "%s/%s", dataPath, de->d_name);
		int gg = 0;
		if (strcmp(ext + 1, "gg") == 0) {
			gg = 1;
		} else if (strcmp(ext + 1, "exe") == 0) {
			const Executable *exe = FindExecutable(path);
			if (exe) {
				Pan_Open(path, exe->pan_offset);
				strcpy(gameName, exe->version.name);
				return &exe->version;
			}
		} else if (strcmp(ext + 1, "pan") != 0) {
			continue;
		}
		const char *sep = strrchr(de->d_name, '-');
		if (!sep) {
			continue;
		}
		int num = 0;
		if (sscanf(sep + 1, "%06d", &num) != 1) {
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
			Gg_Open(path);
		} else {
			Pan_Open(path, 0x0);
		}
	}
	for (int i = 0; _gameVersions[i].name; ++i) {
		if (strcasecmp(gameName, _gameVersions[i].name) == 0) {
			gameVersion = &_gameVersions[i];
			break;
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
		} else if (strcmp(key, "DisableSoundSystem") == 0) {
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
	g_debugMask = DBG_INFO;
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
		const GameVersion *version = FindGame(d, dataPath, gameName);
		closedir(d);
		if (!gameName[0]) {
			warning("No SAUCE game found in '%s'", dataPath);
		} else {
			if (version && version->gid >= GID_MOOP) {
				_windowW = 800;
				_windowH = 600;
			}
			Pan_InitShuffleTable(gameName);
			Pan_InitHeap(PAN_HEAP_SIZE);
			ParseGameIni();
			VMContext *c = VM_NewContext();
			if (version) {
				debug(DBG_INFO, "Found game ID '%s'", version->name);
				VM_SetGameID(c, version->gid);
			}
			c->get_timer = Host_GetTimer;
			VM_InitOpcodes();
			VM_InitSyscalls(c);
			Fio_Init(dataPath, ".");
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
