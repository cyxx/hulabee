
#include "pan.h"
#include "util.h"
#include "vm.h"

static const struct {
	const char *name;
	uint32_t value;
} _COLORS[] = {
	{ "red", 0x0000C0 },
	{ "green", 0x00C000 },
	{ "blue", 0xC00000 },
	{ "bright red", 0x0000FF },
	{ "bright green", 0x00FF00 },
	{ "bright blue", 0xFF0000 },
	{ "yellow", 0x00FFFF },
	{ "cyan", 0xFFFF00 },
	{ "violet", 0xFF00FF },
	{ "black", 0x000000 },
	{ "light grey", 0xC0C0C0 },
	{ "dark grey", 0x646464 },
	{ "brown", 0x004080 },
	{ 0, 0 }
};

VMContext *VM_NewContext() {
	VMContext *c = (VMContext *)calloc(1, sizeof(VMContext));
	if (c) {
		c->classes_count = 1; /* null class 0 */
		c->arrays_next_free = 1;
		for (int i = 1; i < VMARRAYS_COUNT - 1; ++i) {
			c->arrays[i].next_free = i + 1;
		}
		c->objects_next_free = 1;
		for (int i = 1; i < VMOBJECTS_COUNT - 1; ++i) {
			c->objects[i].next_free = i + 1;
		}
		c->threads_next_free = 1;
		for (int i = 1; i < VMTHREADS_COUNT - 1; ++i) {
			c->threads[i].next_free = i + 1;
		}
		c->thread_handle_counter = BASE_HANDLE_THREAD;
		c->gameID = -1; /* to handle bytecode and syscalls differences */
		c->thread_handle_counter = BASE_HANDLE_THREAD;
		for (int i = 0; _COLORS[i].name; ++i) {
			VM_DefineInt(c, _COLORS[i].name, _COLORS[i].value);
		}
	}
	return c;
}

void VM_FreeContext(VMContext *c) {
	free(c);
}

void VM_DefineInt(VMContext *c, const char *name, uint32_t value) {
	/* todo */
}

void VM_DefineVar(VMContext *c, const char *name, const char *val) {
}

void VM_SetGameID(VMContext *c, int gameID) {
	c->gameID = gameID;
}

void VM_RegisterSyscalls(VMContext *c, const VMSyscall *syscalls) {
	for (; syscalls->num > 0; ++syscalls) {
		assert(c->syscalls_count < SYSCALLS_COUNT);
		c->syscalls[c->syscalls_count] = *syscalls;
		++c->syscalls_count;
	}
	debug(DBG_VM, "Total syscalls:%d", c->syscalls_count);
}

static int compareSyscall(const void *a, const void *b) {
	const VMSyscall *syscall1 = (const VMSyscall *)a;
	const VMSyscall *syscall2 = (const VMSyscall *)b;
	return syscall1->num - syscall2->num;
}

void VM_InitSyscalls(VMContext *c) {
	VM_RegisterSyscalls(c, _syscalls_asset);
	VM_RegisterSyscalls(c, _syscalls_console);
	VM_RegisterSyscalls(c, _syscalls_debug);
	VM_RegisterSyscalls(c, _syscalls_file);
	VM_RegisterSyscalls(c, _syscalls_image);
	VM_RegisterSyscalls(c, _syscalls_input);
	VM_RegisterSyscalls(c, _syscalls_math);
	VM_RegisterSyscalls(c, _syscalls_sound);
	VM_RegisterSyscalls(c, _syscalls_sprite);
	VM_RegisterSyscalls(c, _syscalls_string);
	VM_RegisterSyscalls(c, _syscalls_system);
	VM_RegisterSyscalls(c, _syscalls_time);
	VM_RegisterSyscalls(c, _syscalls_window);
	qsort(c->syscalls, c->syscalls_count, sizeof(VMSyscall), compareSyscall);
}

static int compareSyscallByNum(const void *a, const void *b) {
	const uint32_t num = *(const uint32_t *)a;
	const VMSyscall *syscall = (const VMSyscall *)b;
	return num - syscall->num;
}

int VM_FindSyscallIndex(VMContext *c, int num) {
	const VMSyscall *syscall = (const VMSyscall *)bsearch(&num, c->syscalls, c->syscalls_count, sizeof(VMSyscall), compareSyscallByNum);
	if (syscall) {
		return syscall - c->syscalls;
	}
	return -1;
}

