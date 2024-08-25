
#include "host_sdl2.h"
#include "util.h"
#include "vm.h"

static void fn_console_get_char(VMContext *c) {
	VM_Push(c, Host_GetLastKey(), VAR_TYPE_INT32);
	Host_ResetKey();
}

static void fn_console_char_ready(VMContext *c) {
	VM_Push(c, Host_GetLastKey() != 0, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_console[] = {
	{ 10001, fn_console_get_char },
	{ 10004, fn_console_char_ready },
	{ -1, 0 }
};
