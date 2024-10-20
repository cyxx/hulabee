
// [Debug] debugger=1 whisk=1

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

static const char *LOG_FILENAME = "whisk.log";

static const bool TRACE_OPCODES = true;

static const bool DUMP_AFTER_BOOT = true;

static FILE *_out;

static uintptr_t _hostdbg_ptr;

static uint32_t _current_frame;

// host debug interface (Sauce Interpreter)
struct hostdbg_vtbl {
	void *f00_get_class_name;
	void (*__thiscall f04_load_class)(uintptr_t, const char *);
	void *f08_get_object;
	void (*__thiscall f0c_dump_current_thread)(uintptr_t);
	void (*__thiscall f10_dump_class)(uintptr_t, uint32_t); // base:2000000
	void (*__thiscall f14_dump_object)(uintptr_t, uint32_t); // base:3000000
	void (*__thiscall f18_dump_array)(uintptr_t, uint32_t); // base:4000000
	void (*__thiscall f1c_dump_stack)(uintptr_t);
	void (*__thiscall f20_dump_thread)(uintptr_t, uint8_t); // base:5000000
	void (*__thiscall f24_dump_string)(uintptr_t, uint32_t);
	void (*__thiscall f28_log_asset_load)(uintptr_t, uint32_t);
	void *f2c;
	void *f30;
	void *f34;
	void *f38;
	void *f3c;
	void *f40;
	void *f44;
	void *f48;
	void *f4c;
	void *f50;
	void *f54_read_breakpoints;
	void *f58_write_breakpoints;
	void *f5c; // same as f4c
	void *f60;
	void (*__thiscall f64_break_tiner_frame)(uintptr_t);
	void (*__thiscall f68_dump)(uintptr_t); // calls whisk.f20
	void *f6c;
	void (*__thiscall f70_gc)(uintptr_t, uint32_t);
	void *f74;
	void *f78_system_error; // calls whisk.f00
	void *f7c_error; // calls whisk.f00
	void *f80_warning; // calls whisk.f00
	void *f84_log0; // calls whisk.f00
	void *f88_log1; // calls whisk.f00
	void *f8c_log2; // calls whisk.f00
	void *f90_format_string;
	void (*__thiscall f94_breakpoint)(uintptr_t); // calls whisk.f14
	void (*__thiscall f98_cmd)(uintptr_t, const char *);
	void (*__thiscall f9c_stack_frame)(uintptr_t); // calls whisk.f0c
	void *fa0;
	void (*__thiscall fa4_get_counter)(uintptr_t); // log if returns 96
	void (*__thiscall fa8_set_module_info)(uintptr_t, const char *, const char *, uintptr_t); // calls whisk.f20
};

#define HOSTDBG_LOG_THREAD            0x1
#define HOSTDBG_BREAKPOINT_OPCODE     0x4
#define HOSTDBG_BREAKPOINT_FRAME      0x8
#define HOSTDBG_BREAKPOINT_ADDR      0x10
#define HOSTDBG_GARBAGE_COLLECT      0x80
#define HOSTDBG_LOG_ARRAYS          0x100

struct hostdbg_members {
	uintptr_t *vtable;
	uint32_t mask;
	uint32_t break_addr;
	uint32_t break_timer_frame;
	uint32_t log_asset_loading;
};

static void hostdbg_dump() {
	uintptr_t vtbl = *(const uintptr_t *)_hostdbg_ptr;
	(((struct hostdbg_vtbl *)vtbl)->f20_dump_thread)(_hostdbg_ptr, 1);
	(((struct hostdbg_vtbl *)vtbl)->f0c_dump_current_thread)(_hostdbg_ptr);
	(((struct hostdbg_vtbl *)vtbl)->f1c_dump_stack)(_hostdbg_ptr);
	(((struct hostdbg_vtbl *)vtbl)->f10_dump_class)(_hostdbg_ptr, -1);
	(((struct hostdbg_vtbl *)vtbl)->f14_dump_object)(_hostdbg_ptr, -1);
	(((struct hostdbg_vtbl *)vtbl)->f18_dump_array)(_hostdbg_ptr, -1);
}

static void hostdbg_cmd(const char *s) {
	uintptr_t vtbl = *(const uintptr_t *)_hostdbg_ptr;
	(((struct hostdbg_vtbl *)vtbl)->f98_cmd)(_hostdbg_ptr, s);
}

static void hostdbg_init() {
	uintptr_t vtbl = *(const uintptr_t *)_hostdbg_ptr;
	(((struct hostdbg_vtbl *)vtbl)->f28_log_asset_load)(_hostdbg_ptr, 1);
	if (TRACE_OPCODES) {
		// enable opcodes tracing
		hostdbg_cmd("opcodes");
		hostdbg_cmd("trace");
	}
}

static __stdcall void f00_log(void *debug_obj, uint32_t mask, const char *s, ...) {
	char buf[1024];
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	fprintf(_out, "%s", buf);
}

static __stdcall void f04_init(uintptr_t hostdbg_ptr) {
	_hostdbg_ptr = hostdbg_ptr;
	// check_whisk_ptr(hostdbg_ptr);
	hostdbg_init();
//	((struct hostdbg_members *)hostdbg_ptr)->mask = 0x1FF;
}

static __stdcall void f08_boot(uint32_t code) {
	// called if boot()V returns != 0
	assert(code == 2000001);
	if (DUMP_AFTER_BOOT) {
		hostdbg_dump();
	}
}

static __stdcall void f0c_stack_frame(int a, int b) {
	// fprintf(stderr, "WARNING: f0c %x %x\n", a, b);
}

static __stdcall void f10_fini() {
}

static __stdcall void f14_breakpoint() {
	if (TRACE_OPCODES) {
		// step to next opcode
		hostdbg_cmd("step");
	}
}

static __stdcall void f18() {
	fprintf(stderr, "WARNING: f18\n");
}

static __stdcall void f20(const char *a, const char *b, const char *(*__stdcall f)(uint32_t num, uint32_t type)) {
	fprintf(stderr, "WARNING: f20 %s, %s, sub_%x\n", a, b, f);
}

static __stdcall void f28_frame() {
	++_current_frame;
}

static __stdcall void f30(uint32_t a) {
	fprintf(stderr, "WARNING: f30\n");
}

static __stdcall void f_stub() {
	fprintf(stderr, "WARNING: f_stub\n");
}

// client debug interface (Whisk)
static void *funcs_tbl[] = {
	f00_log,
	f04_init,
	f08_boot,
	f0c_stack_frame,
	f10_fini,
	f14_breakpoint,
	f18,
	0, // f1c
	f20,
	0, // f24
	f28_frame,
	0, // f2c
	f30,
};

static struct {
	void *funcs;
} _whisk = {
	funcs_tbl
};

static void check_whisk_ptr(uintptr_t ptr) {
	const uintptr_t whisk = *(const uintptr_t *)(ptr + 0x6C);
	assert(whisk == (uintptr_t)&_whisk);
}

__declspec(dllexport) void *WhiskPreInit() {
	_out = fopen(LOG_FILENAME, "w");
	if (!_out) {
		_out = stdout;
	}
	return &_whisk;
}