void VM_ExecuteSyscallByIndex(VMContext *c, int index) {
	assert(index >= 0 && index < c->syscalls_count);
	(*c->syscalls[index].func)(c);
}

void VM_RunMainBoot(VMContext *c, const char *name, const char *params) {
	VM_LoadClass(c, name, 0);

	VMArray *array = Array_New(c);
	Array_Dim(array, 0x10000 | VAR_TYPE_CHAR, 1, 0);
	VM_Push(c, array->handle, 0x10100 | VAR_TYPE_CHAR);

	const int ret = VM_InvokeStaticMethod(c, name, "boot(C[[)V");
	if (ret < 0) {
		VM_Pop2(c);
		VM_InvokeStaticMethod(c, name, "boot()V");
	}
}

int VM_InvokeStaticMethod(VMContext *c, const char *class_name, const char *static_method) {
	const int num = VM_FindOrLoadClass(c, class_name, 1);
	if (num == 0) {
		error("Can't find class %s", class_name);
		return -1;
	}
	SobData *sob = ClassHandle_GetSob(c, num);
	const int method_num = Sob_FindMethod(sob, static_method);
	if (method_num == 0) {
		warning("Method '%s' not found", static_method);
		return -1;
	}
	return VM_InvokeMethod(c, sob, method_num, 0, 1, 1, 0);
}

static VMScript *prepareCall(VMContext *c, VMScript *script, int class_handle, int obj_handle, int code_num) {
	debug(DBG_VM, "prepareCall c:%p script:%p class_handle:%d obj_handle:%d", c, script, class_handle, obj_handle);
	SobData *sob = ClassHandle_GetSob(c, class_handle);
	SobCodeEntry *code = Sob_GetCode(sob, code_num);
	if (code->class_handle == -1) {
		error("Code entry %d in class %d not fixed up correctly", code_num, class_handle);
	}

	assert(code->locals_ptr);
	int32_t locals_size = READ_LE_UINT32(code->locals_ptr);
	int32_t args_count = READ_LE_UINT32(code->locals_ptr + 4);
	debug(DBG_VM, "locals_size:%d args_count:%d", locals_size, args_count);
	if (locals_size < 0 || args_count < 0 || args_count > locals_size || locals_size > 10000) {
		error("localCount (%d) out of range (%d..%d)", locals_size, 0, 10000);
	}
	++locals_size;
	script->local_vars_count = locals_size;
	script->local_vars = (VMVar *)calloc(locals_size, sizeof(VMVar));
	if (!script->local_vars) {
		error("Failed to allocate %d localVars", locals_size);
	} else {
		int offset = 8;
		for (int i = 1; i < locals_size; ++i) {
			script->local_vars[i].type = READ_LE_UINT32(code->locals_ptr + offset); offset += 4;
			script->local_vars[i].value = 0;
		}
		script->local_vars[0].type = VAR_TYPE_OBJECT;
		script->local_vars[0].value = obj_handle;
	}

	script->obj_handle = obj_handle;

	for (; args_count >= 1; --args_count) {
		VMVar st = VM_Pop2(c);
		VMVar *var = &script->local_vars[args_count];
		VM_CheckVarType(var->type);
		VM_CheckVarType(st.type);
		var->value = VM_ConvertVar(var->type, &st);
	}

	script->code_offset = code->code_offset;
	script->code_data = code->code_ptr;
	script->class_handle = code->class_handle;
	// script->unk14 = code->unk14;
	script->next_script = 0;

	return script;
}

static void endCall(VMContext *c, VMScript *script) {
	debug(DBG_VM, "endCall c:%p script:%p", c, script);
	free(script->local_vars);
	script->local_vars = 0;
	VMScript *next = script->next_script;
	if (next) {
		endCall(c, next);
	}
}

