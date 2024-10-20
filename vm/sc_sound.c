
#include "mixer.h"
#include "pan.h"
#include "util.h"
#include "vm.h"

static void fn_sound_create(VMContext *c) {
	const int channel = Mixer_CreateChannel();
	debug(DBG_SYSCALLS, "Sound:create channel:%d", channel);
	VM_Push(c, channel, VAR_TYPE_INT32);
}

static void fn_sound_destroy(VMContext *c) {
	const int channel = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sound:destroy channel:%d", channel);
	Mixer_DestroyChannel(channel);
}

static void fn_sound_open(VMContext *c) {
	const int asset = VM_PopInt32(c);
	const int channel = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sound:open channel:%d asset:%d", channel, asset);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		Mixer_Open(channel, asset, pb.buffer, pb.size);
	} else {
		warning("Failed to load asset:%d for Sound:open", asset);
	}
}

static void fn_sound_play(VMContext *c) {
	VM_PopInt32(c);
	const int channel = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sound:play channel:%d", channel);
	Mixer_Play(channel);
}

static void fn_sound_pause(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sound:pause");
}

static void fn_sound_resume(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sound:resume");
}

static void fn_sound_stop(VMContext *c) {
	const int channel = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sound:stop channel:%d", channel);
	/* todo: Pan_Unload */
	Mixer_Stop(channel);
}

static void fn_sound_status(VMContext *c) {
	const int channel = VM_PopInt32(c);
	const int status = channel != 0 ? Mixer_GetStatus(channel) : 1 /* DONE */;
	debug(DBG_SYSCALLS, "Sound:status channel:%d status:%d", channel, status);
	VM_Push(c, status, VAR_TYPE_INT32);
}

static void fn_sound_set_volume(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sound:setVolume");
}

static void fn_sound_get_volume(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sound:getVolume");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sound_set_pan(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sound:setPan");
}

static void fn_sound_set_rate(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sound:setRate");
}

static void fn_sound_play_resource(VMContext *c) {
	const int channel = VM_PopInt32(c);
	const int asset = VM_PopInt32(c);
	debug(DBG_SYSCALLS, "Sound:playResource channel:%d asset:%d", channel, asset);
	PanBuffer pb;
	if (Pan_LoadAssetById(asset, &pb)) {
		Mixer_Open(channel, asset, pb.buffer, pb.size);
		Mixer_Play(channel);
	}
}

static void fn_sound_halt(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sound:halt");
}

static void fn_sound_stop_all(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sound:stopAll");
}

static void fn_sound_master_volume(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented Sound:masterVolume");
}

static void fn_sound_playing(VMContext *c) {
	const int asset = VM_PopInt32(c);
	const int status = Mixer_IsPlaying(asset);
	debug(DBG_SYSCALLS, "Sound:playing asset:%d status:%d", asset, status);
	VM_Push(c, status, VAR_TYPE_INT32);
}

static void fn_sound_flags(VMContext *c) {
	VM_PopInt32(c);
	const int mode = VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented Sound:flags");
	if (mode == 1) {
		VM_Push(c, 0, VAR_TYPE_INT32);
	}
}

const VMSyscall _syscalls_sound[] = {
	{ 80001, fn_sound_create },
	{ 80002, fn_sound_destroy },
	{ 80003, fn_sound_open },
	{ 80004, fn_sound_play },
	{ 80005, fn_sound_pause },
	{ 80006, fn_sound_resume },
	{ 80007, fn_sound_stop },
	{ 80008, fn_sound_status },
	{ 80009, fn_sound_set_volume },
	{ 80010, fn_sound_get_volume },
	{ 80011, fn_sound_set_pan },
	{ 80013, fn_sound_set_rate },
	{ 80019, fn_sound_play_resource },
	{ 80022, fn_sound_halt },
	{ 80023, fn_sound_stop_all },
	{ 80024, fn_sound_master_volume },
	{ 80025, fn_sound_playing },
	{ 80026, fn_sound_flags },
	{ -1, 0 }
};
