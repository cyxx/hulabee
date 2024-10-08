
#include "util.h"
#include "vm.h"

static void fn_sound_create(VMContext *c) {
	warning("Unimplemented fn_sound_create");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sound_destroy(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_destroy");
}

static void fn_sound_open(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_open");
}

static void fn_sound_play(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_play");
}

static void fn_sound_pause(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_pause");
}

static void fn_sound_resume(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_resume");
}

static void fn_sound_stop(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_stop");
}

static void fn_sound_status(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_status");
	VM_Push(c, 1 /* DONE */, VAR_TYPE_INT32);
}

static void fn_sound_set_volume(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_setVolume");
}

static void fn_sound_get_volume(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_getVolume");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_sound_set_pan(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_set_pan");
}

static void fn_sound_set_rate(VMContext *c) {
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_set_rate");
}

static void fn_sound_play_resource(VMContext *c) {
	const int asset = VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_playResource asset:%d", asset);
}

static void fn_sound_halt(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_halt");
}

static void fn_sound_stop_all(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_stopAll");
}

static void fn_sound_master_volume(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_masterVolume");
}

static void fn_sound_playing(VMContext *c) {
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_playing");
	VM_Push(c, 1 /* DONE */, VAR_TYPE_INT32);
}

static void fn_sound_flags(VMContext *c) {
	VM_PopInt32(c);
	const int mode = VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented fn_sound_flags");
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
