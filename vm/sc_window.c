
#include "can.h"
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
	debug(DBG_SYSCALLS, "Window:background asset:%d", asset);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		const int type = Pan_GetAssetType(asset);
		if (type == PAN_ASSET_TYPE_IMG) {
			SDL_Surface *s = LoadImg(pb.buffer, pb.size);
			if (s) {
				Host_SetWindowBackground(s);
				SDL_FreeSurface(s);
			}
		} else if (type == PAN_ASSET_TYPE_JPG) {
			SDL_Surface *s = LoadJpg(pb.buffer, pb.size);
			if (s) {
				Host_SetWindowBackground(s);
				SDL_FreeSurface(s);
			}
		} else {
			error("Unsupported asset type:%d for Window:background", type);
		}
		Pan_UnloadAsset(&pb);
	}
}

static void fn_window_move(VMContext *c) {
	assert(g_window);
	int y = VM_PopInt32(c);
	int x = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Window:move x:%d y:%d", x, y);
	SDL_SetWindowPosition(g_window, x, y);
}

static void fn_window_size(VMContext *c) {
	assert(g_window);
	int h = VM_PopInt32(c);
	int w = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Window:size w:%d h:%d", w, h);
	SDL_SetWindowSize(g_window, w, h);
}

static void fn_window_x_size(VMContext *c) {
	debug(DBG_SYSCALLS, "Window:xSize");
	assert(g_window);
	int w;
	SDL_GetWindowSize(g_window, &w, 0);
	VM_Push(c, w, VAR_TYPE_INT32);
}

static void fn_window_y_size(VMContext *c) {
	debug(DBG_SYSCALLS, "Window:ySize");
	assert(g_window);
	int h;
	SDL_GetWindowSize(g_window, 0, &h);
	VM_Push(c, h, VAR_TYPE_INT32);
}

static void fn_window_blank(VMContext *c) {
	VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Window:blank");
	warning("Unimplemented Window:blank");
}

static void fn_window_mode_mangle_rgb(VMContext *c) {
	debug(DBG_SYSCALLS, "Window:modeMangleRGB");
	const uint32_t color = VM_PopInt32(c);
	VM_Push(c, color & 0xFFF8F8F8, VAR_TYPE_INT32);
}

static void fn_window_draw(VMContext *c) {
	const int arg4 = VM_PopInt32(c);
	const int arg3 = VM_PopInt32(c);
	const int arg2 = VM_PopInt32(c);
	const int arg1 = VM_PopInt32(c);
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Window:draw asset:%d %d %d %d %d", asset, arg1, arg2, arg3, arg4);
	assert(g_background);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		const int type = Pan_GetAssetType(asset);
		if (type == PAN_ASSET_TYPE_CAN) {
			CanData *data = LoadCan(pb.buffer, pb.size);
			if (data) {
				const int animation_index = FindAnimation(data, arg1);
				Can_Draw(data, animation_index, 0 /* frame */, g_background, 0 /* x */, 0 /* y */, 0 /* flags */);
				UnloadCan(data);
			}
		} else {
			error("Window:draw attempted to draw asset %d which is not a drawable asset", asset);
		}
		Pan_UnloadAsset(&pb);
	} else {
		error("Window:draw attempted to load asset %d that does not exist", asset);
	}
}

static void fn_window_render_vbl(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Window:renderOnVBlank");
}

static void fn_window_title(VMContext *c) {
	const char *title = VM_PopString(c);
	debug(DBG_SYSCALLS, "Window:title");
	assert(g_window);
	SDL_SetWindowTitle(g_window, title);
}

const VMSyscall _syscalls_window[] = {
	{ 90001, fn_window_create },
	{ 90002, fn_window_background },
	{ 90003, fn_window_move },
	{ 90004, fn_window_size },
	{ 90008, fn_window_x_size },
	{ 90009, fn_window_y_size },
	{ 90010, fn_window_title }, /* wordspiral */
	{ 90011, fn_window_blank },
	{ 90016, fn_window_mode_mangle_rgb },
	{ 90019, fn_window_draw },
	{ 90022, fn_window_render_vbl },
	{ 90026, fn_window_title }, /* piglet1 */
	{ -1, 0 }
};
