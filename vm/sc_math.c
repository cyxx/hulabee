
#include <math.h>
#include "util.h"
#include "vm.h"

static void fn_math_sin(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, sin(f));
}

static void fn_math_cos(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, cos(f));
}

static void fn_math_tan(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, tan(f));
}

static void fn_math_point_in_poly(VMContext *c) {
	VM_Pop(c, 0x10000 | VAR_TYPE_INT32);
	VM_PopInt32(c);
	VM_PopInt32(c);
	warning("Unimplemented math:pointInPoly");
	VM_Push(c, 0, VAR_TYPE_INT32);
}

static void fn_math_asin(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, asin(f));
}

static void fn_math_acos(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, acos(f));
}

static void fn_math_atan(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, atan(f));
}

const VMSyscall _syscalls_math[] = {
	{ 110010, fn_math_sin },
	{ 110011, fn_math_cos },
	{ 110012, fn_math_tan },
	{ 110016, fn_math_point_in_poly },
	{ 110020, fn_math_asin },
	{ 110021, fn_math_acos },
	{ 110022, fn_math_atan },
	{ -1, 0 }
};
