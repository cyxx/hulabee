
#include "ini.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_asset_load(VMContext *c) {
	const char *s = VM_PopString(c);
	const int a = VM_PopInt32(c);
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Asset:load asset:%d %d '%s'", asset, a, s);
	Pan_LoadAssetById(asset, 0);
}

static void fn_asset_exists(VMContext *c) {
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Asset:exists asset:%d", asset);
	VM_Push(c, Pan_HasAsset(asset), VAR_TYPE_INT32);
}

static void fn_asset_load_assets_def(VMContext *c) {
	const char *s = VM_PopString(c);
	debug(DBG_SYSCALLS, "Asset:loadAssetsDef '%s'", s);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_asset_use_pan_files(VMContext *c) {
	VM_Push(c, 1, VAR_TYPE_INT32);
}

static void fn_asset_preload(VMContext *c) {
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Asset:preload asset:%d", asset);
	Pan_LoadAssetById(asset, 0);
}

static void fn_asset_unload(VMContext *c) {
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Asset:unload asset:%d", asset);
	warning("Unimplemented Asset:unload");
}

static void fn_asset_heap_size(VMContext *c) {
	const int b = VM_PopInt32(c);
	const int a = VM_PopInt32(c);
	warning("Unimplemented Asset:heapSize %d %d", a, b);
}

static void fn_asset_get_data_content(VMContext *c) {
	const int asset = VM_PopInt32(c);
	const int mode = VM_PopInt32(c);
	switch (mode) {
	case 1:
		if (Pan_GetAssetType(asset) != PAN_ASSET_TYPE_TXT) {
			error("Asset %d is not a string", asset);
		} else {
			int array_handle = 0;
			PanBuffer pb;
			if (Pan_LoadAssetById(asset, &pb)) {
				VMArray *array = Array_New(c);
				Array_Dim(array, 0x10000 | VAR_TYPE_CHAR, 1, pb.size + 1);
				memcpy(array->data, pb.buffer, pb.size);
				array->data[pb.size] = 0;
				array_handle = array->handle;
			}
			VM_Push(c, array_handle, 0x10000 | VAR_TYPE_CHAR);
		}
		break;
	case 2:
		if (Pan_GetAssetType(asset) != 10) {
			error("Asset %d is not data", asset);
		} else {
			int array_handle = 0;
			PanBuffer pb;
			if (Pan_LoadAssetById(asset, &pb)) {
				VMArray *array = Array_New(c);
				Array_Dim(array, 0x10000 | VAR_TYPE_INT32, 1, pb.size / sizeof(uint32_t));
				memcpy(array->data, pb.buffer, pb.size);
				array_handle = array->handle;
			}
			VM_Push(c, array_handle, 0x10000 | VAR_TYPE_INT32);
		}
		break;
	default:
		error("Illegal mode:%d for Asset:getData()", mode);
		break;
	}
}

static void parse_asset_ini(const char *section, const char *key, const char *value) {
	fprintf(stdout, "Asset:Ini section:%s %s=%s\n", section, key, value);
}

static void fn_asset_read_ini(VMContext *c) {
	const char *val = VM_PopString(c);
	const char *key = VM_PopString(c);
	const char *section = VM_PopString(c);
	const int asset = VM_PopInt32(c);
	warning("Unimplemented Asset:readIni asset:%d section:'%s' key:'%s'", asset, section, key);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		LoadIni(pb.buffer, pb.size, parse_asset_ini);
		Pan_UnloadAsset(&pb);
	}
	VM_PushString(c, val);
}

const VMSyscall _syscalls_asset[] = {
	{ 160001, fn_asset_load },
	{ 160002, fn_asset_exists },
	{ 160003, fn_asset_load_assets_def },
	{ 160004, fn_asset_use_pan_files },
	{ 160005, fn_asset_preload },
	{ 160008, fn_asset_heap_size },
	{ 160009, fn_asset_unload },
	{ 160010, fn_asset_get_data_content },
	{ 160014, fn_asset_read_ini },
	{ -1, 0 }
};

