
#include "util.h"
#include "vm.h"

static void fn_string_split(VMContext *c) {
	const char *sep = VM_PopString(c);
	if (!sep) {
		sep = " \t";
	}
	const char *str = VM_PopString(c);
	debug(DBG_SYSCALLS, "String:split '%s' '%s'", str, sep);

	VMArray *array = Array_New(c);
	Array_Dim(array, 0x10000 | VAR_TYPE_CHAR, 1, 0);

	char *tmp = strdup(str);
	char *ptr = 0;
	char *token = strtok_r(tmp, sep, &ptr);
	if (token) {
		while (1) {
			token = strtok_r(0, sep, &ptr);
			if (!token) {
				break;
			}
			VMArray *array_token = Array_New(c);
			Array_SetString(array_token, token);
			Array_InsertUpper(array, array_token->handle);
		}
	}
	free(tmp);
	VM_Push(c, array->handle, 0x10100 | VAR_TYPE_CHAR);
}

static void fn_string_lower(VMContext *c) {
	const int array = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	debug(DBG_SYSCALLS, "String:lower array:%d", array);
	ArrayHandle_LowerString(c, array);
}

static void fn_string_upper(VMContext *c) {
	const int array = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	debug(DBG_SYSCALLS, "String:upper array:%d", array);
	ArrayHandle_UpperString(c, array);
}

static void fn_string_comparei(VMContext *c) {
	const char *str2 = VM_PopString(c);
	const char *str1 = VM_PopString(c);
	debug(DBG_SYSCALLS, "String:comparei '%s' '%s'", str1, str2);
	const int ret = strcasecmp(str1, str2);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

static void fn_string_compare(VMContext *c) {
	const char *str2 = VM_PopString(c);
	const char *str1 = VM_PopString(c);
	debug(DBG_SYSCALLS, "String:compare '%s' '%s'", str1, str2);
	const int ret = strcmp(str1, str2);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_string[] = {
	{ 140004, fn_string_split },
	{ 140005, fn_string_lower },
	{ 140006, fn_string_upper },
	{ 140007, fn_string_comparei },
	{ 140015, fn_string_compare },
	{ -1, 0 }
};