static int executeMethod(VMContext *c, VMScript *script, VMThread *thread, int flag) {
	debug(DBG_VM, "executeMethod script:%p thread:%p flag:%d", script, thread, flag);
	script->thread = thread;
	if (script->obj_handle) {
		VMObject *obj = VM_GetObjectFromHandle(c, script->obj_handle);
		if (!obj) {
			error("Object handle %d was deleted", script->obj_handle);
		}
		script->obj = obj;
	} else {
		script->obj = 0;
	}
	script->sob_data = ClassHandle_GetSob(c, script->class_handle);
	VMScript *prev_script = c->script;
	c->script = script;
	const uint8_t *prev_code = c->code;
	c->code = script->code_offset + script->code_data;
	script->state = 0;
	while (1) {
		const uint8_t op = *c->code++;
		++script->code_offset;
		VM_ExecuteOpcode(c, op);
		if (script->state != 0) {
			break;
		}
		if (script->thread->state == SCRIPT_STATE_RUNNING) {
			continue;
		}
		if (script->state == 0) {
			script->state = script->thread->state;
		}
		break;
	}
	script->code_offset = c->code - script->code_data;
	c->code = prev_code;
	c->script = prev_script;
	return script->state;
}

static int startMethod(VMContext *c, int class_handle, int obj_handle, int code_num) {
	VMThread *thread = Thread_New(c);
	Thread_Start(thread);

	VMScript *script = (VMScript *)calloc(1, sizeof(VMScript));
	if (!script) {
		error("Failed to allocate VMScript");
	}
	VMScript *current = prepareCall(c, script, class_handle, obj_handle, code_num);

	thread->script = current;
	VM_AddThread(c, thread);

	thread->unk1C = 1;
	const int ret = executeMethod(c, script, thread, 1);
	if (ret != SCRIPT_STATE_ENDED && thread->state != SCRIPT_STATE_DEAD) {
		return thread->handle;
	}

	VM_RemoveThread(c, thread);
	Thread_Delete(c, thread);
	return 0;
}

int VM_StartMethod(VMContext *c, int obj_handle, const char *name) {
	VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
	SobData *sob = ClassHandle_GetSob(c, obj->class_handle);
	const int num = Sob_FindMethod(sob, name);
	if (num != 0) {
		const SobRefEntry *ref = Sob_GetRefMethod(sob, num);
		if (ref->data_index != 0) {
			startMethod(c, obj->class_handle, obj_handle, ref->data_index);
			return 1;
		}
	}
	return 0;
}

static int callMethod(VMContext *c, VMScript *parent, int class_handle, int obj_handle, int code_num) {
	if (c->gc_counter == -2) { /* AGGRESSIVE */
		VM_GC(0);
	}
	VMScript script;
	prepareCall(c, &script, class_handle, obj_handle, code_num);
	++c->method_call_depth;
	if (c->method_call_depth > 500) {
		error("Method calls nested too deep");
	}
	if (parent->next_script) {
		error("m_next not NULL (Internal error)");
	}
	parent->next_script = &script;
	const int ret = executeMethod(c, &script, parent->thread, 1);
	if (ret == SCRIPT_STATE_YIELD) {
		error("Non script method did a breakhere");
	}
	parent->next_script = 0;
	--c->method_call_depth;
	endCall(c, &script);
	return 0;
}

static int invokeMethodInternal(VMContext *c, SobData *sob, int method_num, int class_handle, int obj_handle, int start_call, int is_static) {
	debug(DBG_VM, "invokeMethodInternal method:%d class_handle:%d obj_handle:%d start_call:%d is_static:%d", method_num, class_handle, obj_handle, start_call, is_static);
	const SobRefEntry *ref = Sob_GetRefMethod(sob, method_num);
	const int code_num = ref->data_index;
	if (code_num == 0) {
		error("Calling method on incorrect object %d for variable type %d", obj_handle, class_handle);
	}
	if (start_call != 0) {
		if ((ref->flags & 2) != 0 && is_static == 0) {
			error("Object start of static script @%d", method_num);
		}
		if ((ref->flags & 2) == 0 && is_static != 0) {
			error("Static start of object script @%d", method_num);
		}
		if ((ref->flags & 8) == 0) {
			error("Script @%d called as a method", method_num);
			return 0;
		}
		if (ref->name_index != 0) {
			debug(DBG_VM, "Starting %s.%s", ClassHandle_GetName(c, class_handle), Sob_GetString(sob, ref->name_index));
		}
		const int ret = startMethod(c, class_handle, obj_handle, code_num);
		if (ret != 0) {
			if (start_call == 3) {
				c->script->state = SCRIPT_STATE_YIELD;
				c->script->thread->script_thread_handle = ret;
			}
		}
		return ret;
	} else {
		if ((ref->flags & 2) != 0 && is_static == 0) {
			error("Object call of static method @%d", method_num);
		}
		if ((ref->flags & 2) == 0 && is_static != 0) {
			error("Static call of object script @%d", method_num);
		}
		if ((ref->flags & 8) != 0) {
			error("Method @%d called as a script", method_num);
			return 0;
		}
		if (!c->script) {
			error("Trying to call method @%d with no current method running", method_num);
		}
		if (ref->name_index != 0) {
			debug(DBG_VM, "Calling %s.%s", ClassHandle_GetName(c, class_handle), Sob_GetString(sob, ref->name_index));
		}
		callMethod(c, c->script, class_handle, obj_handle, code_num);
	}
	return 0;
}

