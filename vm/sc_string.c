
#include <time.h>
#include "vm.h"

static void fn_string_compare(VMContext *c) {
	int ret = -1;
	const char *s2 = ArrayHandle_GetString(c, VM_Pop(c, 0x10005));
	const char *s1 = ArrayHandle_GetString(c, VM_Pop(c, 0x10005));
	while (1) {
		if (*s1 != *s2) {
			ret = *s1 - *s2;
			break;
		}
		if (!*s1) {
			ret = 0;
			break;
		}
		++s1;
		++s2;
	}
	VM_Push(c, ret, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_string[] = {
	{ 140015, fn_string_compare },
	{ -1, 0 }
};
