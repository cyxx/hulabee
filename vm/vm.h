
#ifndef VM_H__
#define VM_H__

#include "intern.h"
#include "sob.h"

#define SYSCALLS_COUNT   192
#define VMCLASSES_COUNT  256
#define VMARRAYS_COUNT  4096
#define VMOBJECTS_COUNT 1024
#define VMTHREADS_COUNT  128
#define VMSTACK_SIZE    1024
#define ENVVARS_SIZE       3

enum {
	VAR_TYPE_BYTE   = 4,
	VAR_TYPE_CHAR   = 5,
	VAR_TYPE_INT16  = 6,
	VAR_TYPE_INT32  = 7,
	VAR_TYPE_FLOAT  = 8,
	VAR_TYPE_OBJECT = 9,
	VAR_TYPE_STRUCT = 10,
};

enum {
	BASE_HANDLE_CLASS  = 2000000,
	BASE_HANDLE_OBJECT = 3000000,
	BASE_HANDLE_ARRAY  = 4000000,
	BASE_HANDLE_THREAD = 5000000,
};

enum {
	SCRIPT_STATE_RUNNING = 1,
	SCRIPT_STATE_SUSPEND = 2,
	SCRIPT_STATE_DEAD    = 3,
	SCRIPT_STATE_ENDED   = 4,
	SCRIPT_STATE_YIELD   = 5
};

struct SobData;
struct SobVar;

typedef struct {
	int type;
	int value;
} VMVar;

struct vmcontext_t;

typedef struct {
	int num;
	void (*func)(struct vmcontext_t *);
} VMSyscall;

typedef struct {
	const char *name;
	SobData *sob_data;
} VMClass;

struct vmarray_key_value_t {
	int key;
	int value;
};

typedef struct {
	uint32_t handle;
	uint16_t next_free;
	int type;
	int elem_size;
	uint8_t *data;
	int dimension;
	int col_lower;
	int col_upper;
	int row_lower;
	int row_upper;
	uint32_t offset;
	int unk28;
	int unk34;
	int unk40;
	int struct_size;
	int is_key_value;
	struct vmarray_key_value_t *kv_data;
	int kv_size;
} VMArray;

typedef struct {
	uint32_t handle;
	uint16_t next_free;
	uint32_t class_handle;
	int members_count;
	VMVar *members;
} VMObject;

struct vmscript_t;

typedef struct vmthread_t {
	uint32_t handle;
	uint16_t next_free;
	int id;
	int order;
	int script_thread_handle;
	int break_counter;
	int break_time;
	int state;
	int unk1C;
	struct vmscript_t *script;
	struct vmthread_t *next;
	struct vmthread_t *prev;
	uint32_t labels[8];
} VMThread;

typedef struct vmscript_t {
	VMThread *thread;
	VMObject *obj;
	SobData *sob_data;
	uint32_t class_handle;
	uint32_t obj_handle;
	// int unk14;
	int state;
	struct vmscript_t *next_script;
	uint32_t code_offset;
	const uint8_t *code_data;
	int local_vars_count;
	VMVar *local_vars;
} VMScript;

struct vmenvvar_t {
	const char *name;
	char *value;
};

typedef struct vmcontext_t {
	int syscalls_count;
	VMSyscall syscalls[SYSCALLS_COUNT];
	int classes_count;
	VMClass classes[VMCLASSES_COUNT];
	int arrays_next_free;
	VMArray arrays[VMARRAYS_COUNT];
	int objects_next_free;
	VMObject objects[VMOBJECTS_COUNT];
	int threads_next_free;
	VMThread threads[VMTHREADS_COUNT];
	VMVar stack[VMSTACK_SIZE];
	int sp;
	const uint8_t *code;
	VMScript *script;
	int gameID;
	int gc_counter;
	int frame_counter;
	int method_call_depth;
	VMThread *threads_head, *threads_tail;
	int thread_handle_counter;
	uint32_t (*get_timer)();
	struct vmenvvar_t env_vars[ENVVARS_SIZE];
	int env_vars_count;
} VMContext;