int VM_InvokeMethod(VMContext *c, SobData *sob, int member_index, int obj_handle, int start_call, int is_static, int is_parent) {
	SobRefEntry *refMethod = Sob_GetRefMethod(sob, member_index);
	int class_handle = 0;
	if (obj_handle != 0) {
		VMObject *obj = VM_GetObjectFromHandle(c, obj_handle);
		if (!obj) {
			error("Object handle %d was deleted", obj_handle);
		} else {
			class_handle = obj->class_handle;
		}
	} else {
		SobRefEntry *refClass = Sob_GetRefClass(sob, refMethod->class_index);
		if (refClass->class_handle == 0) {
			const char *name = Sob_GetString(sob, refClass->name_index);
			refClass->class_handle = VM_FindOrLoadClass(c, name, 1);
		}
		class_handle = refClass->class_handle;
	}
	if (refMethod->data_index != 0 && is_parent == 0 && sob->class_handle == class_handle) {
		return invokeMethodInternal(c, sob, member_index, class_handle, obj_handle, start_call, is_static);
	}
	if (obj_handle != 0) {
		if (is_parent != 0) {
			if (!c->script) {
				error("Accessing parent with no current method");
			}
			const int num = c->script->class_handle;
			SobData *sob = ClassHandle_GetSob(c, num);
			class_handle = sob->parent_handle;
			if (class_handle == 0) {
				error("Class %s does not have a parent", ClassHandle_GetName(c, num));
			}
		}
	} else {
		SobRefEntry *ref = Sob_GetRefClass(sob, refMethod->class_index);
		if (ref->data_index == 0) {
			const char *name = Sob_GetString(sob, ref->name_index);
			ref->data_index = VM_FindOrLoadClass(c, name, 1);
		}
		class_handle = ref->data_index;
	}
	SobData *sob2 = ClassHandle_GetSob(c, class_handle);
	if (refMethod->member_index == 0) {
		const char *method = Sob_GetString(sob, refMethod->name_index);
		const int num = Sob_FindMethod(sob2, method);
		if (num == 0) {
			if (strcasecmp("_new_()V", method) == 0) {
				return 0;
			} else if (strcasecmp("_delete_()V", method) == 0) {
				return 0;
			}
			error("Can't find method %s in class %d", method, class_handle);
		}
		refMethod->member_index = num;
	}
	return invokeMethodInternal(c, sob2, refMethod->member_index, class_handle, obj_handle, start_call, is_static);
}

int VM_FindOrLoadClass(VMContext *context, const char *name, int error_flag) {
	assert(name);
	debug(DBG_VM, "VM_FindOrLoadClass '%s' count:%d", name, context->classes_count);
	for (int i = 1; i < context->classes_count; ++i) {
		const VMClass *c = &context->classes[i];
		if (c->name && strcasecmp(name, c->name) == 0) {
			assert(c->sob_data);
			debug(DBG_VM, "Found class num:%d sob:%p", i, c->sob_data);
			return BASE_HANDLE_CLASS + i;
		}
	}
	return VM_LoadClass(context, name, error_flag);
}

