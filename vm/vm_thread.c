
#include "util.h"
#include "vm.h"

VMThread *VM_GetThreadFromHandle(VMContext *c, int num) {
	const int x = num - BASE_HANDLE_THREAD;
	if (x < 0 || x >= c->threads_count) {
		error("Thread handle %d out of range (%d..%d)", num, BASE_HANDLE_THREAD, BASE_HANDLE_THREAD + c->threads_count);
	}
	return &c->threads[x];
}

VMThread *Thread_New(VMContext *c) {
	assert(c->threads_count < VMTHREADS_COUNT);
	VMThread *thread = &c->threads[c->threads_count];
	memset(thread, 0, sizeof(VMThread));
	thread->handle = BASE_HANDLE_THREAD + c->threads_count;
	++c->threads_count;
	return thread;
}

void Thread_Start(VMThread *thread) {
	thread->id = thread->handle;
	thread->state = 1;
}

void Thread_Stop(VMThread *) {
	/* todo */
}

void Thread_Define(VMThread *thread, int num, int offset) {
	if (num < 0 || num >= 8) {
		error("define goto %d out of range (1...%d)", num, 7);
		return;
	}
	thread->labels[num] = offset;
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
