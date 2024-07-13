
#include "img.h"
#include "host_sdl2.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_window_create(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Window:create()");
}

static void fn_window_background(VMContext *c) {
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "window:background asset:%d", asset);
	const int type = Pan_GetAssetType(asset);
	if (type == PAN_ASSET_TYPE_IMG) {
		PanBuffer pb;
		if (Pan_LoadAssetById(asset, &pb)) {
			Bitmap bmp;
			LoadImg(pb.buffer, pb.size, &bmp);
			Host_SetWindowBackgroundTexture(bmp.texture);
		}
	} else {
		warning("Unsupported asset type:%d", type);
	}
}

static void fn_window_size(VMContext *c) {
	assert(g_window);
	int h = VM_PopInt32(c);
	int w = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "window:size w:%d h:%d", w, h);
	SDL_SetWindowSize(g_window, w, h);
}

static void fn_window_x_size(VMContext *c) {
	debug(DBG_SYSCALLS, "window:xSize");
	assert(g_window);
	int w;
	SDL_GetWindowSize(g_window, &w, 0);
	VM_Push(c, w, VAR_TYPE_INT32);
}

static void fn_window_y_size(VMContext *c) {
	debug(DBG_SYSCALLS, "window:ySize");
	assert(g_window);
	int h;
	SDL_GetWindowSize(g_window, 0, &h);
	VM_Push(c, h, VAR_TYPE_INT32);
}

static void fn_window_blank(VMContext *c) {
	VM_PopInt32(c);
	debug(DBG_SYSCALLS, "window:blank");
	warning("Unimplemented Window:blank()");
}

static void fn_window_mode_mangle_rgb(VMContext *c) {
	debug(DBG_SYSCALLS, "window:modeMangleRGB");
	const uint32_t color = VM_PopInt32(c);
	VM_Push(c, color & 0xFFF8F8F8, VAR_TYPE_INT32);
}

const VMSyscall _syscalls_window[] = {
	{ 90001, fn_window_create },
	{ 90002, fn_window_background },
	{ 90004, fn_window_size },
	{ 90008, fn_window_x_size },
	{ 90009, fn_window_y_size },
	{ 90011, fn_window_blank },
	{ 90019, fn_window_mode_mangle_rgb },
	{ -1, 0 }
};
