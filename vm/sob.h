
#ifndef SOB_H__
#define SOB_H__

#include "intern.h"

typedef struct {
	int type;
	int value;
} SobVar;

typedef struct {
	const uint8_t *locals_ptr;
	int32_t locals_offset;
	const uint8_t *code_ptr;
	uint32_t code_offset;
	int32_t class_handle;
	// uint32_t unk14;
} SobCodeEntry;

enum {
	SOB_REFERENCE_TYPE_CLASS  = 2,
	SOB_REFERENCE_TYPE_METHOD = 3,
	SOB_REFERENCE_TYPE_MEMBER = 4,
	SOB_REFERENCE_TYPE_STATIC = 5,
	SOB_REFERENCE_TYPE_ENUM   = 6,
};

typedef struct {
	uint32_t type;
	uint32_t flags;
	uint32_t class_index;
	uint32_t name_index;
	uint32_t member_index;
	uint32_t class_handle;
	uint32_t data_index;
	// uint32_t unk1C;
} SobRefEntry;

typedef struct {
	const char *class_name;
	uint32_t class_handle;
	uint32_t parent_handle;
	int frameworks_count;
	uint32_t *frameworks_data;
	int autoload_count;
	uint32_t *autoload_data;
	int default_membervars_count;
	SobVar *default_membervars_data;
	int staticvars_count;
	SobVar *staticvars_data;
	int codeentries_count;
	SobCodeEntry *codeentries_data;
	int local_count;
	uint8_t *local_data;
	int refentries_count;
	SobRefEntry *refentries_data;
	int stringentries_count;
	uint32_t *stringentries_data;
	int strings_size;
	uint8_t *strings_data;
	int code_size;
	uint8_t *code_data;
	uint8_t fixup_flag;
} SobData;

SobData *LoadSob(const uint8_t *data, int size, int *offset, const char *filename);
void UnloadSob(SobData *sob);

int Sob_FindMember(SobData *sob, const char *name);
int Sob_FindMethod(SobData *sob, const char *name);
int Sob_FindStatic(SobData *sob, const char *name);
SobRefEntry *Sob_GetRefClass(SobData *sob, int num);
SobRefEntry *Sob_GetRefMethod(SobData *sob, int num);
SobRefEntry *Sob_GetRefStatic(SobData *sob, int num);
SobRefEntry *Sob_GetRefMember(SobData *sob, int num);
const char *Sob_GetString(SobData *sob, int num);
SobCodeEntry *Sob_GetCode(SobData *sob, int num);
SobVar *Sob_GetStaticVar(SobData *sob, int num);

#endif /* SOB_H__ */
