
#include "vm.h"

static void fn_debug_print(VMContext *c) {
	const char *s = VM_PopString(c);
	VM_PopInt32(c);
	fprintf(stdout, "DEBUG: %s\n", s);
}

static void fn_debug_println(VMContext *c) {
	fn_debug_print(c);
}

const VMSyscall _syscalls_debug[] = {
	{ 120001, fn_debug_print },
	{ 120002, fn_debug_println },
	{ -1, 0 }
};
