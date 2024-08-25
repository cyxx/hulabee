
#include "random.h"
#include "util.h"
#include "vm.h"

static void op_nop(VMContext *) {
}

static void op_breakhere(VMContext *c) {
	debug(DBG_OPCODES, "op_breakhere");
	c->script->state = 5;
}

static void op_jump(VMContext *c) {
	debug(DBG_OPCODES, "op_jump");
	const int32_t pos = READ_LE_UINT32(c->code);
	c->code += pos - 1;
	c->script->code_offset += pos - 1;

	const SobData *sob = c->script->sob_data;
	const uint8_t *start = sob->code_data;
	const uint8_t *end = start + sob->code_size;
	if (c->code < start || c->code >= end) {
		error("Code ptr %p out of range (%p..%p)", c->code, start, end);
	}
}

static void op_return(VMContext *c) {
	debug(DBG_OPCODES, "op_return");
	c->script->state = 4;
}

static void op_push_int8(VMContext *c) {
	const uint8_t value = *c->code++;
	debug(DBG_OPCODES, "op_push_int8 value:%d", value);
	c->script->code_offset += 1;
	VM_Push(c, value, VAR_TYPE_INT32);
}

static void op_push_int32(VMContext *c) {
	const int value = READ_LE_UINT32(c->code); c->code += 4;
	debug(DBG_OPCODES, "op_push_int32 value:%d", value);
	c->script->code_offset += 4;
	VM_Push(c, value, VAR_TYPE_INT32);
}

static void op_push_me(VMContext *c) {
	VMObject *obj = c->script->obj;
	if (!obj) {
		error("push_me called from static method");
	}
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_me num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	VMVar *var = VM_GetObjectMemberVar(c, obj, num & 0xFFFF);
	for (int i = 0; i < count; ++i, ++var) {
		VM_Push(c, var->value, var->type);
	}
}

static void op_push_member(VMContext *c) {
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Accessing member variable from a NULL object");
	}
	VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
	if (!obj) {
		error("Object handle %d was deleted", obj_handle);
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_member num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	VMVar *var = VM_GetObjectMemberVar(c, obj, num & 0xFFFF);
	for (int i = 0; i < count; ++i, ++var) {
		VM_Push(c, var->value, var->type);
	}
}

static void op_push_local(VMContext *c) {
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_local num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	const VMVar *var = VM_GetLocalVar(c, num & 0xFFFF);
	for (int i = 0; i < count; ++i, ++var) {
		VM_Push(c, var->value, var->type);
	}
}

static void op_push_static(VMContext *c) {
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_static num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	const SobVar *var = VM_GetClassStaticVar(c, c->script->sob_data, num & 0xFFFF);
	for (int i = 0; i < count; ++i, ++var) {
		VM_Push(c, var->value, var->type);
	}
}

static void op_push_static_me(VMContext *c) {
	debug(DBG_OPCODES, "op_push_static_me");
	op_push_static(c);
}

static void op_pop(VMContext *c) {
	debug(DBG_OPCODES, "op_pop");
	VM_Pop2(c);
}

static void op_pop_local(VMContext *c) {
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_local num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	VMVar *var = VM_GetLocalVar(c, num);
	var += count - 1;
	for (int i = 0; i < count; ++i, --var) {
		VM_CheckVarType(var->type);
		VMVar st = VM_Pop2(c);
		VM_CheckVarType(st.type);
		var->value = VM_ConvertVar(var->type, &st);
	}
}

