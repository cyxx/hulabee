
#include "can.h"
#include "img.h"
#include "host_sdl2.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_sprite_create(VMContext *c) {
	debug(DBG_SYSCALLS, "Sprite:create");
	const int sprite_num = Host_CreateSprite();
	VM_Push(c, sprite_num, VAR_TYPE_INT32);
}

static void fn_sprite_at(VMContext *c) {
	int dy = VM_PopInt32(c);
	int dx = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:at sprite:%d x:%d y:%d", sprite_num, dx, dy);
	Host_SetSpritePos(sprite_num, dx, dy);
}

static void fn_sprite_image(VMContext *c) {
	const int asset = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	const int type = Pan_GetAssetType(asset);
	debug(DBG_SYSCALLS, "Sprite:image sprite:%d asset:%d type:%d", sprite_num, asset, type);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		if (type == PAN_ASSET_TYPE_CAN) {
			Animation *data = Host_GetSpriteAnimationData(sprite_num);
			LoadCan(pb.buffer, pb.size, data);
		} else if (type == PAN_ASSET_TYPE_IMG) {
			Bitmap b;
			LoadImg(pb.buffer, pb.size, &b);
			Host_SetSpriteImage(sprite_num, &b);
		} else {
			error("Unsupported type %d for Sprite:Image", type);
		}
	}
}

static void fn_sprite_destroy(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	warning("Unimplemented Sprite:destroy sprite:%d", sprite_num);
}

static void fn_sprite_order(VMContext *c) {
	int order = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:order sprite:%d order:%d", sprite_num, order);
	Host_SetSpriteOrder(sprite_num, order);
}

static void fn_sprite_hidden(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:hidden sprite:%d", sprite_num);
	VM_Push(c, Host_IsSpriteHidden(sprite_num), VAR_TYPE_INT32);
}

static void fn_sprite_hide(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:hide sprite:%d", sprite_num);
	Host_ShowSprite(sprite_num, 0);
}

static void fn_sprite_show(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:show sprite:%d", sprite_num);
	Host_ShowSprite(sprite_num, 1);
}

static void fn_sprite_xpos(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:xPos sprite:%d", sprite_num);
	int x_pos;
	Host_GetSpritePos(sprite_num, &x_pos, 0);
	VM_Push(c, x_pos, VAR_TYPE_INT32);
}

static void fn_sprite_ypos(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:yPos sprite:%d", sprite_num);
	int y_pos;
	Host_GetSpritePos(sprite_num, 0, &y_pos);
	VM_Push(c, y_pos, VAR_TYPE_INT32);
}

static void fn_sprite_xsize(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:xSize sprite:%d", sprite_num);
	int w;
	//Host_GetSpriteSize(sprite_num, &w, 0);
	w = 1; /* tofix */
	VM_Push(c, w, VAR_TYPE_INT32);
}

static void fn_sprite_ysize(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:ySize sprite:%d", sprite_num);
	int h;
	//Host_GetSpriteSize(sprite_num, 0, &h);
	h = 1; /* tofix */
	VM_Push(c, h, VAR_TYPE_INT32);
}

static void fn_sprite_frame(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:frame");
}

static void fn_sprite_rate(VMContext *c) {
	VM_Pop(c, VAR_TYPE_FLOAT);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:rate");
}

static void fn_sprite_num_frames(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:numFrames sprite:%d", sprite_num);
	int count = GetAnimationFramesCount(Host_GetSpriteAnimationData(sprite_num), Host_GetSpriteAnim(sprite_num));
	VM_Push(c, count, VAR_TYPE_INT32);
}

static void fn_sprite_attach_image(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:attachImage");
}

static void fn_sprite_asset_id(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sprite:assetId");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sprite_get_frame(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sprite:getFrame");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sprite_get_rate(VMContext *c) {
	VM_PopInt32(c);
	VM_PushFloat(c, 1.);
	warning("Unimplemented Sprite:getRate");
}

static void fn_sprite_refresh(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sprite:refresh");
}

static void fn_sprite_animation(VMContext *c) {
	int num = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:animation sprite:%d animation:%d", sprite_num, num);
	Host_SetSpriteAnim(sprite_num, FindAnimation(Host_GetSpriteAnimationData(sprite_num), num));
}

static void fn_sprite_show_layer(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:showLayer");
}

static void fn_sprite_hide_layer(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:hideLayer");
}