static void fixUp(VMContext *c, SobData *sob) {
	if (sob->fixup_flag) {
		return;
	}
	sob->parent_handle = 0;
	sob->refentries_data[1].class_handle = sob->class_handle;
	for (int i = 0; i < sob->codeentries_count; ++i) {
		SobCodeEntry *code = &sob->codeentries_data[i + 1];
		if (code->locals_offset != -1) {
			assert((code->locals_offset & 3) == 0);
			code->locals_ptr = sob->local_data + code->locals_offset;
			code->code_ptr = sob->code_data;
			code->class_handle = sob->class_handle;
		}
	}
	if (sob->frameworks_count > 0) {
		const int num = sob->frameworks_data[0];
		SobRefEntry *ref = Sob_GetRefClass(sob, num);
		const char *name = Sob_GetString(sob, ref->name_index);
		const int parent_handle = VM_FindOrLoadClass(c, name, 1);
		sob->parent_handle = parent_handle;
		debug(DBG_VM, "ParentClass handle %d name '%s'", parent_handle, name);
		if (!sob->fixup_flag) {
			SobData *parentSob = ClassHandle_GetSob(c, parent_handle);
			for (int i = 1; i <= sob->refentries_count; ++i) {
				const SobRefEntry *ref = &sob->refentries_data[i];
				if (ref->class_index == 1 && ref->type == SOB_REFERENCE_TYPE_METHOD) {
					SobCodeEntry *code = &sob->codeentries_data[ref->data_index];
					if (code->locals_offset != -1) {
						continue;
					}
					const char *name = Sob_GetString(sob, ref->name_index);
					int method_num = Sob_FindMethod(parentSob, name);
					if (method_num == 0) {
						error("Virtual function %s not found in class %d", name, sob->parent_handle);
						continue;
					}
					const SobRefEntry *methodRef = Sob_GetRefMethod(parentSob, method_num);
					const SobCodeEntry *parentCodeEntry = &parentSob->codeentries_data[methodRef->data_index];
					if (parentCodeEntry->code_ptr == 0) {
						error("Virtual function %s not found in class %d", name, sob->parent_handle);
						continue;
					}
					code->code_offset = parentCodeEntry->code_offset;
					code->code_ptr = parentCodeEntry->code_ptr;
					code->locals_ptr = parentCodeEntry->locals_ptr;
					code->class_handle = parentCodeEntry->class_handle;
					// code->unk14 = parentCodeEntry->unk14;
				}
			}
		}
	}
	sob->fixup_flag = 1;
}

static int startStaticClassMethod(VMContext *c, int class_handle, const char *name) {
	SobData *sob = ClassHandle_GetSob(c, class_handle);
	const int method_num = Sob_FindMethod(sob, name);
	if (method_num != 0) {
		const SobRefEntry *ref = &sob->refentries_data[method_num];
		if (ref->data_index != 0) {
			startMethod(c, class_handle, 0, ref->data_index);
			return 1;
		}
	}
	return 0;
}

int VM_LoadClass(VMContext *context, const char *name, int error_flag) {
	debug(DBG_VM, "VM_LoadClass '%s'", name);
	const int first_class_handle = context->classes_count;
	char filename[64];
	snprintf(filename, sizeof(filename), "%s.sob", name);
	PanBuffer pb;
	if (!Pan_LoadAssetByName(filename, &pb)) {
		if (error_flag) {
			error("Failed to load class '%s'", name);
		}
		return 0;
	}
	for (int offset = 0; offset < pb.size; ) {
		SobData *sob = LoadSob(pb.buffer, pb.size, &offset, filename);

		assert(context->classes_count < VMCLASSES_COUNT);
		const int num = context->classes_count;
		VMClass *c = &context->classes[context->classes_count++];
		memset(c, 0, sizeof(VMClass));
		c->sob_data = sob;
		const int handle = BASE_HANDLE_CLASS + num;

		sob->class_handle = handle;

		fixUp(context, sob);

		SobRefEntry *ref = Sob_GetRefClass(sob, 1); /* class name is first reference */
		const char *class_name = Sob_GetString(sob, ref->name_index);
		debug(DBG_VM, "Class handle %d name %s sob %p", sob->class_handle, class_name, sob);
		sob->class_name = class_name;
		c->name = class_name;
		const int method_num = Sob_FindMethod(sob, "_static_()V");
		if (method_num != 0) {
			startStaticClassMethod(context, handle, "_static_()V");
		}
		for (int i = 0; i < sob->autoload_count; ++i) {
			const char *name = Sob_GetString(sob, sob->autoload_data[i]);
			VM_FindOrLoadClass(context, name, 1);
		}
	}
	return BASE_HANDLE_CLASS + first_class_handle;
}