VMContext *VM_NewContext();
void VM_FreeContext(VMContext *);
void VM_DefineInt(VMContext *, const char *name, uint32_t value);
void VM_DefineVar(VMContext *, const char *name, const char *val);
void VM_ReplaceVar(VMContext *c, const char *s, char *out, int len);
void VM_SetGameID(VMContext *, int gameID);
void VM_InitSyscalls();
void VM_RegisterSyscalls(VMContext *, const VMSyscall *);
void VM_InitSyscalls(VMContext *);
int VM_FindSyscallIndex(VMContext *, int num);
void VM_ExecuteSyscallByIndex(VMContext *, int index);
void VM_RunMainBoot(VMContext *c, const char *name, const char *params);
int VM_InvokeStaticMethod(VMContext *c, const char *class_name, const char *static_name);
int VM_InvokeMethod(VMContext *c, SobData *sob, int member_index, int obj_handle, int start_call, int is_static, int is_parent);
int VM_StartMethod(VMContext *c, int obj_handle, const char *name);
int VM_FindOrLoadClass(VMContext *, const char *name, int error_flag);
int VM_LoadClass(VMContext *, const char *name, int error_flag);
void VM_StartCallback(VMContext *, int handle, const char *name);
void VM_RunThreads(VMContext *);
void VM_GC(int);
int VM_ConvertVar(int type, const VMVar *var);
void VM_CheckVarType(int type);
SobVar *VM_GetClassStaticVar(VMContext *c, SobData *sob, int num);
VMVar *VM_GetLocalVar(VMContext *c, int num);
VMVar *VM_GetObjectMemberVar(VMContext *c, VMObject *obj, int num);
const char *VM_GetVarTypeName(int type);
VMClass *VM_GetClassFromHandle(VMContext *c, int num);
int VM_GetClassHandleFromRef(VMContext *c, SobData *sob, int num, int flag);
SobData *ClassHandle_GetSob(VMContext *c, int num);
const char *ClassHandle_GetName(VMContext *c, int num);
void VM_AddThread(VMContext *c, VMThread *thread);
void VM_RemoveThread(VMContext *c, VMThread *thread);
void VM_StopThread(VMContext *c, int num, int handle);
int VM_CountThreads(VMContext *c, int num);
void VM_DeleteObject(VMContext *c, VMObject *obj, int call_delete);

// vm_opcodes
void VM_InitOpcodes();
void VM_ExecuteOpcode(VMContext *c, int op);

// vm_stack
int VM_Pop(VMContext *, int expected_type);
VMVar VM_Pop2(VMContext *);
VMVar VM_Top2(VMContext *);
void VM_Push(VMContext *, int value, int type);
void VM_PushString(VMContext *, const char *s);
void VM_PushFloat(VMContext *c, float f);
int VM_PopInt32(VMContext *);
float VM_PopFloat(VMContext *);
const char *VM_PopString(VMContext *);

// vm_array
VMArray *VM_GetArrayFromHandle(VMContext *c, int num);
VMArray *Array_New(VMContext *c);
void Array_Dim(VMArray *array, int type, int col_lower, int col_upper);
void Array_Dim2(VMArray *array, int type, int row_lower, int row_upper, int col_lower, int col_upper);
void Array_SetString(VMArray *array, const char *s);
int Array_Get(VMArray *array, int offset);
void Array_Set(VMArray *array, int offset, int value);
int Array_Find(VMArray *array, int value);
int Array_DeleteIndex(VMArray *array, int start, int end);
int Array_CheckIndex(VMArray *array, int offset);
void Array_InsertUpper(VMArray *array, int value);
int Array_DeleteUpper(VMArray *array);
int Array_GetStringLength(VMArray *array);
int Array_Copy1(VMContext *c, VMArray *array);
int Array_Range1(VMContext *c, VMArray *array, int start, int end);
int Array_Rand(VMArray *array);
const char *ArrayHandle_GetString(VMContext *c, int handle);
void ArrayHandle_ConcatString(VMContext *c, int array1, int array2);
int ArrayHandle_CompareString(VMContext *c, int array1, int array2);
int ArrayHandle_AddString(VMContext *c, int array1, int array2);
void ArrayHandle_LowerString(VMContext *c, int array);
void ArrayHandle_UpperString(VMContext *c, int array);
void ArrayHandle_Delete(VMContext *c, int handle);

// vm_thread
VMThread *Thread_New(VMContext *c);
void Thread_Delete(VMContext *c, VMThread *);
void Thread_Start(VMThread *);
void Thread_Define(VMThread *, int num, int offset);
void ThreadHandle_GoTo(VMContext *c, int handle, int num);
int ThreadHandle_FindId(VMContext *c, int handle);

// vm_object
VMObject *VM_GetObjectFromHandle(VMContext *c, int num);
VMObject *Object_New(VMContext *c);
VMVar *Object_GetMemberVar(VMObject *obj, int num);
int ObjectHandle_Create(VMContext *c, int class_handle);
void ObjectHandle_Delete(VMContext *c, int obj_handle, int call_delete);

extern const VMSyscall _syscalls_asset[];
extern const VMSyscall _syscalls_console[];
extern const VMSyscall _syscalls_debug[];
extern const VMSyscall _syscalls_file[];
extern const VMSyscall _syscalls_image[];
extern const VMSyscall _syscalls_input[];
extern const VMSyscall _syscalls_math[];
extern const VMSyscall _syscalls_sound[];
extern const VMSyscall _syscalls_sprite[];
extern const VMSyscall _syscalls_string[];
extern const VMSyscall _syscalls_system[];
extern const VMSyscall _syscalls_time[];
extern const VMSyscall _syscalls_window[];

#endif /* VM_H__ */
