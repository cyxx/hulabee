
#include "util.h"
#include "vm.h"


static void compareVarType(int type1, int type2) {
	if (type1 == type2 || type1 == 12 || type2 == 12) {
		return;
	}
	error("Popped wrong type %s, expected %s", VM_GetVarTypeName(type1), VM_GetVarTypeName(type2));
}

int VM_Pop(VMContext *c, int expectedType) {
	--c->sp;
	if (c->sp < 0) {
		error("Stack underflow");
	}
	debug(DBG_STACK, "pop sp:0x%lx", c->sp);
	const VMVar *var = &c->stack[c->sp];
	const int value = var->value;
	if (value != 0) {
		compareVarType(var->type, expectedType);
	}
	return value;
}

VMVar VM_Pop2(VMContext *c) {
	--c->sp;
	if (c->sp < 0) {
		error("Stack underflow");
	}
	debug(DBG_STACK, "pop sp:0x%x", c->sp);
	return c->stack[c->sp];
}

VMVar VM_Top2(VMContext *c) {
	if (c->sp < 1) {
		error("Stack underflow");
	}
	return c->stack[c->sp - 1];
}

void VM_Push(VMContext *c, int value, int type) {
	VMVar *var = &c->stack[c->sp];
	var->value = value;
	var->type  = type;
	++c->sp;
	debug(DBG_STACK, "push sp:0x%x value:%d type:%d", c->sp, value, type);
	if (c->sp >= VMSTACK_SIZE) {
		error("Stack overflow");
	}
}

void VM_PushString(VMContext *c, const char *s) {
	int handle = 0;
	if (s) {
		VMArray *array = Array_New(c);
		Array_SetString(array, s);
		handle = array->handle;
	}
	VM_Push(c, handle, 0x10000 | VAR_TYPE_CHAR);
}

void VM_PushFloat(VMContext *c, float f) {
	const uint32_t val = *(const uint32_t *)&f;
	VM_Push(c, val, VAR_TYPE_FLOAT);
}

int VM_PopInt32(VMContext *c) {
	return VM_Pop(c, VAR_TYPE_INT32);
}

float VM_PopFloat(VMContext *c) {
	const uint32_t val = VM_Pop(c, VAR_TYPE_FLOAT);
	return *(const float *)&val;
}

const char *VM_PopString(VMContext *c) {
	const int num = VM_Pop(c, 0x10000 | VAR_TYPE_CHAR);
	if (num != 0) {
		return ArrayHandle_GetString(c, num);
	}
	return 0;
}