void VM_StartCallback(VMContext *c, int handle, const char *name) {
	debug(DBG_VM, "VM_StartCallback '%s' handle:%d", name, handle);
	if (handle >= BASE_HANDLE_OBJECT) {
		VM_StartMethod(c, handle, name);
	} else if (handle >= BASE_HANDLE_CLASS) {
		startStaticClassMethod(c, handle, name);
	} else {
		error("callback scope not a object or class");
	}
}

void VM_RunThreads(VMContext *context) {
	++context->frame_counter;
	VMThread *thread = context->threads_head;
	while (thread) {
		thread->unk1C = 0;
		thread = thread->next;
	}
	bool changed = false;
	do {
		thread = context->threads_head;
		while (thread) {
			VMThread *current = thread->next;
			if (!current) {
				break;
			}
			if (thread->order > current->order) {
				/* todo */
				warning("Executing threads out of order");
				break;
				// changed = true;
			}
			thread = thread->next;
		}
	} while (changed);
	thread = context->threads_head;
	while (thread) {
		VMThread *next_thread = thread->next;
		debug(DBG_VM, "Thread handle:%d id:%d order:%d state:%d", thread->handle, thread->id, thread->order, thread->state);
		if (thread->state == SCRIPT_STATE_DEAD) {
			VM_RemoveThread(context, thread);
			Thread_Delete(context, thread);
			goto next;
		}
		if (thread->unk1C != 0) {
			goto next;
		}
		if (thread->break_counter != 0) {
			--thread->break_counter;
			goto next;
		}
		if (thread->break_time != 0) {
			if (thread->break_time > (*context->get_timer)()) {
				goto next;
			}
			thread->break_time = 0;
		}
		if (thread->script_thread_handle != 0) {
			const int num = thread->script_thread_handle;
			for (VMThread *current = context->threads_head; current; current = current->next) {
				if (num == current->handle) {
					goto next;
				}
			}
			thread->script_thread_handle = 0;
		}
		if (context->sp != 0) {
			warning("Stack not empty between threads");
			context->sp = 0;
		}
		const int r = executeMethod(context, thread->script, thread, 1);
		if (r != SCRIPT_STATE_ENDED && thread->state != SCRIPT_STATE_DEAD) {
		} else {
			VM_RemoveThread(context, thread);
			Thread_Delete(context, thread);
		}
next:
		thread = next_thread;
	}
}

void VM_GC(int flag) {
	/* todo */
}

int VM_ConvertVar(int type, const VMVar *var) { /* to, from */
	if (type != var->type && var->value != 0 && (type & 0xFF) != 12 && (var->type & 0xFF) != 12) {
		if (var->type == 9 && type <= 7) {
			return var->value;
		}
		if (var->type <= 7 && type == 9) {
			return var->value;
		}
		if (var->type <= 7 && type <= 7) {
			return var->value;
		}
		if (var->type == 10 || type == 10) {
			return var->value;
		}
		error("Can't convert from %s to %s", VM_GetVarTypeName(var->type), VM_GetVarTypeName(type));
	}
	return var->value;
}

void VM_CheckVarType(int type) {
	type &= 0xFF;
	if (type != 12) {
		if (type <= 0 || type >= 11) {
			error("Illegal variable type:%d", type);
		}
	}
}

SobVar *VM_GetClassStaticVar(VMContext *c, SobData *sob, int num) {
	debug(DBG_VM, "VM_GetClassStaticVar num:%d", num);
	SobRefEntry *ref = 0;
	while (1) {
		num &= 0xFFFF;
		ref = Sob_GetRefStatic(sob, num);
		if (ref->class_index == 1) {
			break;
		}
		SobRefEntry *ref2 = Sob_GetRefClass(sob, ref->class_index);
		if (ref2->class_handle == 0) {
			const char *name = Sob_GetString(sob, ref2->name_index);
			ref2->class_handle = VM_FindOrLoadClass(c, name, 1);
		}
		SobData *sob2 = ClassHandle_GetSob(c, ref2->class_handle);
		int num2 = ref->member_index;
		if (ref->member_index == 0) {
			const char *name = Sob_GetString(sob, ref->name_index);
			num2 = Sob_FindStatic(sob2, name);
			if (num2 == 0) {
				error("Static variable '%s' not found", name);
			}
			/* ref->member_index = num2; */ // not set in the original
		}
		SobRefEntry *ref3 = Sob_GetRefStatic(sob2, num2);
		if (ref3->class_index != 1) {
			const char *name = Sob_GetString(sob, ref->name_index);
			error("Indirect static reference '%s' failed", name);
		}
		sob = sob2;
		num = num2;
	}
	assert(ref);
	const int x = ref->data_index;
	return Sob_GetStaticVar(sob, x);
}

