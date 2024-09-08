
#include <math.h>
#include "util.h"
#include "vm.h"

static void fn_math_sqrt(VMContext *c) {
	const float f = VM_PopFloat(c);
	VM_PushFloat(c, sqrt(f));
}

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

static void fn_math_point_in_rect(VMContext *c) {
	const int y2 = VM_PopInt32(c);
	const int x2 = VM_PopInt32(c);
	const int y1 = VM_PopInt32(c);
	const int x1 = VM_PopInt32(c);
	const int py = VM_PopInt32(c);
	const int px = VM_PopInt32(c);
	int inside = 0;
	if (px >= x1 && px <= x2 && py >= y1 && py <= y2) {
		inside = 1;
	}
	VM_Push(c, inside, VAR_TYPE_INT32);
}

static void fn_math_point_in_poly(VMContext *c) {
	const int array = VM_Pop(c, 0x10000 | VAR_TYPE_INT32);
	const int y = VM_PopInt32(c);
	const int x = VM_PopInt32(c);
	VMArray *vertices = VM_GetArrayFromHandle(c, array);
	const int count = (vertices->col_upper - vertices->col_lower + 1);
	assert((count & 1) == 0 && count >= 6);
	const int num_vertices = (count / 2);
	debug(DBG_SYSCALLS, "math:pointInPoly [%d,%d] vertices:%d", x, y, num_vertices);

	int inside = 0;
	int p1_x = READ_LE_UINT32(vertices->data);
	int p1_y = READ_LE_UINT32(vertices->data + 4);
	for (int i = 1; i <= num_vertices; ++i) {
		int p2_x = READ_LE_UINT32(vertices->data + (i % num_vertices) * 8);
		int p2_y = READ_LE_UINT32(vertices->data + (i % num_vertices) * 8 + 4);
		if (y > MIN(p1_y, p2_y) && y <= MAX(p1_y, p2_y)) {
			if (x <= MAX(p1_x, p2_x)) {
				if (p1_x == p2_x) {
					inside = !inside;
				} else {
					const int intersection = (y - p1_y) * (p2_x - p1_x) / (p2_y - p1_y) + p1_x;
					if (x <= intersection) {
						inside = !inside;
					}
				}
			}
		}
		p1_x = p2_x;
		p1_y = p2_y;
	}
	VM_Push(c, inside, VAR_TYPE_INT32);
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
	{ 110009, fn_math_sqrt },
	{ 110010, fn_math_sin },
	{ 110011, fn_math_cos },
	{ 110012, fn_math_tan },
	{ 110015, fn_math_point_in_rect },
	{ 110016, fn_math_point_in_poly },
	{ 110020, fn_math_asin },
	{ 110021, fn_math_acos },
	{ 110022, fn_math_atan },
	{ -1, 0 }
};
