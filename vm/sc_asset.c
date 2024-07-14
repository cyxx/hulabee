
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_asset_load(VMContext *c) {
	const char *s = "";
	int num = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	if (num) {
		s = ArrayHandle_GetString(c, num);
	}
	int a = VM_PopInt32(c);
	int b = VM_PopInt32(c);
	warning("Unimplemented Asset:load '%s' %d %d", s, a, b);
}

static void fn_asset_exists(VMContext *c) {
	int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Asset:exists asset:%d", asset);
	VM_Push(c, Pan_HasAsset(asset), VAR_TYPE_INT32);
}

static void fn_asset_load_assets_def(VMContext *c) {
	const char *s = "";
	int num = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	if (num) {
		s = ArrayHandle_GetString(c, num);
	}
	VM_Push(c, 0, VAR_TYPE_INT32);
	warning("Unimplemented Asset:loadAssetsDef %s %d", s, num);
}

static void fn_asset_use_pan_files(VMContext *c) {
	VM_Push(c, 1, VAR_TYPE_INT32);
}

static void fn_asset_preload(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Asset:preload");
}

static void fn_asset_heap_size(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Asset:heapSize");
}

static void fn_asset_get_data_content(VMContext *c) {
	const int asset = VM_PopInt32(c);
	const int mode = VM_PopInt32(c);
	switch (mode) {
	case 1:
		if (Pan_GetAssetType(asset) != 8) {
			error("Asset %d is not a string", asset);
		} else {
			warning("getDataContent mode:%d unimplemented", mode);
			VM_PushString(c, 0);
		}
		break;
	case 2:
		if (Pan_GetAssetType(asset) != 10) {
			error("Asset %d is not data", asset);
		} else {
			int array = 0;
			PanBuffer pb;
			if (Pan_LoadAssetById(asset, &pb)) {
				// array = loadDataAsArray(VAR_TYPE_INT32, buffer);
			}
			VM_Push(c, array, 0x10000 | VAR_TYPE_INT32);
		}
		break;
	default:
		error("Illegal mode:%d for Asset:getData()", mode);
		break;
	}
}

const VMSyscall _syscalls_asset[] = {
	{ 160001, fn_asset_load },
	{ 160002, fn_asset_exists },
	{ 160003, fn_asset_load_assets_def },
	{ 160004, fn_asset_use_pan_files },
	{ 160005, fn_asset_preload },
	{ 160008, fn_asset_heap_size },
	{ 160010, fn_asset_get_data_content },
	{ -1, 0 }
};

