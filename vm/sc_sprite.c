
#include "can.h"
#include "img.h"
#include "host_sdl2.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_sprite_create(VMContext *c) {
	const int sprite_num = Host_SpriteNew();
	debug(DBG_SYSCALLS, "Sprite:create sprite:%d", sprite_num);
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
	HostSprite *spr = Host_SpriteGet(sprite_num);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		if (type == PAN_ASSET_TYPE_CAN) {
			CanData *data = LoadCan(pb.buffer, pb.size);
			if (data) {
				spr->animation_data = data;
				Host_SetSpriteAnim(sprite_num, 0); /* todo: should be first animation ID, not 0 */
			}
		} else if (type == PAN_ASSET_TYPE_IMG) {
			SDL_Surface *s = LoadImg(pb.buffer, pb.size);
			if (s) {
				Host_SetSpriteImage(sprite_num, s);
			}
			Pan_UnloadAsset(&pb);
		} else {
			error("Unsupported type %d for Sprite:Image", type);
		}
		spr->asset = asset;
	} else {
		error("Failed to load asset:%d type:%d for Sprite:image", asset, type);
	}
}

static void fn_sprite_destroy(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:destroy sprite:%d", sprite_num);
	Host_SpriteDelete(sprite_num);
}

static void fn_sprite_order(VMContext *c) {
	int order = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:order sprite:%d order:%d", sprite_num, order);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	spr->order = order;
}

static void fn_sprite_hidden(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:hidden sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	VM_Push(c, spr->hidden, VAR_TYPE_INT32);
}

static void fn_sprite_hide(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:hide sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	spr->hidden = 1;
}

static void fn_sprite_show(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:show sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	spr->hidden = 0;
}

static void fn_sprite_xpos(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:xPos sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	VM_Push(c, spr->x, VAR_TYPE_INT32);
}

static void fn_sprite_ypos(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:yPos sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	VM_Push(c, spr->y, VAR_TYPE_INT32);
}

static void fn_sprite_xsize(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:xSize sprite:%d", sprite_num);
	int w;
	Host_GetSpriteSize(sprite_num, &w, 0);
	VM_Push(c, w, VAR_TYPE_INT32);
}

static void fn_sprite_ysize(VMContext *c) {
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:ySize sprite:%d", sprite_num);
	int h;
	Host_GetSpriteSize(sprite_num, 0, &h);
	VM_Push(c, h, VAR_TYPE_INT32);
}

static void fn_sprite_frame(VMContext *c) {
	int frame = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:frame sprite:%d frame:%d", sprite_num, frame);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	if (spr->animation_state) {
		if (frame != 0) {
			spr->animation_state->current_frame = frame - 1;
		} else {
			error("Unhandled Sprite:frame 0");
		}
	}
}

static void fn_sprite_rate(VMContext *c) {
	const float rate = VM_Pop(c, VAR_TYPE_FLOAT);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:rate sprite:%d rate:%f", sprite_num, rate);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	spr->rate = rate;
}

static void fn_sprite_num_frames(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:numFrames sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int count = GetAnimationFramesCount(spr->animation_data, Host_GetSpriteAnim(sprite_num));
	VM_Push(c, count, VAR_TYPE_INT32);
}

static void fn_sprite_attach_image(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sprite:attachImage");
}

static void fn_sprite_asset_id(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:assetId", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	VM_Push(c, spr->asset, VAR_TYPE_INT32);
}

static void fn_sprite_get_frame(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:getFrame sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int frame = 0;
	if (spr->animation_state) {
		frame = spr->animation_state->current_frame;
	}
	VM_Push(c, frame, VAR_TYPE_INT32);
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
	HostSprite *spr = Host_SpriteGet(sprite_num);
	if (spr->animation_data) {
		const int animation_index = FindAnimation(spr->animation_data, num);
		Host_SetSpriteAnim(sprite_num, animation_index);
	}
}

static void fn_sprite_show_layer(VMContext *c) {
	int layer = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	warning("Unimplemented Sprite:showLayer sprite:%d layer:%d", sprite_num, layer);
}

static void fn_sprite_hide_layer(VMContext *c) {
	int layer = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	warning("Unimplemented Sprite:hideLayer sprite:%d layer:%d", sprite_num, layer);
}

static void fn_sprite_animation_bounds(VMContext *c) {
	int anim = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:animationBounds sprite:%d animation:%d", sprite_num, anim);
	int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
	HostSprite *spr = Host_SpriteGet(sprite_num);
	GetAnimationBounds(spr->animation_data, anim, &x1, &y1, &x2, &y2);
	debug(DBG_SYSCALLS, "Sprite:animationBounds [%d,%d,%d,%d]", x1, y1, x2, y2);
	VM_Push(c, x1, VAR_TYPE_INT32);
	VM_Push(c, y1, VAR_TYPE_INT32);
	VM_Push(c, x2 - x1, VAR_TYPE_INT32);
	VM_Push(c, y2 - y1, VAR_TYPE_INT32);
}

