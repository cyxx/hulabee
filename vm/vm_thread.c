
#include "util.h"
#include "vm.h"

VMThread *VM_GetThreadFromHandle(VMContext *c, int num) {
	const int x = num - BASE_HANDLE_THREAD;
	if (x < 0 || x >= VMTHREADS_COUNT) {
		error("Thread handle %d out of range (%d..%d)", num, BASE_HANDLE_THREAD, BASE_HANDLE_THREAD + VMTHREADS_COUNT);
	}
	VMThread *thread = &c->threads[x];
	assert(thread->handle == num);
	return thread;
}

VMThread *Thread_New(VMContext *c) {
	assert(c->threads_next_free != 0);
	const int num = c->threads_next_free;
	VMThread *thread = &c->threads[num];
	c->threads_next_free = thread->next_free;
	memset(thread, 0, sizeof(VMThread));
	thread->handle = BASE_HANDLE_THREAD + num;
	return thread;
}

void Thread_Delete(VMContext *c, VMThread *thread) {
	VMScript *script = thread->script;
	if (script) {
		/* todo */
	}
	thread->next_free = c->threads_next_free;
	c->threads_next_free = thread - c->threads;
}

void Thread_Start(VMThread *thread) {
	thread->id = thread->handle;
	thread->state = 1;
}

void Thread_Define(VMThread *thread, int num, int offset) {
	if (num < 0 || num >= 8) {
		error("define goto %d out of range (1...%d)", num, 7);
		return;
	}
	thread->labels[num] = offset;
}

void ThreadHandle_GoTo(VMContext *c, int handle, int num) {
	if (num < 0 || num >= 8) {
		error("define goto %d out of range (1...%d)", num, 7);
		return;
	}
	for (VMThread *thread = c->threads_tail; thread; thread = thread->prev) {
		if ((handle == 0 || thread->id == handle) && thread->labels[num] != 0) {
			thread->script->code_offset = thread->labels[num];
			thread->break_counter = 0;
			thread->break_time = 0;
		}
	}
}

int ThreadHandle_FindId(VMContext *c, int handle) {
	VMThread *thread = VM_GetThreadFromHandle(c, handle);
	for (VMThread *current = c->threads_head; current; current = current->prev) {
		if (current->handle == handle) {
			assert(current == thread);
			return current->id;
		}
	}
	return 0;
}
