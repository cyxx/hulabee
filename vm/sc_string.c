
#include "util.h"
#include "vm.h"

static void fn_string_split(VMContext *c) {
	const int array2 = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const char *sep = array2 ? ArrayHandle_GetString(c, array2) : " \t";
	const int array1 = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const char *str = ArrayHandle_GetString(c, array1);
	debug(DBG_SYSCALLS, "string:split '%s' '%s'", str, sep);

	VMArray *array = Array_New(c);
	Array_Dim(array, 0x10000 | VAR_TYPE_CHAR, 1, 0);

	char *tmp = strdup(str);
	char *ptr = 0;
	char *token = strtok_r(tmp, sep, &ptr);
	if (token) {
		/* todo */
		while (1) {
			token = strtok_r(0, sep, &ptr);
			if (!token) {
				break;
			}
			/* todo */
		}
	}
	free(tmp);
	VM_Push(c, array->handle, 0x10100 | VAR_TYPE_CHAR);
}

static void fn_string_lower(VMContext *c) {
	const int array = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	debug(DBG_SYSCALLS, "string:lower array:%d", array);
	ArrayHandle_LowerString(c, array);
}

static void fn_string_upper(VMContext *c) {
	const int array = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	debug(DBG_SYSCALLS, "string:upper array:%d", array);
	ArrayHandle_UpperString(c, array);
}

static void fn_string_compare(VMContext *c) {
	const int array2 = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const int array1 = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	debug(DBG_SYSCALLS, "string:compare");
	const int ret = ArrayHandle_CompareString(c, array1, array2);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_string[] = {
	{ 140004, fn_string_split },
	{ 140005, fn_string_lower },
	{ 140006, fn_string_upper },
	{ 140015, fn_string_compare },
	{ -1, 0 }
};
