
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
	VM_PopInt32(c);
	warning("Unimplemented fn_system_gc");
}

static void fn_system_query(VMContext *c) {
	int what = VM_PopInt32(c);
	warning("Unimplemented fn_system_query");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_system_error(VMContext *c) {
	const char *msg = VM_PopString(c);
	warning("Unimplemented fn_system_error '%s'", msg);
}

static void fn_system_warning(VMContext *c) {
	const char *msg = VM_PopString(c);
	warning("%s", msg);
}

static void fn_system_message_box2(VMContext *c) {
	int flags = VM_PopInt32(c);
	const char *message = VM_PopString(c);
	const char *title = VM_PopString(c);
	SDL_ShowSimpleMessageBox(0 /* flags */, title, message, g_window);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_system_set_ini_string(VMContext *c) {
	const char *val = VM_PopString(c);
	const char *key = VM_PopString(c);
	const char *section = VM_PopString(c);
	const char *filename = VM_PopString(c);
	warning("Unimplemented fn_system_set_ini_string %s %s %s %s", filename, section, key, val);
}

static void fn_system_get_ini_string(VMContext *c) {
	const char *val = VM_PopString(c);
	const char *key = VM_PopString(c);
	const char *section = VM_PopString(c);
	const char *filename = VM_PopString(c);
	warning("Unimplemented fn_system_get_ini_string %s %s %s %s", filename, section, key, val);
	VM_PushString(c, "");
}

static void fn_system_spawn(VMContext *c) {
	const char *s = VM_PopString(c);
	warning("Unimplemented System:spawn '%s'", s);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_system_get_language(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented System:getLanguage");
	VM_Push(c, 100000, VAR_TYPE_INT32);
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
	{ -1, 0 }
};
