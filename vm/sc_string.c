
#include <time.h>
#include "vm.h"

static void fn_string_compare(VMContext *c) {
	const int array2 = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const int array1 = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const int ret = ArrayHandle_CompareString(c, array1, array2);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_string[] = {
	{ 140015, fn_string_compare },
	{ -1, 0 }
};