static void fn_sprite_loop(VMContext *c) {
	const int loop = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:loop sprite:%d loop:%d", sprite_num, loop);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	if (spr->animation_state) {
		//spr->animation_state->loop = loop;
	}
}

static void fn_sprite_done(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:done sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int done = 1;
	if (spr->animation_state) {
		done = Can_Done(spr->animation_data, spr->animation_state);
	}
	VM_Push(c, done, VAR_TYPE_INT32);
}

static void fn_sprite_frame_bounds(VMContext *c) {
	if (c->gameID >= GID_MONSTERS) {
		VM_PopInt32(c);
	}
	int frame = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:frameBounds sprite:%d frame:%d", sprite_num, frame);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
	GetCanBitmapBounds(spr->animation_data, Host_GetSpriteAnim(sprite_num), frame - 1, &x1, &y1, &x2, &y2);
	debug(DBG_SYSCALLS, "Sprite:frameBounds [%d,%d,%d,%d]", x1, y1, x2 - x1, y2 - y1);
	VM_Push(c, x1, VAR_TYPE_INT32);
	VM_Push(c, y1, VAR_TYPE_INT32);
	VM_Push(c, x2 - x1, VAR_TYPE_INT32);
	VM_Push(c, y2 - y1, VAR_TYPE_INT32);
}

static void fn_sprite_trigger(VMContext *c) {
	int frame = VM_PopInt32(c);
	const int trigger = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:trigger sprite_num:%d %d %d", sprite_num, trigger, frame);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int res = 0;
	if (spr->animation_state) {
		if (frame == 0) {
			frame = spr->animation_state->current_frame;
		} else {
			--frame;
		}
		if (Can_HasTrigger(spr->animation_data, spr->animation_state, frame, trigger)) {
			res = 1;
		}
	}
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void fn_sprite_triggers(VMContext *c) {
	int frame = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:triggers sprite_num:%d %d", sprite_num, frame);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int res = 0;
	if (spr->animation_state) {
		if (frame == 0) {
			frame = spr->animation_state->current_frame;
		} else {
			--frame;
		}
		if (Can_GetTriggersCount(spr->animation_data, spr->animation_state, frame) != 0) {
			res = 1;
		}
	}
	VM_Push(c, res, VAR_TYPE_INT32);
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
	const int flag = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:flipX sprite_num:%d %d", sprite_num, flag);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	spr->flip_x = flag;
	if (flag) {
		warning("Unimplemented Sprite:flipX");
	}
}

static void fn_sprite_flip_y(VMContext *c) {
	const int flag = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:flipY sprite_num:%d %d", sprite_num, flag);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	spr->flip_y = flag;
	if (flag) {
		warning("Unimplemented Sprite:flipY");
	}
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

static void fn_sprite_get_animation(VMContext *c) {
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:getAnimation sprite:%d", sprite_num);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int anim = 0;
	if (spr->animation_state) {
		anim = spr->animation_state->current_animation;
	}
	VM_Push(c, spr->animation_data->entries[anim].num, VAR_TYPE_INT32);
}

static void fn_sprite_has_animation(VMContext *c) {
	const int animation = VM_PopInt32(c);
	const int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:hasAnimation sprite:%d num:%d", sprite_num, animation);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	VM_Push(c, !(FindAnimation(spr->animation_data, animation) < 0), VAR_TYPE_INT32);
}

static void fn_sprite_hit(VMContext *c) {
	const int y = VM_PopInt32(c);
	const int x = VM_PopInt32(c);
	int sprite_num = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sprite:hit sprite:%d [%d,%d]", sprite_num, x, y);
	HostSprite *spr = Host_SpriteGet(sprite_num);
	int anim = 0;
	if (spr->animation_state) {
		anim = spr->animation_state->current_animation;
	}
	int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
	GetAnimationBounds(spr->animation_data, anim, &x1, &y1, &x2, &y2);
	const int res = (y >= y1 && y <= y2) && (x >= x1 && x <= x2);
	VM_Push(c, res, VAR_TYPE_INT32);
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
	{ 30065, fn_sprite_get_animation },
	{ 30067, fn_sprite_has_animation },
	{ 30069, fn_sprite_hit },
	{ 30075, fn_sprite_blend_layer },
	{ -1, 0 }
};
