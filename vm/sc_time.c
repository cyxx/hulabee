
#include <time.h>
#include "vm.h"

static void fn_time_get_time(VMContext *c) {
	const int res = time(0);
	VM_Push(c, res, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_time[] = {
	{ 150001, fn_time_get_time },
	{ -1, 0 }
};
