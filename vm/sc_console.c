
#include "util.h"
#include "vm.h"

static void fn_console_get_char(VMContext *c) {
	// warning("Unimplemented Console:getChar()");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_console_char_ready(VMContext *c) {
	// warning("Unimplemented Console:charReady()");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_console[] = {
	{ 10001, fn_console_get_char },
	{ 10004, fn_console_char_ready },
	{ -1, 0 }
};
