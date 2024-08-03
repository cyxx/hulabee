
#include "host_sdl2.h"
#include "util.h"
#include "vm.h"

static void fn_system_frame_rate(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_system_frame_rate");
}

static void fn_system_timer(VMContext *c) {
	VM_Push(c, SDL_GetTicks(), VAR_TYPE_INT32);
}

static void fn_system_gc(VMContext *c) {
	const int counter = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "System:gc counter:%d", counter);
	if (counter == -1) {
		VM_GC(1);
	} else {
		c->gc_counter = counter;
	}
}

static void fn_system_query(VMContext *c) {
	int what = VM_PopInt32(c);
	switch (what) {
	case 1: /* environment */
		VM_Push(c, 1, VAR_TYPE_INT32);
		break;
	case 2: /* OS */
		VM_Push(c, 0, VAR_TYPE_INT32); /* 2 for Windows */
		break;
	case 3: /* Physical RAM */
	case 4: /* Processor */
	case 5: /* Processor Speed */
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
	warning("Unimplemented System:setINI %s %s %s %s", filename, section, key, val);
}

static void fn_system_get_ini_string(VMContext *c) {
	const char *val = VM_PopString(c);
	const char *key = VM_PopString(c);
	const char *section = VM_PopString(c);
	const char *filename = VM_PopString(c);
	warning("Unimplemented System:getINI %s %s %s %s", filename, section, key, val);
	VM_PushString(c, val ? val : "");
}

static void fn_system_spawn(VMContext *c) {
	const char *s = VM_PopString(c);
	warning("Unimplemented System:spawn '%s'", s);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_system_get_language(VMContext *c) {
	const int defaultLanguage = VM_PopInt32(c);
	warning("Unimplemented System:getLanguage");
	int language = defaultLanguage;
	if (language == 0) {
		language = 100000; /* English */
	}
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
