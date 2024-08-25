
#include "util.h"
#include "vm.h"

static void fn_file_open(VMContext *c) {
	const int a = VM_PopInt32(c);
	const char *name = VM_PopString(c);
	warning("Unimplemented file:open '%s' %d", name, a);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_file_exists(VMContext *c) {
	const char *name = VM_PopString(c);
	warning("Unimplemented file:exists '%s'", name);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_file_get_list(VMContext *c) {
	const char *s = VM_PopString(c);
	const int a = VM_PopInt32(c);
	warning("Unimplemented file:getList s:%s %d", s, a);
	VMArray *array = Array_New(c);
	Array_Dim(array, 0x10000 | VAR_TYPE_CHAR, 1, 0);
	VM_Push(c, array->handle, 0x10100 | VAR_TYPE_CHAR);
}

static void fn_file_create_directory(VMContext *c) {
	const char *name = VM_PopString(c);
	warning("Unimplemented file:createDirectory '%s'", name);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_file[] = {
	{ 20001, fn_file_open },
	{ 20006, fn_file_exists },
	{ 20017, fn_file_get_list },
	{ 20021, fn_file_create_directory },
	{ -1, 0 }
};
