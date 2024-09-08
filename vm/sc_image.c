
#include "can.h"
#include "host_sdl2.h"
#include "img.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_image_new_size(VMContext *c) {
	const int d = VM_PopInt32(c);
	const int h = VM_PopInt32(c);
	const int w = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Image:newSize w:%d h:%d %d", w, h, d);
	const int image_num = Host_ImageNew();
	Host_ImageCreate(image_num, w, h, d);
	VM_Push(c, image_num, VAR_TYPE_INT32);
}

static void fn_image_new_image(VMContext *c) {
	int b = VM_PopInt32(c);
	int a = VM_PopInt32(c);
	int asset = VM_PopInt32(c);
	int image_num = Host_ImageNew();
	int type = Pan_GetAssetType(asset);
	debug(DBG_SYSCALLS, "Image:newImage type:%d asset:%d %d %d", type, asset, a, b);
	HostImage *img = Host_ImageGet(image_num);
	if (type == PAN_ASSET_TYPE_IMG) {
		PanBuffer pb;
		if (Pan_LoadAssetById(asset, &pb)) {
			SDL_Surface *s = LoadImg(pb.buffer, pb.size);
			img->s = s;
			Pan_UnloadAsset(&pb);
		}
	} else {
		error("Unhandled asset type:%d for Image:new", type);
	}
	VM_Push(c, image_num, VAR_TYPE_INT32);
}

static void fn_image_draw(VMContext *c) {
	int a = VM_PopInt32(c);
	int y = VM_PopInt32(c);
	int x = VM_PopInt32(c);
	int num = VM_PopInt32(c);
	warning("Unimplemented Image:draw %d %d %d %d", num, x, y, a);
}

static void fn_image_image_at(VMContext *c) {
	int b = VM_PopInt32(c);
	int a = VM_PopInt32(c);
	int frame = VM_PopInt32(c);
	int anim = VM_PopInt32(c);
	int asset = VM_PopInt32(c);
	int image_num = VM_PopInt32(c);
	int type = Pan_GetAssetType(asset);
	debug(DBG_SYSCALLS, "Image:imageAt asset:%d type:%d image:%d %d %d %d %d", asset, type, image_num, anim, frame, a, b);
	HostImage *img = Host_ImageGet(image_num);
	if (type == PAN_ASSET_TYPE_CAN) {
		PanBuffer pb;
		if (Pan_LoadAssetById(asset, &pb)) {
			CanData *data = LoadCan(pb.buffer, pb.size);
			if (data) {
				const int animation_index = FindAnimation(data, anim);
				Can_Draw(data, animation_index, frame - 1, img->s, 0 /* x */, 0 /* y */, 0 /* flags */);
				UnloadCan(data);
				Pan_UnloadAsset(&pb);
			}
		}
	} else {
		error("Unhandled asset type:%d for Image:at", type);
	}
}

static void fn_image_clear(VMContext *c) {
	int color = VM_PopInt32(c);
	int num = VM_PopInt32(c);
	warning("Unimplemented Image:clear num:%d color:0x%x", num, color);
}

static void fn_image_print_at(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_Pop(c, 0x10000 | VAR_TYPE_INT32);
	VM_PopInt32(c);
	VM_PopString(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:printAt");
}

static void fn_image_print(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_Pop(c, 0x10000 | VAR_TYPE_INT32);
	VM_PopInt32(c);
	VM_PopString(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:print");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_image_line(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:line");
}

static void fn_image_rect(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:rect");
}

static void fn_image_get_x_size(VMContext *c) {
	const int image_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Image:x_size num:%d", image_num);
	HostImage *img = Host_ImageGet(image_num);
	const SDL_Surface *s = img->s;
	VM_Push(c, s ? s->w : 0, VAR_TYPE_INT32);
}

static void fn_image_get_y_size(VMContext *c) {
	const int image_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Image:y_size num:%d", image_num);
	HostImage *img = Host_ImageGet(image_num);
	const SDL_Surface *s = img->s;
	VM_Push(c, s ? s->h : 0, VAR_TYPE_INT32);
}

static void fn_image_pen_color(VMContext *c) {
	const int color = VM_PopInt32(c);
	const int image_num = VM_PopInt32(c);
	warning("Unimplemented Image:penColor num:%d color:0x%x", image_num, color);
}

static void fn_image_destroy(VMContext *c) {
	const int image_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Image:destroy num:%d", image_num);
	Host_ImageDelete(image_num);
}

const VMSyscall _syscalls_image[] = {
	{ 100001, fn_image_new_size },
	{ 100002, fn_image_new_image },
	{ 100003, fn_image_draw },
	{ 100005, fn_image_image_at },
	{ 100007, fn_image_clear },
	{ 100009, fn_image_print_at },
	{ 100010, fn_image_print },
	{ 100011, fn_image_line },
	{ 100012, fn_image_rect },
	{ 100025, fn_image_get_x_size },
	{ 100026, fn_image_get_y_size },
	{ 100028, fn_image_pen_color },
	{ 100031, fn_image_destroy },
	{ -1, 0 }
};
