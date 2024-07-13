
#include "host_sdl2.h"
#include "img.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_image_new_size(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:newSize()");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_image_new_image(VMContext *c) {
	debug(DBG_SYSCALLS, "Image:newImage");
	int a = VM_PopInt32(c);
	int b = VM_PopInt32(c);
	int asset = VM_PopInt32(c);
	int num = 0;
	int type = Pan_GetAssetType(asset);
	if (type == PAN_ASSET_TYPE_IMG) {
		PanBuffer pb;
		if (Pan_LoadAssetById(asset, &pb)) {
			Bitmap bmp;
			LoadImg(pb.buffer, pb.size, &bmp);
			num = Host_CreateImage(&bmp);
		}
	} else {
		warning("Unhandled asset type:%d for Image", type);
	}
	VM_Push(c, num, VAR_TYPE_INT32);
}

static void fn_image_draw(VMContext *c) {
	int a = VM_PopInt32(c);
	int b = VM_PopInt32(c);
	int c_ = VM_PopInt32(c);
	int d = VM_PopInt32(c);
	warning("Unimplemented Image:draw %d %d %d %d", a, b, c, d);
}

static void fn_image_image_at(VMContext *c) {
	int a = VM_PopInt32(c);
	int b = VM_PopInt32(c);
	int c_ = VM_PopInt32(c);
	int d = VM_PopInt32(c);
	int asset = VM_PopInt32(c);
	int f = VM_PopInt32(c);
	int type = Pan_GetAssetType(asset);
	warning("Unimplemented Image:imageAt %d %d %d %d asset:%d type:%d %d", a, b, c, d, asset, type, f);
}

static void fn_image_clear(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:clear");
}

static void fn_image_print_at(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_Pop(c, 0x10007);
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
	VM_Pop(c, 0x10007);
	VM_PopInt32(c);
	VM_PopString(c);
	VM_PopInt32(c);
	warning("Unimplemented Image:print");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_image_get_x_size(VMContext *c) {
	VM_PopInt32(c);
	VM_Push(c, 0, VAR_TYPE_INT32);
	warning("Unimplemented Image:x_size");
}

static void fn_image_get_y_size(VMContext *c) {
	VM_PopInt32(c);
	VM_Push(c, 0, VAR_TYPE_INT32);
	warning("Unimplemented Image:y_size");
}

const VMSyscall _syscalls_image[] = {
	{ 100001, fn_image_new_size },
	{ 100002, fn_image_new_image },
	{ 100003, fn_image_draw },
	{ 100005, fn_image_image_at },
	{ 100007, fn_image_clear },
	{ 100009, fn_image_print_at },
	{ 100010, fn_image_print },
	{ 100025, fn_image_get_x_size },
	{ 100026, fn_image_get_y_size },
	{ -1, 0 }
};
