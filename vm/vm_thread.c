
#include "util.h"
#include "vm.h"

VMThread *Thread_New(VMContext *c) {
	assert(c->threads_next_free != 0);
	const int num = c->threads_next_free;
	VMThread *thread = &c->threads[num];
	c->threads_next_free = thread->next_free;
	memset(thread, 0, sizeof(VMThread));
	++c->thread_handle_counter;
	if (c->thread_handle_counter > BASE_HANDLE_THREAD + 1999999) {
		error("Thread handle %d overflow", c->thread_handle_counter);
	}
	thread->handle = thread->id = c->thread_handle_counter;
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
	for (VMThread *thread = c->threads_head; thread; thread = thread->next) {
		if ((handle == 0 || thread->id == handle) && thread->labels[num] != 0) {
			thread->script->code_offset = thread->labels[num];
			thread->break_counter = 0;
			thread->break_time = 0;
		}
	}
}

int ThreadHandle_FindId(VMContext *c, int handle) {
	for (VMThread *current = c->threads_head; current; current = current->next) {
		if (current->handle == handle) {
			return current->id;
		}
	}
	return 0;
}
