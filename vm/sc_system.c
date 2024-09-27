
#include "host_sdl2.h"
#include "util.h"
#include "vm.h"

static void fn_system_frame_rate(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_system_frame_rate");
}

static void fn_system_timer(VMContext *c) {
	VM_Push(c, Host_GetTimer(), VAR_TYPE_INT32);
}

static void fn_system_gc(VMContext *c) {
	const int counter = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "System:gc counter:%d", counter);
	if (counter == -1) { /* NOW */
		VM_GC(1);
	} else {
		c->gc_counter = counter;
	}
}

static void fn_system_query(VMContext *c) {
	const int what = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "System:query what:%d", what);
	switch (what) {
	case 1: /* environment */
		VM_Push(c, 1, VAR_TYPE_INT32);
		break;
	case 2: /* OS */
		// 1: MAC
		// 2: WINDOWS
		VM_Push(c, 0, VAR_TYPE_INT32);
		break;
	case 3: { /* Physical RAM (bytes) */
			int memory_mb = SDL_GetSystemRAM();
			if (memory_mb > 1024) {
				memory_mb = 1024;
			}
			VM_Push(c, memory_mb << 20, VAR_TYPE_INT32);
		}
		break;
	case 4: /* Processor */
		// 1: I486
		// 2: PENTIUM
		// 3: PENTIUM2
		// 3: PENTIUM3
		// 101: M68000
		// 102: POWERPC
		VM_Push(c, 3, VAR_TYPE_INT32);
		break;
	case 5: /* Processor Speed (MHz) */
		{
			/* use perf counter as an approximation */
			const uint64_t f = SDL_GetPerformanceFrequency();
			const uint32_t f_mhz = f / (1000 * 1000);
			VM_Push(c, f_mhz, VAR_TYPE_INT32);
		}
		break;
	default:
		warning("Unhandled system:query %d", what);
		VM_Push(c, 0, VAR_TYPE_INT32);
	}
}

static void fn_system_error(VMContext *c) {
	const char *msg = VM_PopString(c);
	fprintf(stderr, "ERROR: %s\n", msg);
}

static void fn_system_warning(VMContext *c) {
	const char *msg = VM_PopString(c);
	warning("%s", msg);
}

static void fn_system_message_box2(VMContext *c) {
	const int flags = VM_PopInt32(c);
	const char *message = VM_PopString(c);
	const char *title = VM_PopString(c);
	debug(DBG_SYSCALLS, "System:messageBox2 title:'%s' message:'%s' flags:0x%x", title, message, flags);
	SDL_ShowSimpleMessageBox(0 /* flags */, title, message, g_window);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_system_set_ini_string(VMContext *c) {
	const char *val = VM_PopString(c);
	const char *key = VM_PopString(c);
	const char *section = VM_PopString(c);
	const char *filename = VM_PopString(c);
	char path[1024];
	if (filename) {
		VM_ReplaceVar(c, filename, path, sizeof(path));
	} else {
		path[0] = 0;
	}
	warning("Unimplemented System:setINI %s (%s) %s %s %s", path, filename, section, key, val);
}

static void fn_system_get_ini_string(VMContext *c) {
	const char *val = VM_PopString(c);
	const char *key = VM_PopString(c);
	const char *section = VM_PopString(c);
	const char *filename = VM_PopString(c);
	char path[1024];
	if (filename) {
		VM_ReplaceVar(c, filename, path, sizeof(path));
	} else {
		path[0] = 0;
	}
	warning("Unimplemented System:getINI %s (%s) %s %s %s", path, filename, section, key, val);
	if (c->gameID == GID_MOOP || c->gameID == GID_OLLO) {
		if (strcmp(section, "Debug") == 0 && strcmp(key, "EnableCheatKeys") == 0) {
			val = "100801";
		}
	}
	VM_PushString(c, val ? val : "");
}

static void fn_system_spawn(VMContext *c) {
	const char *s = VM_PopString(c);
	warning("Unimplemented System:spawn '%s'", s);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static const struct {
	const char *code;
	int language;
} _LANGUAGES[] = {
	{ "en", 100000 },
	{ "fr", 200000 },
	{ "de", 300000 },
	{ "sp", 400000 },
	{ "it", 600000 },
	{ "nl", 700000 },
	{ "jp", 800000 },
	{ "ko", 900000 },
	{ 0, 0 }
};

static void fn_system_get_language(VMContext *c) {
	const int defaultLanguage = VM_PopInt32(c);
	int language = defaultLanguage;
	SDL_Locale *locales = SDL_GetPreferredLocales();
	if (locales) {
		/* lookup the first locale */
		if (locales->language) {
			for (int i = 0; _LANGUAGES[i].code; ++i) {
				if (strncmp(_LANGUAGES[i].code, locales->language, strlen(_LANGUAGES[i].code)) == 0) {
					language = _LANGUAGES[i].language;
					break;
				}
			}
		}
		SDL_free(locales);
	}
	if (language == 0) {
		language = 100000; /* English */
	}
	debug(DBG_SYSCALLS, "System:getLanguage %d default:%d", language, defaultLanguage);
	VM_Push(c, language, VAR_TYPE_INT32);
}

static void fn_system_copy_protection(VMContext *c) {
	warning("Unimplemented System:copyProtection");
}

static void fn_system_property(VMContext *c) {
	VM_Pop2(c);
	VM_PopInt32(c);
	warning("Unimplemented System:property");
}

const VMSyscall _syscalls_system[] = {
	{ 60001, fn_system_frame_rate },
	{ 60002, fn_system_timer },
	{ 60003, fn_system_gc },
	{ 60004, fn_system_query },
	{ 60010, fn_system_error },
	{ 60011, fn_system_warning },
	{ 60016, fn_system_message_box2 },
	{ 60017, fn_system_set_ini_string },
	{ 60018, fn_system_get_ini_string },
	{ 60019, fn_system_spawn },
	{ 60025, fn_system_get_language },
	{ 60047, fn_system_copy_protection },
	{ 60048, fn_system_property },
	{ -1, 0 }
};