static void fn_sprite_animation_bounds(VMContext *c) {
	int anim = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:animationBounds sprite:%d animation:%d", sprite_num, anim);
	int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
	GetAnimationFrameBounds(Host_GetSpriteAnimationData(sprite_num), anim, &x1, &y1, &x2, &y2);
	debug(DBG_SYSCALLS, "Sprite:animationBounds %d %d %d %d", x1, y1, x2, y2);
	VM_Push(c, x1, VAR_TYPE_INT32);
	VM_Push(c, y1, VAR_TYPE_INT32);
	VM_Push(c, x2 - x1, VAR_TYPE_INT32);
	VM_Push(c, y2 - y1, VAR_TYPE_INT32);
}

static void fn_sprite_loop(VMContext *c) {
	int a = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	warning("Unimplemented Sprite:loop %d %d", sprite_num, a);
}

static void fn_sprite_done(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sprite:done");
	VM_Push(c, 1, VAR_TYPE_INT32);
}

static void fn_sprite_frame_bounds(VMContext *c) {
	if (c->gameID >= GID_MONSTERS) {
		VM_PopInt32(c);
	}
	int frame = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:frameBounds sprite:%d frame:%d", sprite_num, frame);
	int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
	GetAnimationBitmapBounds(Host_GetSpriteAnimationData(sprite_num), Host_GetSpriteAnim(sprite_num), frame, &x1, &y1, &x2, &y2);
	debug(DBG_SYSCALLS, "Sprite:frameBounds %d %d %d %d", x1, y1, x2 - x1, y2 - y1);
	VM_Push(c, x1, VAR_TYPE_INT32);
	VM_Push(c, y1, VAR_TYPE_INT32);
	VM_Push(c, x2 - x1, VAR_TYPE_INT32);
	VM_Push(c, y2 - y1, VAR_TYPE_INT32);
}

static void fn_sprite_trigger(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:trigger");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sprite_triggers(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:triggers");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sprite_set_clip_rect(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:setClipRect");
}

static void fn_sprite_flip_x(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:flipX");
}

static void fn_sprite_flip_y(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:flipY");
}

static void fn_sprite_rotate(VMContext *c) {
	VM_PopInt32(c);
	VM_PopFloat(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:rotate");
}

static void fn_sprite_scale(VMContext *c) {
	VM_PopInt32(c);
	VM_PopFloat(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:scale");
}

static void fn_sprite_adjust_color(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:adjustColor");
}

static void fn_sprite_has_animation(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:hasAnimation");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sprite_hit(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:hit");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sprite_blend_layer(VMContext *c) {
	VM_PopInt32(c);
	VM_PopFloat(c);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:blendLayer");
}

const VMSyscall _syscalls_sprite[] = {
	{ 30001, fn_sprite_create },
	{ 30002, fn_sprite_at },
	{ 30003, fn_sprite_image },
	{ 30004, fn_sprite_destroy },
	{ 30005, fn_sprite_order },
	{ 30007, fn_sprite_hidden },
	{ 30008, fn_sprite_hide },
	{ 30009, fn_sprite_show },
	{ 30010, fn_sprite_xpos },
	{ 30011, fn_sprite_ypos },
	{ 30012, fn_sprite_xsize },
	{ 30013, fn_sprite_ysize },
	{ 30015, fn_sprite_frame },
	{ 30016, fn_sprite_rate },
	{ 30017, fn_sprite_num_frames },
	{ 30018, fn_sprite_attach_image },
	{ 30020, fn_sprite_asset_id },
	{ 30021, fn_sprite_get_frame },
	{ 30022, fn_sprite_get_rate },
	{ 30024, fn_sprite_refresh },
	{ 30025, fn_sprite_animation },
	{ 30026, fn_sprite_show_layer },
	{ 30027, fn_sprite_hide_layer },
	{ 30028, fn_sprite_animation_bounds },
	{ 30029, fn_sprite_loop },
	{ 30033, fn_sprite_done },
	{ 30034, fn_sprite_frame_bounds },
	{ 30037, fn_sprite_trigger },
	{ 30038, fn_sprite_triggers },
	{ 30055, fn_sprite_set_clip_rect },
	{ 30057, fn_sprite_flip_x },
	{ 30058, fn_sprite_flip_y },
	{ 30060, fn_sprite_rotate },
	{ 30061, fn_sprite_scale },
	{ 30063, fn_sprite_adjust_color },
	{ 30067, fn_sprite_has_animation },
	{ 30069, fn_sprite_hit },
	{ 30075, fn_sprite_blend_layer },
	{ -1, 0 }
};