VMVar *VM_GetLocalVar(VMContext *c, int num) {
	if (num < 0 || num > c->script->local_vars_count) {
		error("Local variable %d out of range (%d..%d)", num, 0, c->script->local_vars_count);
	}
	VMVar *var = &c->script->local_vars[num];
	return var;
}

VMVar *VM_GetObjectMemberVar(VMContext *c, VMObject *obj, int num) {
	SobData *sob1 = c->script->sob_data;
	SobRefEntry *ref = Sob_GetRefMember(sob1, num);
	if (ref->member_index == 0) {
		const int class_handle = VM_GetClassHandleFromRef(c, sob1, ref->class_index, 1);
		SobData *sob2 = ClassHandle_GetSob(c, class_handle);
		const char *name = Sob_GetString(sob1, ref->name_index);
		ref->member_index = Sob_FindMember(sob2, name);
		if (ref->member_index == 0) {
			error("Undefined symbol '%s'", name);
		}
	}
	const int member_num = ref->member_index & 0xFFFF;
	SobData *sob3 = ClassHandle_GetSob(c, obj->class_handle);
	SobRefEntry *ref2 = Sob_GetRefMember(sob3, member_num);
	if ((ref2->flags & 2) != 0) {
		error("Member access to static variable @%d", member_num);
	}
	VMVar *member = Object_GetMemberVar(obj, ref2->data_index);
	return member;
}

const char *VM_GetVarTypeName(int type) {
	switch (type & 0xFF) {
	case 0:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "NONE[][]";
			} else {
				return "NONE[]";
			}
		} else {
			if (type & 0x100) {
				return "NONE[]";
			} else {
				return "NONE";
			}
		}
		break;
	case 1:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "INT1[][]";
			} else {
				return "INT1[]";
			}
		} else {
			if (type & 0x100) {
				return "INT1[]";
			} else {
				return "INT1";
			}
		}
		break;
	case 2:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "INT2[][]";
			} else {
				return "INT2[]";
			}
		} else {
			if (type & 0x100) {
				return "INT2[]";
			} else {
				return "INT2";
			}
		}
		break;
	case 3:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "INT4[][]";
			} else {
				return "INT4[]";
			}
		} else {
			if (type & 0x100) {
				return "INT4[]";
			} else {
				return "INT4";
			}
		}
		break;
	case 4:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "BYTE[][]";
			} else {
				return "BYTE[]";
			}
		} else {
			if (type & 0x100) {
				return "BYTE[]";
			} else {
				return "BYTE";
			}
		}
		break;
	case 5:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "CHAR[][]";
			} else {
				return "CHAR[]";
			}
		} else {
			if (type & 0x100) {
				return "CHAR[]";
			} else {
				return "CHAR";
			}
		}
		break;
	case 6:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "INT16[][]";
			} else {
				return "INT16[]";
			}
		} else {
			if (type & 0x100) {
				return "INT16[]";
			} else {
				return "INT16";
			}
		}
		break;
	case 7:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "INT32[][]";
			} else {
				return "INT32[]";
			}
		} else {
			if (type & 0x100) {
				return "INT32[]";
			} else {
				return "INT32";
			}
		}
		break;
	case 8:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "FLOAT[][]";
			} else {
				return "FLOAT[]";
			}
		} else {
			if (type & 0x100) {
				return "FLOAT[]";
			} else {
				return "FLOAT";
			}
		}
		break;
	case 9:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "OBJECT[][]";
			} else {
				return "OBJECT[]";
			}
		} else {
			if (type & 0x100) {
				return "OBJECT[]";
			} else {
				return "OBJECT";
			}
		}
		break;
	case 10:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "STRUCT[][]";
			} else {
				return "STRUCT[]";
			}
		} else {
			if (type & 0x100) {
				return "STRUCT[]";
			} else {
				return "STRUCT";
			}
		}
		break;
	default:
		if (type & 0x10000) {
			if (type & 0x100) {
				return "UNKNOWN[][]";
			} else {
				return "UNKNOWN[]";
			}
		} else {
			if (type & 0x100) {
				return "UNKNOWN[]";
			} else {
				return "UNKNOWN";
			}
		}
		break;
	}
	return "";
}

