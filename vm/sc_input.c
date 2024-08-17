
#include "host_sdl2.h"
#include "util.h"
#include "vm.h"

static int _cursorCurrentHandle = 1;

static void fn_input_new_cursor(VMContext *c) {
	const int image_num = VM_PopInt32(c);
	int b = VM_PopInt32(c);
	int a = VM_PopInt32(c);
	warning("Unimplemented fn_input_new_cursor %d %d image:%d", a, b, image_num);
	VM_Push(c, _cursorCurrentHandle, VAR_TYPE_INT32);
	++_cursorCurrentHandle;
}

static void fn_input_set_cursor(VMContext *c) {
	const int cursor_num = VM_PopInt32(c);
	warning("Unimplemented fn_input_set_cursor cursor:%d", cursor_num);
}

static void fn_input_get_shift_key(VMContext *c) {
	const int state = (SDL_GetModState() & KMOD_SHIFT) != 0;
	VM_Push(c, state, VAR_TYPE_INT32);
}

static void fn_input_get_ctrl_key(VMContext *c) {
	const int state = (SDL_GetModState() & KMOD_CTRL) != 0;
	VM_Push(c, state, VAR_TYPE_INT32);
}

static void fn_input_get_key_state(VMContext *c) {
	const int code = VM_PopInt32(c);
	warning("Unimplemented fn_input_get_key_state %d", code);
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_input_get_cursor_x(VMContext *c) {
	int x;
	SDL_GetMouseState(&x, 0);
	debug(DBG_SYSCALLS, "Input:getCursorX x:%d", x);
	VM_Push(c, x, VAR_TYPE_INT32);
}

static void fn_input_get_cursor_y(VMContext *c) {
	int y;
	SDL_GetMouseState(0, &y);
	debug(DBG_SYSCALLS, "Input:getCursorY y:%d", y);
	VM_Push(c, y, VAR_TYPE_INT32);
}

static void fn_input_get_left_button(VMContext *c) {
	const int buttons = SDL_GetMouseState(0, 0);
	debug(DBG_SYSCALLS, "Input:leftButtons buttons:0x%x", buttons);
	VM_Push(c, (buttons & SDL_BUTTON_LEFT) != 0, VAR_TYPE_INT32);
}

static void fn_input_get_right_button(VMContext *c) {
	const int buttons = SDL_GetMouseState(0, 0);
	debug(DBG_SYSCALLS, "Input:rightButtons buttons:0x%x", buttons);
	VM_Push(c, (buttons & SDL_BUTTON_RIGHT) != 0, VAR_TYPE_INT32);
}

static void fn_input_get_left_click(VMContext *c) {
	debug(DBG_SYSCALLS, "Input:leftClick");
	VM_Push(c, Host_GetLeftClick(), VAR_TYPE_INT32);
}

static void fn_input_get_right_click(VMContext *c) {
	debug(DBG_SYSCALLS, "Input:rightClick()");
	VM_Push(c, Host_GetRightClick(), VAR_TYPE_INT32);
}

static void fn_input_show_cursor(VMContext *c) {
	debug(DBG_SYSCALLS, "Input:showCursor");
	SDL_ShowCursor(1);
}

static void fn_input_hide_cursor(VMContext *c) {
	debug(DBG_SYSCALLS, "Input:hideCursor");
	SDL_ShowCursor(0);
}

const VMSyscall _syscalls_input[] = {
	{ 40001, fn_input_new_cursor },
	{ 40003, fn_input_set_cursor },
	{ 40004, fn_input_get_shift_key },
	{ 40005, fn_input_get_ctrl_key },
	{ 40007, fn_input_get_key_state },
	{ 40008, fn_input_get_cursor_x },
	{ 40009, fn_input_get_cursor_y },
	{ 40010, fn_input_get_left_button },
	{ 40011, fn_input_get_right_button },
	{ 40012, fn_input_get_left_click },
	{ 40013, fn_input_get_right_click },
	{ 40014, fn_input_show_cursor },
	{ 40015, fn_input_hide_cursor },
	{ -1, 0 }
};
