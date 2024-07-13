
#include "sob.h"
#include "util.h"

static const uint32_t SEP_TAG = 0xabcdabcd;

SobData *LoadSob(const uint8_t *data, int size, int *offset, const char *filename) {
	SobData *sob = (SobData *)calloc(1, sizeof(SobData));
	if (!sob) {
		error("Failed to allocate SobData");
	} else {
		int sep, count;

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("1Bad file format in %s", filename);

		*offset += sizeof(uint32_t) * 5;

		count = Read32(data, size, offset);
		sob->frameworks_count = count;
		sob->frameworks_data = (uint32_t *)calloc(count, sizeof(uint32_t));
		if (!sob->frameworks_data) {
			error("Failed to allocate %d SobData.frameworks", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->frameworks_data[i] = Read32(data, size, offset);
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("2Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->autoload_count = count;
		sob->autoload_data = (uint32_t *)calloc(count, sizeof(uint32_t));
		if (!sob->autoload_data) {
			error("Failed to allocate %d SobData.autoload", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->autoload_data[i] = Read32(data, size, offset);
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("3Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->default_membervars_count = count;
		sob->default_membervars_data = (SobVar *)calloc(count + 1, sizeof(SobVar));
		if (!sob->default_membervars_data) {
			error("Failed to allocate %d SobData.default_membervars", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->default_membervars_data[i + 1].type = Read32(data, size, offset);
				sob->default_membervars_data[i + 1].value = Read32(data, size, offset);
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("4Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->staticvars_count = count;
		sob->staticvars_data = (SobVar *)calloc(count + 1, sizeof(SobVar));
		if (!sob->staticvars_data) {
			error("Failed to allocate %d SobData.staticvars", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->staticvars_data[i + 1].type  = Read32(data, size, offset);
				sob->staticvars_data[i + 1].value = Read32(data, size, offset);
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("5Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->codeentries_count = count;
		sob->codeentries_data = (SobCodeEntry *)calloc(count + 1, sizeof(SobCodeEntry));
		if (!sob->codeentries_data) {
			error("Failed to allocate %d SobData.codeentries", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->codeentries_data[i + 1].code_offset = Read32(data, size, offset);
				sob->codeentries_data[i + 1].locals_offset = Read32(data, size, offset);
				sob->codeentries_data[i + 1].class_handle = -1;
				sob->codeentries_data[i + 1].locals_ptr = 0;
				sob->codeentries_data[i + 1].code_ptr = 0;
				// sob->codeentries_data[i + 1].unk14 = i + 1;
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("6Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->local_count = count;
		sob->local_data = (uint8_t *)malloc(count);
		if (!sob->local_data) {
			error("Failed to allocate %d bytes for SobData.local_data", count);
		} else {
			memcpy(sob->local_data, data + *offset, count); *offset += count;
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("7Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->refentries_count = count;
		sob->refentries_data = (SobRefEntry *)calloc(count + 1, sizeof(SobRefEntry));
		if (!sob->refentries_data) {
			error("Failed to allocate %d SobData.refentries", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->refentries_data[i + 1].type = Read32(data, size, offset);
				sob->refentries_data[i + 1].flags = Read32(data, size, offset);
				sob->refentries_data[i + 1].class_index = Read32(data, size, offset);
				sob->refentries_data[i + 1].name_index = Read32(data, size, offset);
				sob->refentries_data[i + 1].member_index = Read32(data, size, offset);
				sob->refentries_data[i + 1].class_handle = 0;
				sob->refentries_data[i + 1].data_index = Read32(data, size, offset);
				*offset += sizeof(uint32_t);
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("8Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->stringentries_count = count;
		sob->stringentries_data = (uint32_t *)calloc(count + 1, sizeof(uint32_t));
		if (!sob->stringentries_data) {
			error("Failed to allocate %d SobData.stringentries", count);
		} else {
			for (int i = 0; i < count; ++i) {
				sob->stringentries_data[i + 1] = Read32(data, size, offset);
			}
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("9Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->strings_size = count;
		sob->strings_data = (uint8_t *)malloc(count);
		if (!sob->strings_data) {
			error("Failed to allocate %d bytes for SobData.strings_data", count);
		} else {
			memcpy(sob->strings_data, data + *offset, count); *offset += count;
		}

		sep = Read32(data, size, offset);
		if (sep != SEP_TAG) error("10Bad file format in %s", filename);

		count = Read32(data, size, offset);
		sob->code_size = count;
		sob->code_data = (uint8_t *)malloc(count);
		if (!sob->code_data) {
			error("Failed to allocate %d bytes for SobData.code_data", count);
		} else {
			memcpy(sob->code_data, data + *offset, count); *offset += count;
		}

		sep = Read32(data, size, offset);
		if (sep == 0x12345678) {
			while (*offset < size) {
				sep = Read32(data, size, offset);
				if (sep == SEP_TAG) {
					break;
				}
				*offset -= 4;
				while (*offset < size) {
					const uint8_t chr = data[*offset]; *offset += 1;
					if (chr == 0xA) {
						break;
					}
				}
			}
		} else {
			if (sep != SEP_TAG) error("11Bad file format in %s", filename);
			sep = Read32(data, size, offset);
			if (sep != SEP_TAG) error("12Bad file format in %s", filename);
		}
	}
	return sob;
}

void UnloadSob(SobData *sob) {
	if (sob) {
		free(sob->frameworks_data);
		free(sob->autoload_data);
		free(sob->default_membervars_data);
		free(sob->staticvars_data);
		free(sob->codeentries_data);
		free(sob->local_data);
		free(sob->refentries_data);
		free(sob->stringentries_data);
		free(sob->strings_data);
		free(sob->code_data);
		free(sob);
	}
}

static int checkClassRefRange(SobData *sob, int num) {
	if (num <= 0 || num > sob->refentries_count) {
		error("Class ref %d out of range (%d..%d)", num, 1, sob->refentries_count);
	}
	return num;
}

static int checkStaticVarRange(SobData *sob, int num) {
	if (num < 0 || num > sob->staticvars_count) {
		error("Static index %d out of range (%d..%d)", num, 0, sob->staticvars_count);
	}
	return num;
}

static int checkCodeEntryRange(SobData *sob, int num) {
	if (num < 0 || num > sob->codeentries_count) {
		error("Code index %d out of range (%d..%d)", num, 0, sob->codeentries_count);
	}
	return num;
}

static int checkMemberRefRange(SobData *sob, int num) {
	if (num < 0 || num > sob->refentries_count) {
		error("Member ref %d out of range (%d..%d)", num, 0, sob->refentries_count);
	}
	return num;
}

static int checkMethodRefRange(SobData *sob, int num) {
	if (num < 0 || num > sob->refentries_count) {
		error("Method ref %d out of range (%d..%d)", num, 0, sob->refentries_count);
	}
	return num;
}

int Sob_FindMember(SobData *sob, const char *s) {
	for (int i = 1; i <= sob->refentries_count; ++i) {
		const SobRefEntry *ref = &sob->refentries_data[i];
		if (ref->class_index == 1 && ref->type == SOB_REFERENCE_TYPE_MEMBER) {
			const int x = ref->name_index;
			const char *name = Sob_GetString(sob, x);
			if (strcmp(s, name) == 0) {
				debug(DBG_SOB, "Found member '%s' index %d", name, i);
				return i;
			}
		}
	}
	return 0;
}

int Sob_FindMethod(SobData *sob, const char *s) {
	for (int i = 1; i <= sob->refentries_count; ++i) {
		const SobRefEntry *ref = &sob->refentries_data[i];
		if (ref->class_index == 1 && ref->type == SOB_REFERENCE_TYPE_METHOD) {
			const int x = ref->name_index;
			const char *name = Sob_GetString(sob, x);
			if (strcmp(s, name) == 0) {
				debug(DBG_SOB, "Found method '%s' index %d", name, i);
				return i;
			}
		}
	}
	return 0;
}

int Sob_FindStatic(SobData *sob, const char *s) {
	for (int i = 1; i <= sob->refentries_count; ++i) {
		const SobRefEntry *ref = &sob->refentries_data[i];
		if (ref->class_index == 1 && ref->type == SOB_REFERENCE_TYPE_STATIC) {
			const int x = ref->name_index;
			const char *name = Sob_GetString(sob, x);
			if (strcmp(s, name) == 0) {
				debug(DBG_SOB, "Found static '%s' index %d", name, i);
				return i;
			}
		}
	}
	return 0;
}

SobRefEntry *Sob_GetRefClass(SobData *sob, int num) {
	checkClassRefRange(sob, num);
	SobRefEntry *ref = &sob->refentries_data[num];
	if (ref->type != SOB_REFERENCE_TYPE_CLASS) {
		error("Class ref %d does not contain a valid Class", num);
	}
	return ref;
}

SobRefEntry *Sob_GetRefMethod(SobData *sob, int num) {
	checkMethodRefRange(sob, num);
	SobRefEntry *ref = &sob->refentries_data[num];
	if (ref->type != SOB_REFERENCE_TYPE_METHOD) {
		error("Method ref %d does not contain a valid Method", num);
	}
	return ref;
}

SobRefEntry *Sob_GetRefStatic(SobData *sob, int num) {
	if (num < 0 || num > sob->refentries_count) {
		error("Variable ref %d out of range (%d..%d)", num, 0, sob->refentries_count);
	}
	SobRefEntry *ref = &sob->refentries_data[num];
	if (ref->type != SOB_REFERENCE_TYPE_STATIC) {
		error("Variable ref @%d does not contain a valid Variable", num);
	}
	return ref;
}

SobRefEntry *Sob_GetRefMember(SobData *sob, int num) {
	checkMemberRefRange(sob, num);
	SobRefEntry *ref = &sob->refentries_data[num];
	if (ref->type != SOB_REFERENCE_TYPE_MEMBER) {
		error("Member ref %d does not contain a valid Member", num);
	}
	return ref;
}

const char *Sob_GetString(SobData *sob, int num) {
	if (num <= 0 || num > sob->stringentries_count) {
		error("String index %d out of range (%d..%d)", num, 1, sob->stringentries_count);
	}
	const char *str = (const char *)sob->strings_data + sob->stringentries_data[num];
	return str;
}

SobCodeEntry *Sob_GetCode(SobData *sob, int num) {
	checkCodeEntryRange(sob, num);
	return &sob->codeentries_data[num];
}

SobVar *Sob_GetStaticVar(SobData *sob, int num) {
	checkStaticVarRange(sob, num);
	return &sob->staticvars_data[num];
}