VMClass *VM_GetClassFromHandle(VMContext *c, int num) {
	const int x = num - BASE_HANDLE_CLASS;
	if (x < 0 || x >= c->classes_count) {
		error("Class handle %d out of range (%d..%d)", num, BASE_HANDLE_CLASS, BASE_HANDLE_CLASS + c->classes_count);
	}
	return &c->classes[x];
}

int VM_GetClassHandleFromRef(VMContext *c, SobData *sob, int num, int flag) {
	SobRefEntry *ref = Sob_GetRefClass(sob, num);
	if (ref->class_handle == 0) {
		const char *name = Sob_GetString(sob, ref->name_index);
		ref->class_handle = VM_FindOrLoadClass(c, name, flag);
	}
	return ref->class_handle;
}

SobData *ClassHandle_GetSob(VMContext *context, int num) {
	VMClass *c = VM_GetClassFromHandle(context, num);
	return c->sob_data;
}

const char *ClassHandle_GetName(VMContext *context, int num) {
	VMClass *c = VM_GetClassFromHandle(context, num);
	return c->name;
}

void VM_AddThread(VMContext *c, VMThread *thread) {
	if (c->threads_head == 0) {
		c->threads_head = c->threads_tail = thread;
		thread->prev = thread->next = 0;
	} else {
                c->threads_head->prev = thread;
		thread->next = c->threads_head;
		thread->prev = 0;
		c->threads_head = thread;
	}
}

void VM_RemoveThread(VMContext *c, VMThread *thread) {
	VMThread *next = thread->next;
	if (next) {
		next->prev = thread->prev;
	} else {
		c->threads_tail = thread->prev;
	}
	VMThread *prev = thread->prev;
	if (prev) {
		prev->next = thread->next;
	} else {
		c->threads_head = thread->next;
	}
}

static void stopThreadByObject(VMContext *c, int obj_handle, int thread_num) {
	for (VMThread *thread = c->threads_head; thread; thread = thread->next) {
		VMScript *script = thread->script;
		if (script->obj_handle == obj_handle && thread->id != thread_num) {
			const uint32_t offset = thread->labels[0];
			if (offset != 0) {
				script->code_offset = offset;
				thread->labels[0] = 0;
				executeMethod(c, script, thread, 1);
			}
			thread->state = SCRIPT_STATE_DEAD;
		}
	}
}

void VM_StopThread(VMContext *c, int num, int handle) {
	if (num >= 3000000 && num <= 3999999) {
		stopThreadByObject(c, num, handle);
	} else {
		for (VMThread *thread = c->threads_head; thread; thread = thread->next) {
			if (num == thread->id || num == thread->handle) {
				if (thread->id != handle) {
					const uint32_t offset = thread->labels[0];
					if (offset != 0) {
						VMScript *script = thread->script;
						script->code_offset = offset;
						thread->labels[0] = 0;
						executeMethod(c, script, thread, 1);
					}
					thread->state = SCRIPT_STATE_DEAD;
				}
			}
		}
	}
}

int VM_CountThreads(VMContext *c, int num) {
	int count = 0;
	for (VMThread *thread = c->threads_head; thread; thread = thread->next) {
		if (num == thread->id) {
			++count;
		}
	}
	return count;
}

void VM_DeleteObject(VMContext *c, VMObject *obj, int call_delete) {
	if (call_delete) {
		SobData *sob = ClassHandle_GetSob(c, obj->class_handle);
		const int num = Sob_FindMethod(sob, "_delete_()V");
		if (num != 0) {
			if (c->script) {
				VM_InvokeMethod(c, sob, num, obj->handle, 0, 0, 0);
			} else {
				VM_StartMethod(c, obj->handle, "_delete_()V");
			}
		}
		stopThreadByObject(c, obj->handle, 0);
	}
}
