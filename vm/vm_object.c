
#include "util.h"
#include "vm.h"

VMObject *VM_GetObjectFromHandle(VMContext *c, int num) {
	const int x = num - BASE_HANDLE_OBJECT;
	if (x < 0 || x >= c->objects_count) {
		error("Object handle %d out of range (%d..%d)", num, BASE_HANDLE_OBJECT, BASE_HANDLE_OBJECT + c->objects_count);
	}
	VMObject *obj = &c->objects[x];
	assert(obj->handle == num);
	return obj;
}

VMObject *Object_New(VMContext *c) {
	assert(c->objects_count < VMOBJECTS_COUNT);
	VMObject *object = &c->objects[c->objects_count];
	memset(object, 0, sizeof(VMObject));
	object->handle = BASE_HANDLE_OBJECT + c->objects_count;
	++c->objects_count;
	return object;
}

VMVar *Object_GetMemberVar(VMObject *obj, int num) {
	if (num < 0 || num > obj->members_count) {
		error("Member index %d out of range (%d..%d)", num, 1, obj->members_count);
	}
	return &obj->members[num];
}

int ObjectHandle_Create(VMContext *c, int class_handle) {
	VMObject *obj = Object_New(c);
	obj->class_handle = class_handle;
	SobData *sob = ClassHandle_GetSob(c, class_handle);
	const int count = sob->default_membervars_count;
	if (count != 0) {
		obj->members = (VMVar *)calloc(count + 1, sizeof(VMVar));
		if (!obj->members) {
			error("Failed to allocate %d member vars", count + 1);
		} else {
			for (int i = 1; i <= sob->default_membervars_count; ++i) {
				const SobVar *var = &sob->default_membervars_data[i];
				VM_CheckVarType(var->type);
				obj->members[i].type = var->type;
				obj->members[i].value = var->value;
			}
			obj->members_count = count;
		}
	}
	return obj->handle;
}

void ObjectHandle_Delete(VMContext *c, int obj_handle, int call_delete) {
	if (obj_handle != 0) {
		VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
		VM_DeleteObject(c, obj, call_delete);
		warning("ObjectHandle_Delete unimplemented");
		free(obj->members);
		memset(obj, 0, sizeof(VMObject));
	}
}
