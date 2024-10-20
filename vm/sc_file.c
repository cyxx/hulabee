
#include "fileio.h"
#include "util.h"
#include "vm.h"

static void fn_file_open(VMContext *c) {
	const int mode = VM_PopInt32(c);
	const char *name = VM_PopString(c);
	char path[1024];
	if (name) {
		Fio_ResolvePath(name, path, sizeof(path));
	} else {
		path[0] = 0;
	}
	debug(DBG_SYSCALLS, "File:open '%s' %d", path, mode);
	const int fh = Fio_Open(path, mode);
	VM_Push(c, fh ? BASE_HANDLE_FILE + fh : 0, VAR_TYPE_INT32);
}

static void fn_file_eof(VMContext *c) {
	const int handle = VM_PopInt32(c) - BASE_HANDLE_FILE;
	debug(DBG_SYSCALLS, "File:eof f:%d", handle);
	const int ret = Fio_Eof(handle);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

static void fn_file_close(VMContext *c) {
	const int handle = VM_PopInt32(c) - BASE_HANDLE_FILE;
	debug(DBG_SYSCALLS, "File:close f:%d", handle);
	Fio_Close(handle);
}

static void fn_file_exists(VMContext *c) {
	const char *name = VM_PopString(c);
	char path[1024];
	if (name) {
		Fio_ResolvePath(name, path, sizeof(path));
	} else {
		path[0] = 0;
	}
	debug(DBG_SYSCALLS, "File:exists '%s'", path);
	const int ret = Fio_Exists(path);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

static void fn_file_delete(VMContext *c) {
	const char *name = VM_PopString(c);
	char path[1024];
	if (name) {
		Fio_ResolvePath(name, path, sizeof(path));
	} else {
		path[0] = 0;
	}
	warning("Unimplemented File:delete '%s'", path);
	VM_Push(c, -1, VAR_TYPE_INT32);
}

static void fn_file_write_int(VMContext *c) {
	const int handle = VM_PopInt32(c) - BASE_HANDLE_FILE;
	const int value = VM_PopInt32(c);
	const int sz = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "File:writeInt f:%d value:%d sz:%d", handle, value, sz);
	Fio_WriteInt(handle, value, sz);
}

static void fn_file_read_int(VMContext *c) {
	const int handle = VM_PopInt32(c) - BASE_HANDLE_FILE;
	const int sz = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "File:readInt f:%d sz:%d", handle, sz);
	const int x = Fio_ReadInt(handle, sz);
	VM_Push(c, x, VAR_TYPE_INT32);
}

static void fn_file_get_list(VMContext *c) {
	const char *s = VM_PopString(c);
	const int a = VM_PopInt32(c);
	warning("Unimplemented File:getList s:%s %d", s, a);
	VMArray *array = Array_New(c);
	Array_Dim(array, 0x10000 | VAR_TYPE_CHAR, 1, 0);
	VM_Push(c, array->handle, 0x10100 | VAR_TYPE_CHAR);
}

static void fn_file_create_directory(VMContext *c) {
	const char *name = VM_PopString(c);
	char path[1024];
	if (name) {
		Fio_ResolvePath(name, path, sizeof(path));
	} else {
		path[0] = 0;
	}
	warning("Unimplemented File:createDirectory '%s' (%s)", path, name);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_file[] = {
	{ 20001, fn_file_open },
	{ 20003, fn_file_eof },
	{ 20004, fn_file_close },
	{ 20006, fn_file_exists },
	{ 20007, fn_file_delete },
	{ 20009, fn_file_write_int },
	{ 20010, fn_file_read_int },
	{ 20017, fn_file_get_list },
	{ 20021, fn_file_create_directory },
	{ -1, 0 }
};