static void op_pop_me(VMContext *c) {
	VMObject *obj = c->script->obj;
	if (!obj) {
		error("pop_me called from static method");
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_me num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	VMVar *var = VM_GetObjectMemberVar(c, obj, num & 0xFFFF);
	var += count - 1;
	for (int i = 0; i < count; ++i, --var) {
		VMVar st2 = VM_Pop2(c);
		VM_CheckVarType(st2.type);
		VM_CheckVarType(var->type);
		var->value = VM_ConvertVar(var->type, &st2);
	}
}

static void op_pop_member(VMContext *c) {
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Accessing member variable from a NULL object");
	}
	VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
	if (!obj) {
		error("Object handle %d was deleted", obj_handle);
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_member num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	VMVar *var = VM_GetObjectMemberVar(c, obj, num & 0xFFFF);
	var += count - 1;
	for (int i = 0; i < count; ++i, --var) {
		VMVar *var = VM_GetObjectMemberVar(c, obj, num);
		VMVar st2 = VM_Pop2(c);
		VM_CheckVarType(st2.type);
		VM_CheckVarType(var->type);
		var->value = VM_ConvertVar(var->type, &st2);
	}
}

static void op_pop_static(VMContext *c) {
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_static num:%d", num);
	int count = 1;
	if (num & 0xFFFF0000) {
		assert((num & 0xFF000000) == 0);
		count = (num >> 16) & 0xFF;
	}
	SobVar *var = VM_GetClassStaticVar(c, c->script->sob_data, num & 0xFFFF);
	var += count - 1;
	for (int i = 0; i < count; ++i, --var) {
		VMVar st = VM_Pop2(c);
		VM_CheckVarType(var->type);
		VM_CheckVarType(st.type);
		var->value = VM_ConvertVar(var->type, &st);
	}
}

static void op_pop_static_me(VMContext *c) {
	debug(DBG_OPCODES, "op_pop_static_me");
	op_pop_static(c);
}

static void op_call_me(VMContext *c) {
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_call_me num:%d", num);
	VM_InvokeMethod(c, c->script->sob_data, num, c->script->obj_handle, 0, 0, 0);
}

static void op_call_method(VMContext *c) {
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Calling method from NULL object");
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_call_method num:%d obj:%d", num, obj_handle);
	VM_InvokeMethod(c, c->script->sob_data, num, obj_handle, 0, 0, 0);
}

static void op_call_static(VMContext *c) {
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_call_static num:%d", num);
	VM_InvokeMethod(c, c->script->sob_data, num, 0, 0, 1, 0);
}

static void op_new(VMContext *c) {
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_new num:%d", num);
	SobData *sob = c->script->sob_data;
	SobRefEntry *ref = Sob_GetRefClass(sob, num);
	if (ref->class_handle == 0) {
		const char *name = Sob_GetString(sob, ref->name_index);
                ref->class_handle = VM_FindOrLoadClass(c, name, 1);
	}
	const int obj_handle = ObjectHandle_Create(c, ref->class_handle);
	VM_Push(c, obj_handle, VAR_TYPE_OBJECT);
}

static void op_add_int(VMContext *c) {
	debug(DBG_OPCODES, "op_add_int");
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = a.value + b.value;
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_sub_int(VMContext *c) {
	debug(DBG_OPCODES, "op_sub_int");
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = a.value - b.value;
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_mul_int(VMContext *c) {
	debug(DBG_OPCODES, "op_mul_int");
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = a.value * b.value;
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_div_int(VMContext *c) {
	debug(DBG_OPCODES, "op_div_int");
	VMVar b = VM_Pop2(c);
	int div = b.value;
	if (div == 0) {
		div = 1;
	}
	VMVar a = VM_Pop2(c);
	const int res = a.value / div;
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_neg_int(VMContext *c) {
	debug(DBG_OPCODES, "op_neg_int");
	VMVar st = VM_Pop2(c);
	if (st.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(st.type));
	}
	VM_Push(c, -st.value, st.type);
}

static void op_start_method(VMContext *c) {
	const int type = *c->code++;
	++c->script->code_offset;
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_start_method num:%d type:%d", num, type);
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Starting script from a NULL object");
	}
	if (type == 2) {
		const int ret = VM_InvokeMethod(c, c->script->sob_data, num, obj_handle, 2, 0, 0);
		VM_Push(c, ret, VAR_TYPE_INT32);
	} else {
		VM_InvokeMethod(c, c->script->sob_data, num, obj_handle, type, 0, 0);
	}
}

static void op_start_static(VMContext *c) {
	const int type = *c->code++;
	++c->script->code_offset;
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_start_static num:%d type:%d", num, type);
	if (type == 2) {
		const int ret = VM_InvokeMethod(c, c->script->sob_data, num, 0, 2, 1, 0);
		VM_Push(c, ret, VAR_TYPE_INT32);
	} else {
		VM_InvokeMethod(c, c->script->sob_data, num, 0, type, 1, 0);
	}
}

static void op_start_me(VMContext *c) {
	const int type = *c->code++;
	++c->script->code_offset;
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_start_me num:%d type:%d", num, type);
	if (type == 2) {
		const int ret = VM_InvokeMethod(c, c->script->sob_data, num, c->script->obj_handle, 2, 0, 0);
		VM_Push(c, ret, VAR_TYPE_INT32);
	} else {
		VM_InvokeMethod(c, c->script->sob_data, num, c->script->obj_handle, type, 0, 0);
	}
}

static void op_if_eq(VMContext *c) {
	VMVar st = VM_Pop2(c);
	debug(DBG_OPCODES, "op_if_eq");
	if (st.value != 0) {
		op_jump(c);
	} else {
		c->code += 4;
		c->script->code_offset += 4;
	}
}

static void op_if_neq(VMContext *c) {
	VMVar st = VM_Pop2(c);
	debug(DBG_OPCODES, "op_if_neq");
	if (st.value == 0) {
		op_jump(c);
	} else {
		c->code += 4;
		c->script->code_offset += 4;
	}
}

static void op_and(VMContext *c) {
	debug(DBG_OPCODES, "op_and");
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value != 0 && b.value != 0);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_or(VMContext *c) {
	debug(DBG_OPCODES, "op_or");
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value != 0 || b.value != 0);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_eq_int(VMContext *c) {
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value == b.value);
	debug(DBG_OPCODES, "op_eq_int");
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_neq_int(VMContext *c) {
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value != b.value);
	debug(DBG_OPCODES, "op_neq_int");
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_leq_int(VMContext *c) {
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value <= b.value);
	debug(DBG_OPCODES, "op_leq_int %d <= %d", a.value, b.value);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_geq_int(VMContext *c) {
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value >= b.value);
	debug(DBG_OPCODES, "op_geq_int %d >= %d", a.value, b.value);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_lt_int(VMContext *c) {
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value < b.value);
	debug(DBG_OPCODES, "op_lt_int %d < %d", a.value, b.value);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_gt_int(VMContext *c) {
	VMVar b = VM_Pop2(c);
	VMVar a = VM_Pop2(c);
	const int res = (a.value > b.value);
	debug(DBG_OPCODES, "op_gt_int %d > %d", a.value, b.value);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_push_local_array(VMContext *c) {
	VMVar st = VM_Pop2(c);
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_local_array num:%d", num);
	const VMVar *var = VM_GetLocalVar(c, num & 0xFFFF);
	int type = var->type & 0xFFFF;
	if (type & 0x100) {
		type &= ~0x100;
		type |= 0x10000;
	}
	VM_CheckVarType(type);
	if (num & 0xFFFF0000) {
		error("Unimplemented op_push_local_array num:0x%x", num);
	} else {
		if ((var->type & 0x10000) == 0) {
			error("Using array reference on non-array variable");
		} else {
			VMArray *array = VM_GetArrayFromHandle(c, var->value);
			VM_Push(c, Array_Get(array, st.value), type);
		}
	}
}

static void op_pop_static_array(VMContext *c) {
	VMVar st = VM_Pop2(c);
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_static_array num:%d", num);
	if (num & 0xFFFF0000) {
		error("Unimplemented op_pop_static_array num:0x%x", num);
	} else {
		SobVar *var = VM_GetClassStaticVar(c, c->script->sob_data, num);
		VMVar st2 = VM_Pop2(c);
		VM_CheckVarType(var->type);
		VM_CheckVarType(st2.type);
		if ((var->type & 0x10000) == 0) {
			error("Using array reference on non-array variable");
		} else {
			VMArray *array = VM_GetArrayFromHandle(c, var->value);
			int type = var->type & 0xFFFF;
			if (type & 0x100) {
				type &= ~0x100;
				type |= 0x10000;
			}
			Array_Set(array, st.value, VM_ConvertVar(type, &st2));
		}
	}
}

static void op_pop_static_me_array(VMContext *c) {
	op_pop_static_array(c);
}

static void op_pop_me_array(VMContext *c) {
	VMObject *obj = c->script->obj;
	if (!obj) {
		error("pop_me_array called from static method");
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_me_array num:%d", num);

	VMVar st = VM_Pop2(c);

	if (num & 0xFFFF0000) {
		error("Unimplemented op_pop_me_array num:0x%x", num);
	} else {
		VMVar *var = VM_GetObjectMemberVar(c, obj, num);
		VMVar st2 = VM_Pop2(c);
		VM_CheckVarType(var->type);
		VM_CheckVarType(st2.type);
		if ((var->type & 0x10000) == 0) {
			error("Using array reference on non-array variable");
		} else {
			VMArray *array = VM_GetArrayFromHandle(c, var->value);
			int type = var->type & 0xFFFF;
			if (type & 0x100) {
				type &= ~0x100;
				type |= 0x10000;
			}
			Array_Set(array, st.value, VM_ConvertVar(type, &st2));
		}
	}
}

static void op_push_me1(VMContext *c) {
	VMObject *obj = c->script->obj;
	if (!obj) {
		error("push_me1 called from static method");
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_me1 num:%d", num);

	if (num & 0xFFFF0000) {
		error("Unimplemented op_push_me1 num:0x%x", num);
	} else {
		VMVar *var = VM_GetObjectMemberVar(c, obj, num);
		const int x = VM_PopInt32(c);
		int type = var->type & 0xFFFF;
		if (type & 0x100) {
			type &= ~0x100;
			type |= 0x10000;
		}
		VM_CheckVarType(var->type);
		if ((var->type & 0x10000) == 0) {
			error("Using array reference on non-array variable");
		} else {
			VMArray *array = VM_GetArrayFromHandle(c, var->value);
			const int value = Array_Get(array, x);
			VM_Push(c, value, type);
		}
	}
}

static void op_push_static_array(VMContext *c) {
	int x = VM_PopInt32(c);

	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_static_array num:%d", num);

	if (num & 0xFFFF0000) {
		error("Unimplemented op_push_static_array num:0x%x", num);
	} else {
		const SobVar *var = VM_GetClassStaticVar(c, c->script->sob_data, num);
		int type = var->type & 0xFFFF;
		if (type & 0x100) {
			type &= ~0x100;
			type |= 0x10000;
		}
		VM_CheckVarType(var->type);
		if ((var->type & 0x10000) == 0) {
			error("Using array reference on non-array variable");
		} else {
			VMArray *array = VM_GetArrayFromHandle(c, var->value);
			const int value = Array_Get(array, x);
			VM_Push(c, value, type);
		}
	}
}

static void op_pop_local_array(VMContext *c) {
	VMVar st1 = VM_Pop2(c);

	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_pop_local_array num:%d", num);

	if (num & 0xFFFF0000) {
		error("Unimplemented op_pop_local_array num:0x%x", num);
	} else {
		const VMVar *var = VM_GetLocalVar(c, num);
		VMVar st2 = VM_Pop2(c);
		VM_CheckVarType(var->type);
		VM_CheckVarType(st2.type);
		if ((var->type & 0x10000) == 0) {
			error("Using array reference on non-array variable");
		} else {
			VMArray *array = VM_GetArrayFromHandle(c, var->value);
			int type = var->type & 0xFFFF;
			if (type & 0x100) {
				type &= ~0x100;
				type |= 0x10000;
			}
			Array_Set(array, st1.value, VM_ConvertVar(type, &st2));
		}
	}
}

static void op_push_static_me_array(VMContext *c) {
	debug(DBG_OPCODES, "op_push_static_me_array");
	op_push_static_array(c);
}

static void op_push_string(VMContext *c) {
	const int32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_string num:%d", num);
	const char *str = Sob_GetString(c->script->sob_data, num);
	VMArray *array = Array_New(c);
	Array_SetString(array, str);
	VM_Push(c, array->handle, 0x10000 | VAR_TYPE_CHAR);
}

static void op_add_str(VMContext *c) {
	VMVar st2 = VM_Pop2(c);
	VMVar st1 = VM_Pop2(c);
	if (st1.type != 0x10005 && st1.value != 0 && (st1.type & 0xFF) != 12 && (st1.type & 0xFF) != 10) {
		error("Can't convert from %s to %s", VM_GetVarTypeName(st1.type), VM_GetVarTypeName(0x10005));
	}
	if (st2.type != 0x10005 && st2.value != 0 && (st2.type & 0xFF) != 12 && (st2.type & 0xFF) != 10) {
		error("Can't convert from %s to %s", VM_GetVarTypeName(st2.type), VM_GetVarTypeName(0x10005));
	}
	int array = ArrayHandle_AddString(c, st1.value, st2.value);
	VM_Push(c, array, 0x10000 | VAR_TYPE_CHAR);
}

static void op_syscall(VMContext *c) {
	const int32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_syscall num:%d", num);
	const int x = VM_FindSyscallIndex(c, num);
	assert(x != -1);
	VM_ExecuteSyscallByIndex(c, x);
	uint8_t *code = (uint8_t *)c->code;
	code[-5] = 0xB6; /* op_fast_syscall */
	WRITE_LE_UINT32(code - 4, x);
}

static void op_fsyscall(VMContext *c) {
	const int32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_fsyscall num:%d", num);
	const int x = VM_FindSyscallIndex(c, num);
	assert(x != -1);
	VM_ExecuteSyscallByIndex(c, x);
	uint8_t *code = (uint8_t *)c->code;
	code[-5] = 0xB7; /* op_fast_fsyscall */
	WRITE_LE_UINT32(code - 4, x);
}

static void op_dim(VMContext *c) {
	debug(DBG_OPCODES, "op_dim");
	VMVar st = VM_Pop2(c);
	if (st.type & 0x10000) {
		VMVar st2 = VM_Pop2(c);
		VMArray *array = Array_New(c);
		Array_Dim(array, st.type & 0xFF0FFFF, 1, st2.value);
		VM_Push(c, array->handle, st.type);
	} else {
		error("Can't dim a non-array variable %s", VM_GetVarTypeName(st.type));
	}
}

static void op_quit(VMContext *c) {
	debug(DBG_OPCODES, "op_quit");
	int ret = 0;
	if (c->gameID >= GID_MOOP) {
		VM_Pop(c, VAR_TYPE_INT32);
	}
	exit(ret);
}

static void op_col_lower(VMContext *c) {
	debug(DBG_OPCODES, "op_col_lower");
	VMVar v0 = VM_Pop2(c);
	if ((v0.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(v0.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, v0.value);
	VM_Push(c, array->col_lower, VAR_TYPE_INT32);
}

static void op_col_upper(VMContext *c) {
	debug(DBG_OPCODES, "op_col_upper");
	VMVar v0 = VM_Pop2(c);
	if ((v0.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(v0.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, v0.value);
	VM_Push(c, array->col_upper, VAR_TYPE_INT32);
}

static void op_col_size(VMContext *c) {
	debug(DBG_OPCODES, "op_col_size");
	VMVar v0 = VM_Pop2(c);
	if ((v0.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(v0.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, v0.value);
	VM_Push(c, array->col_upper - array->col_lower + 1, VAR_TYPE_INT32);
}

static void op_mod(VMContext *c) {
	debug(DBG_OPCODES, "op_mod");
	VMVar v0 = VM_Pop2(c);
	int div = v0.value;
	if (div == 0) {
		div = 1;
	}
	VMVar v1 = VM_Pop2(c);
	const int res = v1.value % div;
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_rand_int(VMContext *c) {
	debug(DBG_OPCODES, "op_rand_int");
	VMVar v0 = VM_Pop2(c);
	VMVar v1 = VM_Pop2(c);
	if (v0.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(v0.type));
	}
	if (v1.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(v1.type));
	}
	int r;
	if (v1.value > v0.value) {
		r = GetRandomNumber(v0.value, v1.value);
	} else {
		r = GetRandomNumber(v1.value, v0.value);
	}
	VM_Push(c, r, v1.type);
}

static void op_strlen(VMContext *c) {
	debug(DBG_OPCODES, "op_strlen");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	const int len = Array_GetStringLength(array);
	VM_Push(c, len, VAR_TYPE_INT32);
}

static void op_not_int(VMContext *c) {
	debug(DBG_OPCODES, "op_not_int");
	VMVar st = VM_Pop2(c);
	VM_Push(c, st.value == 0, VAR_TYPE_INT32);
}

static void op_copy1(VMContext *c) {
	debug(DBG_OPCODES, "op_copy1");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	const int handle = Array_Copy1(c, array);
	VM_Push(c, handle, 0x10000 | array->type);
}

static void op_range1(VMContext *c) {
	debug(DBG_OPCODES, "op_range1");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VMVar end = VM_Pop2(c);
	VMVar start = VM_Pop2(c);
	const int handle = Array_Range1(c, array, start.value, end.value);
	VM_Push(c, handle, 0x10000 | array->type);
}

static void op_swap(VMContext *c) {
	debug(DBG_OPCODES, "op_swap");
	VMVar a = VM_Pop2(c);
	VMVar b = VM_Pop2(c);
	VM_Push(c, a.value, a.type);
	VM_Push(c, b.value, b.type);
}

static void op_dim2(VMContext *c) {
	debug(DBG_OPCODES, "op_dim2");
	VMVar st1 = VM_Pop2(c);
	if ((st1.type & 0x10000) == 0) {
		error("Can't dim a non-array variable %s", VM_GetVarTypeName(st1.type));
	}
	int st2 = VM_Pop(c, VAR_TYPE_INT32);
	int st3 = VM_Pop(c, VAR_TYPE_INT32);
	VMArray *array = Array_New(c);
	Array_Dim2(array, st1.type & 0xFFFF, 1, st3, 1, st2);
	VM_Push(c, array->handle, st1.type);
}

static void op_row_lower(VMContext *c) {
	debug(DBG_OPCODES, "op_row_lower");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VM_Push(c, array->row_lower, VAR_TYPE_INT32);
}

static void op_row_upper(VMContext *c) {
	debug(DBG_OPCODES, "op_row_upper");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VM_Push(c, array->row_upper, VAR_TYPE_INT32);
}

static void op_row_size(VMContext *c) {
	debug(DBG_OPCODES, "op_row_size");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VM_Push(c, array->row_upper - array->row_lower + 1, VAR_TYPE_INT32);
}

static void op_poppush_array(VMContext *c) {
	int type = 7;
	if (c->gameID >= GID_MONSTERS) {
		type = READ_LE_UINT32(c->code); c->code += 4;
		c->script->code_offset += 4;
	}
	VMVar st = VM_Pop2(c);
	debug(DBG_OPCODES, "op_poppush_array %d", st.value);
	if (c->gameID >= GID_MONSTERS && st.value != 0 && (type & 0xFF) != 10) {
		type = VM_Top2(c).type;
	}
	VMArray *array = Array_New(c);
	Array_Dim(array, type, 1, st.value);
	for (int i = st.value; i >= 1; --i) {
		VMVar st2 = VM_Pop2(c);
		assert(array->struct_size == 0);
		Array_Set(array, i, st2.value);
	}
	VM_Push(c, array->handle, 0x10000 | 12);
}

static void op_band(VMContext *c) {
	debug(DBG_OPCODES, "op_band");
	VMVar b = VM_Pop2(c);
	if (b.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(b.type));
	}
	VMVar a = VM_Pop2(c);
	if (a.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(a.type));
	}
	VM_Push(c, a.value & b.value, a.type);
}

static void op_bor(VMContext *c) {
	debug(DBG_OPCODES, "op_bor");
	VMVar b = VM_Pop2(c);
	if (b.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(b.type));
	}
	VMVar a = VM_Pop2(c);
	if (a.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(a.type));
	}
	VM_Push(c, a.value | b.value, a.type);
}


static void op_stop(VMContext *c) {
	debug(DBG_OPCODES, "op_stop");
	VMVar st = VM_Pop2(c);
	VMThread *thread = c->script->thread;
	VM_StopThread(c, st.value, thread->handle);
}

static void op_stop_me(VMContext *c) {
	debug(DBG_OPCODES, "op_stop_me");
	VMThread *thread = c->script->thread;
	thread->state = 3;
	c->script->state = 4;
}

static void op_running(VMContext *c) {
	debug(DBG_OPCODES, "op_running");
	VMVar st = VM_Pop2(c);
	const int count = VM_CountThreads(c, st.value);
	VM_Push(c, count, VAR_TYPE_INT32);
}

static void op_threadid(VMContext *c) {
	debug(DBG_OPCODES, "op_threadid");
	VMVar st = VM_Pop2(c);
	if (st.value == -1) {
		VM_Push(c, c->script->thread->id, VAR_TYPE_INT32);
	} else {
		const int id = ThreadHandle_FindId(c, st.value);
		VM_Push(c, id, VAR_TYPE_INT32);
	}
}

static void op_min_int(VMContext *c) {
	debug(DBG_OPCODES, "op_min_int");
	VMVar b = VM_Pop2(c);
	if (b.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(b.type));
	}
	VMVar a = VM_Pop2(c);
	if (a.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(a.type));
	}
	VM_Push(c, a.value < b.value ? a.value : b.value, a.type);
}

static void op_max_int(VMContext *c) {
	debug(DBG_OPCODES, "op_max_int");
	VMVar b = VM_Pop2(c);
	if (b.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(b.type));
	}
	VMVar a = VM_Pop2(c);
	if (a.type > VAR_TYPE_INT32) {
		error("Can't perform computations on %s variables", VM_GetVarTypeName(a.type));
	}
	VM_Push(c, a.value > b.value ? a.value : b.value, a.type);
}

static void op_dup(VMContext *c) {
	debug(DBG_OPCODES, "op_dup");
	VMVar a = VM_Pop2(c);
	VM_Push(c, a.value, a.type);
	VM_Push(c, a.value, a.type);
}

static void op_streq(VMContext *c) {
	VMVar st2 = VM_Pop2(c);
	VMVar st1 = VM_Pop2(c);
	if (st1.type != (0x10000 | VAR_TYPE_CHAR) && st1.value != 0 && (st1.type & 0xFF) != 12 && (st1.type & 0xFF) != 10) {
		error("Can't convert from %s to %s", VM_GetVarTypeName(st1.type), VM_GetVarTypeName(0x10000 | VAR_TYPE_CHAR));
	}
	if (st2.type != (0x10000 | VAR_TYPE_CHAR) && st2.value != 0 && (st2.type & 0xFF) != 12 && (st2.type & 0xFF) != 10) {
		error("Can't convert from %s to %s", VM_GetVarTypeName(st2.type), VM_GetVarTypeName(0x10000 | VAR_TYPE_CHAR));
	}
	const int res = ArrayHandle_CompareString(c, st1.value, st2.value);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_insert_upper(VMContext *c) {
	debug(DBG_OPCODES, "op_insert_upper");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	assert(array->struct_size == 0);
	VMVar st2 = VM_Pop2(c);
	Array_InsertUpper(array, st2.value);
}

static void op_delete_upper(VMContext *c) {
	debug(DBG_OPCODES, "op_delete_upper");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	warning("Unimplemented op_delete_upper");
	VM_Push(c, 0, array->type);
}

static void op_class_handle(VMContext *c) {
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_class_handle num:%d", num);
	SobData *sob = c->script->sob_data;
	SobRefEntry *ref = Sob_GetRefClass(sob, num);
	if (ref->class_handle == 0) {
		const char *name = Sob_GetString(sob, ref->name_index);
		ref->class_handle = VM_FindOrLoadClass(c, name, 0);
	}
	VM_Push(c, ref->class_handle, VAR_TYPE_INT32);
}

static void op_class_name(VMContext *c) {
	debug(DBG_OPCODES, "op_class_name");
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Accessing member variable from a NULL object");
	}
	VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
	if (!obj) {
		error("Object handle %d was deleted", obj_handle);
	}
	const char *name = ClassHandle_GetName(c, obj->class_handle);
	VMArray *array = Array_New(c);
	Array_SetString(array, name);
	VM_Push(c, array->handle, 0x10000 | VAR_TYPE_CHAR);
}

static void op_class_type(VMContext *c) {
	debug(DBG_OPCODES, "op_class_type");
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Accessing member variable from a NULL object");
	}
	VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
	if (!obj) {
		error("Object handle %d was deleted", obj_handle);
	}
	VM_Push(c, obj->class_handle, VAR_TYPE_INT32);
}

static void op_delete(VMContext *c) {
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	ObjectHandle_Delete(c, obj_handle, 1);
}

static void op_itof(VMContext *c) {
	debug(DBG_OPCODES, "op_itof");
	VMVar st = VM_Pop2(c);
	float f = st.value;
	VM_PushFloat(c, f);
}

static void op_ftoi(VMContext *c) {
	debug(DBG_OPCODES, "op_ftoi");
	const float f = VM_PopFloat(c);
	VM_Push(c, (int)f, VAR_TYPE_INT32);
}

static void op_itos(VMContext *c) {
	debug(DBG_OPCODES, "op_itos");
	VMVar st = VM_Pop2(c);
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%d", st.value);
	VMArray *array = Array_New(c);
	Array_SetString(array, buffer);
	VM_Push(c, array->handle, 0x10000 | VAR_TYPE_CHAR);
}

static void op_ftos(VMContext *c) {
	debug(DBG_OPCODES, "op_ftos");
	const float f = VM_PopFloat(c);
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%f", f);
	VMArray *array = Array_New(c);
	Array_SetString(array, buffer);
	VM_Push(c, array->handle, 0x10000 | VAR_TYPE_CHAR);
}

static void op_stoi(VMContext *c) {
	debug(DBG_OPCODES, "op_stoi");
	VMVar st = VM_Pop2(c);
	const char *p = ArrayHandle_GetString(c, st.value);
	VM_Push(c, p ? atoi(p) : 0, VAR_TYPE_INT32);
}

static void op_stof(VMContext *c) {
	debug(DBG_OPCODES, "op_stof");
	VMVar st = VM_Pop2(c);
	const char *p = ArrayHandle_GetString(c, st.value);
	const float f = p ? atof(p) : 0;
	VM_PushFloat(c, f);
}

static void op_add_float(VMContext *c) {
	debug(DBG_OPCODES, "op_add_float");
	float f2 = VM_PopFloat(c);
	float f1 = VM_PopFloat(c);
	f1 += f2;
	VM_PushFloat(c, f1);
}

static void op_sub_float(VMContext *c) {
	debug(DBG_OPCODES, "op_sub_float");
	float f2 = VM_PopFloat(c);
	float f1 = VM_PopFloat(c);
	f1 -= f2;
	VM_PushFloat(c, f1);
}

static void op_mul_float(VMContext *c) {
	debug(DBG_OPCODES, "op_mul_float");
	float f2 = VM_PopFloat(c);
	float f1 = VM_PopFloat(c);
	f1 *= f2;
	VM_PushFloat(c, f1);
}

static void op_div_float(VMContext *c) {
	debug(DBG_OPCODES, "op_div_float");
	float f2 = VM_PopFloat(c);
	if (f2 == 0.) {
		f2 = 1.;
	}
	float f1 = VM_PopFloat(c);
	f1 /= f2;
	VM_PushFloat(c, f1);
}

static void op_eq_float(VMContext *c) {
	debug(DBG_OPCODES, "op_eq_float");
	float f2 = VM_PopFloat(c);
	float f1 = VM_PopFloat(c);
	float res = (f1 == f2) ? 1 : 0;
	VM_PushFloat(c, res);
}

static void op_neq_float(VMContext *c) {
	debug(DBG_OPCODES, "op_neq_float");
	float f2 = VM_PopFloat(c);
	float f1 = VM_PopFloat(c);
	float res = (f1 != f2) ? 1 : 0;
	VM_PushFloat(c, res);
}

static void op_push_float(VMContext *c) {
	const int val = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	float f = *(const float *)&val;
	debug(DBG_OPCODES, "op_push_float %f", f);
	VM_PushFloat(c, f);
}

static void op_start_callback(VMContext *c) {
	debug(DBG_OPCODES, "op_start_callback");
	const int array = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const char *name = ArrayHandle_GetString(c, array);
	VMVar st = VM_Pop2(c);
	VM_StartCallback(c, st.value, name);
}

static void op_call_parent(VMContext *c) {
	const int obj_handle = VM_Pop(c, VAR_TYPE_OBJECT);
	if (obj_handle == 0) {
		error("Calling method from NULL object");
	}
	const int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_call_parent num:%d", num);
	VM_InvokeMethod(c, c->script->sob_data, num, obj_handle, 0, 0, 1);
}

static void op_new_expr(VMContext *c) {
	VMVar st = VM_Pop2(c);
	const int class_handle = st.value;
	debug(DBG_OPCODES, "op_new_expr class:%d", class_handle);
	const int obj_handle = ObjectHandle_Create(c, class_handle);
	SobData *sob = ClassHandle_GetSob(c, class_handle);
	const int num = Sob_FindMethod(sob, "_new_()V");
	if (num != 0) {
		VM_InvokeMethod(c, sob, num, obj_handle, 0, 0, 0);
	}
	VM_Push(c, obj_handle, VAR_TYPE_OBJECT);
}

static void op_array_find(VMContext *c) {
	debug(DBG_OPCODES, "op_array_find");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VMVar st2 = VM_Pop2(c);
	const int res = Array_Find(array, st2.value);
	VM_Push(c, res, VAR_TYPE_INT32);
}

static void op_breakmany(VMContext *c) {
	const int count = VM_PopInt32(c);
	debug(DBG_OPCODES, "op_breakmany count:%d", count);
	c->script->thread->break_counter = count;
	c->script->state = 5;
}

static void op_breaktime(VMContext *c) {
	const float delta = VM_PopFloat(c);
	debug(DBG_OPCODES, "op_breaktime delta:%f", delta);
	c->script->thread->break_time = 0;
	c->script->thread->break_counter = 0;
	c->script->state = 5;
	warning("Unimplemented op_breaktime");
}

static void op_delete_array(VMContext *c) {
	debug(DBG_OPCODES, "op_delete_array");
	VMVar st = VM_Pop2(c);
	ArrayHandle_Delete(c, st.value);
}

static void op_setthreadid(VMContext *c) {
	debug(DBG_OPCODES, "op_setthreadid");
	const int id = VM_PopInt32(c);
	c->script->thread->id = id;
}

static void op_setthreadorder(VMContext *c) {
	debug(DBG_OPCODES, "op_setthreadorder");
	const int order = VM_PopInt32(c);
	c->script->thread->order = order;
}

static void op_call_callback(VMContext *c) {
	debug(DBG_OPCODES, "op_call_callback");
	op_start_callback(c);
}

static void op_delete_index(VMContext *c) {
	debug(DBG_OPCODES, "op_delete_index");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VMVar st2 = VM_Pop2(c);
	const int ret = Array_DeleteIndex(array, st2.value, st2.value);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

static void op_check_index(VMContext *c) {
	debug(DBG_OPCODES, "op_check_index");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	VMVar st2 = VM_Pop2(c);
	const int ret = Array_CheckIndex(array, st2.value);
	VM_Push(c, ret, VAR_TYPE_INT32);
}

static void op_strcat(VMContext *c) {
	VMVar st2 = VM_Pop2(c);
	VMVar st1 = VM_Pop2(c);
	if (st1.type != 0x10005 && st1.value != 0 && (st1.type & 0xFF) != 12 && (st1.type & 0xFF) != 10) {
		error("Can't convert from %s to %s", VM_GetVarTypeName(st1.type), VM_GetVarTypeName(0x10005));
	}
	if (st2.type != 0x10005 && st2.value != 0 && (st2.type & 0xFF) != 12 && (st2.type & 0xFF) != 10) {
		error("Can't convert from %s to %s", VM_GetVarTypeName(st2.type), VM_GetVarTypeName(0x10005));
	}
	ArrayHandle_ConcatString(c, st1.value, st2.value);
}

static void op_assert(VMContext *c) {
	debug(DBG_OPCODES, "op_assert");
	VM_Pop2(c);
	VM_Pop2(c);
}

static void op_gotodefine(VMContext *c) {
	debug(DBG_OPCODES, "op_gotodefine");
	VMVar st = VM_Pop2(c);
	const uint32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	if (num == 0) {
		Thread_Define(c->script->thread, st.value, 0);
	} else {
		Thread_Define(c->script->thread, st.value, c->script->code_offset + num - 5);
	}
}

static void op_gotothread(VMContext *c) {
	debug(DBG_OPCODES, "op_gotothread");
	VMVar st2 = VM_Pop2(c);
	VMVar st1 = VM_Pop2(c);
	ThreadHandle_GoTo(c, st1.value, st2.value);
}

static void op_dim_int(VMContext *c) {
	debug(DBG_OPCODES, "op_dim_int");
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Can't dim a non-array variable %s", VM_GetVarTypeName(st.type));
	} else {
		VMArray *array = Array_New(c);
		Array_Dim(array, st.type & 0xFFFF, 1, 0);
		array->is_key_value = 1;
		VM_Push(c, array->handle, st.type);
	}
}

static void op_array_rand(VMContext *c) {
	VMVar st = VM_Pop2(c);
	if ((st.type & 0x10000) == 0) {
		error("Calling array operator on type %s", VM_GetVarTypeName(st.type));
	}
	VMArray *array = VM_GetArrayFromHandle(c, st.value);
	int value = Array_Rand(array);
	VM_Push(c, value, array->type);
}

static void op_fast_syscall(VMContext *c) {
	const int32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_fast_syscall num:%d", num);
	VM_ExecuteSyscallByIndex(c, num);
}

static void op_fast_fsyscall(VMContext *c) {
	const int32_t num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_fast_fsyscall num:%d", num);
	VM_ExecuteSyscallByIndex(c, num);
}

static void op_push_raw_local_array(VMContext *c) {
	VMVar st = VM_Pop2(c);
	int num = READ_LE_UINT32(c->code); c->code += 4;
	c->script->code_offset += 4;
	debug(DBG_OPCODES, "op_push_raw_local_array num:%d", num);
        num &= 0xFFFF;
	const VMVar *var = VM_GetLocalVar(c, num);
	int type = var->type & 0xFFFF;
	if (type & 0x100) {
		type &= ~0x100;
		type |= 0x10000;
	}
	VMArray *array = VM_GetArrayFromHandle(c, var->value);
	if (array->dimension == 2) {
		error("Accessing [n,n] as [n]");
	}
	const int value = Array_Get(array, st.value);
	VM_Push(c, value, type);
}

static void op_classname_handle(VMContext *c) {
	const char *name = VM_PopString(c);
	debug(DBG_OPCODES, "op_classname_handle name:'%s'", name);
	const int num = VM_FindOrLoadClass(c, name, 0);
	VM_Push(c, num, VAR_TYPE_INT32);
}

static void op_format_string(VMContext *c) {
	debug(DBG_OPCODES, "op_format_string");
	const int array_handle = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	const char *fmt = ArrayHandle_GetString(c, array_handle);
	const int count = VM_Pop(c, VAR_TYPE_INT32);
	assert(count <= 4);
	VMVar args[4];
	for (int i = 0; i < count; ++i) {
		args[i] = VM_Pop2(c);
	}
	char buffer[1024];
#define ARG(c, x) (args[x].type == (0x10000 | VAR_TYPE_CHAR) ? ArrayHandle_GetString(c, args[x].value) : args[x].value)
	switch (count) {
	case 1:
		snprintf(buffer, sizeof(buffer), fmt, ARG(c, 0));
		break;
	case 2:
		snprintf(buffer, sizeof(buffer), fmt, ARG(c, 1), ARG(c, 0));
		break;
	case 3:
		snprintf(buffer, sizeof(buffer), fmt, ARG(c, 2), ARG(c, 1), ARG(c, 0));
		break;
	default:
		error("Unhandled op_format_string args_count:%d", count);
		break;
	}
#undef ARG
	debug(DBG_OPCODES, "op_format_string s:'%s'", buffer);
	VMArray *array = Array_New(c);
	Array_SetString(array, buffer);
	VM_Push(c, array->handle, 0x10000 | VAR_TYPE_CHAR);
}

static void op_iftop_eq(VMContext *c) {
	debug(DBG_OPCODES, "op_iftop_eq");
	VMVar st = VM_Top2(c);
	if (st.value != 0) {
		op_jump(c);
	} else {
		c->code += 4;
		c->script->code_offset += 4;
	}
}

static void op_iftop_neq(VMContext *c) {
	debug(DBG_OPCODES, "op_iftop_neq");
	VMVar st = VM_Top2(c);
	if (st.value == 0) {
		op_jump(c);
	} else {
		c->code += 4;
		c->script->code_offset += 4;
	}
}

static void (*_opcodes[256])(VMContext *);

void VM_InitOpcodes() {
	for (int i = 0; i < 256; ++i) {
		_opcodes[i] = &op_nop;
	}
	_opcodes[0x01] = &op_breakhere;
	_opcodes[0x02] = &op_jump;
	_opcodes[0x04] = &op_return;
	_opcodes[0x05] = &op_return;
	_opcodes[0x06] = &op_push_int8;
	_opcodes[0x07] = &op_push_int32;
	_opcodes[0x08] = &op_push_local;
	_opcodes[0x09] = &op_push_me;
	_opcodes[0x0a] = &op_push_member;
	_opcodes[0x0b] = &op_push_static_me;
	_opcodes[0x0c] = &op_push_static;
	_opcodes[0x0d] = &op_pop;
	_opcodes[0x0e] = &op_pop_local;
	_opcodes[0x0f] = &op_pop_me;
	_opcodes[0x10] = &op_pop_member;
	_opcodes[0x11] = &op_pop_static_me;
	_opcodes[0x12] = &op_pop_static;
	_opcodes[0x13] = &op_call_me;
	_opcodes[0x14] = &op_call_method;
	_opcodes[0x15] = &op_call_static;
	_opcodes[0x17] = &op_new;
	_opcodes[0x18] = &op_add_int;
	_opcodes[0x19] = &op_sub_int;
	_opcodes[0x1a] = &op_mul_int;
	_opcodes[0x1b] = &op_div_int;
	_opcodes[0x1c] = &op_neg_int;
	_opcodes[0x1e] = &op_start_method;
	_opcodes[0x20] = &op_start_static;
	_opcodes[0x21] = &op_start_me;
	_opcodes[0x28] = &op_if_eq;
	_opcodes[0x29] = &op_if_neq;
	_opcodes[0x2a] = &op_and;
	_opcodes[0x2b] = &op_or;
	_opcodes[0x2c] = &op_eq_int;
	_opcodes[0x2d] = &op_neq_int;
	_opcodes[0x2e] = &op_leq_int;
	_opcodes[0x2f] = &op_geq_int;
	_opcodes[0x30] = &op_lt_int;
	_opcodes[0x31] = &op_gt_int;
	_opcodes[0x32] = &op_push_local_array;
	_opcodes[0x33] = &op_push_me1;
	_opcodes[0x35] = &op_push_static_me_array;
	_opcodes[0x36] = &op_push_static_array;
	_opcodes[0x37] = &op_pop_local_array;
	_opcodes[0x38] = &op_pop_me_array;
	_opcodes[0x3a] = &op_pop_static_me_array;
	_opcodes[0x3b] = &op_pop_static_array;
	_opcodes[0x3c] = &op_push_string;
	_opcodes[0x3d] = &op_add_str;
	_opcodes[0x3e] = &op_syscall;
	_opcodes[0x3f] = &op_fsyscall;
	_opcodes[0x40] = &op_dim;
	_opcodes[0x41] = &op_quit;
	_opcodes[0x42] = &op_col_lower;
	_opcodes[0x43] = &op_col_upper;
	_opcodes[0x44] = &op_col_size;
	_opcodes[0x45] = &op_mod;
	_opcodes[0x46] = &op_rand_int;
	_opcodes[0x47] = &op_strlen;
	_opcodes[0x48] = &op_not_int;
	_opcodes[0x4a] = &op_copy1;
	_opcodes[0x4b] = &op_range1;
	_opcodes[0x4c] = &op_swap;
	_opcodes[0x4d] = &op_dim2;
	// _opcodes[0x55] = &op_pop_local_array2;
	_opcodes[0x5b] = &op_row_lower;
	_opcodes[0x5c] = &op_row_upper;
	_opcodes[0x5d] = &op_row_size;
	_opcodes[0x61] = &op_poppush_array;
	_opcodes[0x62] = &op_band;
	_opcodes[0x63] = &op_bor;
	_opcodes[0x64] = &op_stop;
	_opcodes[0x65] = &op_stop_me;
	_opcodes[0x66] = &op_running;
	_opcodes[0x67] = &op_threadid;
	_opcodes[0x68] = &op_min_int;
	_opcodes[0x69] = &op_max_int;
	_opcodes[0x6c] = &op_dup;
	_opcodes[0x6d] = &op_streq;
	_opcodes[0x70] = &op_insert_upper;
	_opcodes[0x72] = &op_delete_upper;
	_opcodes[0x73] = &op_class_handle;
	_opcodes[0x74] = &op_class_name;
	_opcodes[0x75] = &op_class_type;
	_opcodes[0x76] = &op_delete;
	_opcodes[0x77] = &op_itof;
	_opcodes[0x78] = &op_ftoi;
	_opcodes[0x79] = &op_itos;
	_opcodes[0x7a] = &op_stoi;
	_opcodes[0x7b] = &op_ftos;
	_opcodes[0x7c] = &op_stof;
	_opcodes[0x7d] = &op_add_float;
	_opcodes[0x7e] = &op_sub_float;
	_opcodes[0x7f] = &op_mul_float;
	_opcodes[0x80] = &op_div_float;
	_opcodes[0x86] = &op_eq_float;
	_opcodes[0x87] = &op_neq_float;
	_opcodes[0x8c] = &op_push_float;
	_opcodes[0x8e] = &op_start_callback;
	_opcodes[0x8f] = &op_call_parent;
	_opcodes[0x91] = &op_new_expr;
	_opcodes[0x92] = &op_array_find;
	_opcodes[0x93] = &op_breakmany;
	_opcodes[0x94] = &op_breaktime;
	_opcodes[0x95] = &op_setthreadid;
	_opcodes[0x96] = &op_setthreadorder;
	_opcodes[0x97] = &op_call_callback;
	_opcodes[0xa5] = &op_delete_index;
	_opcodes[0xab] = &op_check_index;
	_opcodes[0xad] = &op_delete_array;
	_opcodes[0xae] = &op_strcat;
	_opcodes[0xaf] = &op_assert;
	_opcodes[0xb0] = &op_gotodefine;
	_opcodes[0xb1] = &op_gotothread;
	_opcodes[0xb2] = &op_dim_int;
	_opcodes[0xb5] = &op_array_rand;
	_opcodes[0xb6] = &op_fast_syscall;
	_opcodes[0xb7] = &op_fast_fsyscall;
	_opcodes[0xb8] = &op_push_raw_local_array;
	_opcodes[0xb9] = &op_classname_handle;
	_opcodes[0xba] = &op_format_string;
	_opcodes[0xbc] = &op_iftop_eq;
	_opcodes[0xbd] = &op_iftop_neq;
}

void VM_ExecuteOpcode(VMContext *c, int op) {
	debug(DBG_OPCODES, "VM_ExecuteOpcode op:0x%02x", op);
	if (_opcodes[op] == &op_nop) {
		error("Unimplemented opcode 0x%02x", op);
	} else {
		(*_opcodes[op])(c);
	}
}
