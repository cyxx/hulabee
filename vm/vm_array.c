
#include "random.h"
#include "util.h"
#include "vm.h"

VMArray *VM_GetArrayFromHandle(VMContext *c, int num) {
	const int x = num - BASE_HANDLE_ARRAY;
	if (x < 0 || x >= VMARRAYS_COUNT) {
		error("Array handle %d out of range (%d..%d)", num, BASE_HANDLE_ARRAY, BASE_HANDLE_ARRAY + VMARRAYS_COUNT);
	}
	VMArray *array = &c->arrays[x];
	assert(array->handle == num);
	return array;
}

VMArray *Array_New(VMContext *c) {
	assert(c->arrays_next_free != 0);
	const int num = c->arrays_next_free;
	VMArray *array = &c->arrays[num];
	c->arrays_next_free = array->next_free;
	memset(array, 0, sizeof(VMArray));
	array->handle = BASE_HANDLE_ARRAY + num;
	return array;
}

static void initArray(VMArray *array, int type) {
	array->type = type;
	array->struct_size = 0;
	int type2 = type;
	if (type2 & 0x100) {
		type2 &= ~0x100;
		type2 |= 0x10000;
	}
	if (type2 & 0x10000) {
		array->elem_size = 4;
	} else {
		switch (type & 0xFF) {
		case VAR_TYPE_BYTE:
		case VAR_TYPE_CHAR:
			array->elem_size = 1;
			break;
		case VAR_TYPE_INT32:
		case VAR_TYPE_FLOAT:
		case VAR_TYPE_OBJECT:
		case 12:
			array->elem_size = 4;
			break;
		case VAR_TYPE_STRUCT:
			array->struct_size = (type >> 20) & 0xFF;
			assert(array->struct_size != 0);
			array->elem_size = array->struct_size * 4;
			break;
		case VAR_TYPE_INT16:
		case 11:
		default:
			error("Illegal array type 0x%08x", type);
			break;
		}
	}
	array->data = 0;
	array->col_upper = -1;
	array->col_lower = -1;
	array->row_upper = -1;
	array->row_lower = -1;
	array->offset = 0;
}

static void checkArrayTypeString(VMArray *array) {
	if (array->type != VAR_TYPE_CHAR) {
		error("Array %d is not a string", array->handle);
	}
}

void Array_Dim(VMArray *array, int type, int col_lower, int col_upper) {
	initArray(array, type);
	array->col_upper = col_upper;
	array->col_lower = col_lower;
	const int size = col_upper - col_lower + 1;
	array->data = (uint8_t *)calloc(size, array->elem_size);
	if (!array->data) {
		error("Failed to allocate %d bytes in Array_Dim", size * array->elem_size);
	}
}

void Array_Dim2(VMArray *array, int type, int row_lower, int row_upper, int col_lower, int col_upper) {
	initArray(array, type);
	array->row_upper = row_upper;
	array->col_upper = col_upper;
	array->row_lower = row_lower;
	array->col_lower = col_lower;
	array->dimension = 2;
	const int size = (row_upper - row_lower + 1) * (col_upper - col_lower + 1);
	array->data = (uint8_t *)calloc(size, array->elem_size);
	if (!array->data) {
		error("Failed to allocate %d bytes in Array_Dim2", size * array->elem_size);
	}
}

void Array_SetString(VMArray *array, const char *s) {
	initArray(array, VAR_TYPE_CHAR);
	array->col_lower = 1;
	array->dimension = 1;
	array->col_upper = (strlen(s) + 1);
	array->data = (uint8_t *)calloc(array->col_upper, array->elem_size);
	if (!array->data) {
		error("Failed to allocate %d bytes in Array_SetString", array->col_upper * array->elem_size);
	} else {
		memcpy(array->data, s, array->col_upper);
	}
}

int Array_Get(VMArray *array, int offset) {
	if (array->is_key_value) {
		for (int i = 0; i < array->kv_size; ++i) {
			if (array->kv_data[i].key == offset) {
				return array->kv_data[i].value;
			}
		}
		return 0;
	}
	switch (array->elem_size) {
	case 1:
		return array->data[array->offset - array->col_lower + offset];
	case 4:
		return READ_LE_UINT32(array->data + (array->offset - array->col_lower + offset) * sizeof(uint32_t));
	default:
		error("Array %d data size (%d) is illegal", array->handle, array->elem_size);
	}
	return 0;
}

void Array_Set(VMArray *array, int offset, int value) {
	if (array->is_key_value) {
		for (int i = 0; i < array->kv_size; ++i) {
			if (array->kv_data[i].key == offset) {
				array->kv_data[i].value = value;
				return;
			}
		}
		array->kv_data = (struct vmarray_key_value_t *)realloc(array->kv_data, (array->kv_size + 1) * sizeof(struct vmarray_key_value_t));
		if (array->kv_data) {
			array->kv_data[array->kv_size].key = offset;
			array->kv_data[array->kv_size].value = value;
			++array->kv_size;
		}
		return;
	}
	switch (array->elem_size) {
	case 1:
		array->data[array->offset - array->col_lower + offset] = value;
		break;
	case 4:
		WRITE_LE_UINT32(array->data + (array->offset - array->col_lower + offset) * sizeof(uint32_t), value);
		break;
	default:
		error("Array %d data size (%d) is illegal", array->handle, array->elem_size);
	}
}

int Array_Find(VMArray *array, int value) {
	for (int i = array->col_lower; i <= array->col_upper; ++i) {
		if (Array_Get(array, i) == value) {
			return i;
		}
	}
	return 0;
}

int Array_DeleteIndex(VMArray *array, int start, int end) {
	assert(!array->is_key_value);
	assert(start <= end);
	if (start < array->col_lower) {
		start = array->col_lower;
	}
	if (end > array->col_upper) {
		end = array->col_upper;
	}
	const int count = end - start + 1;
	for (int i = start; i + count <= array->col_upper; ++i) {
		Array_Set(array, i, Array_Get(array, i + count));
	}
	array->col_upper -= count;
	if (array->type == VAR_TYPE_CHAR) {
		Array_Set(array, array->col_upper, 0);
	}
	return 1;
}

int Array_CheckIndex(VMArray *array, int index) {
	return array->dimension != 2 && index >= array->col_lower && index <= array->col_upper;
}

static void initUnk28(VMArray *array, int before, int after) {
	const int new_size = array->col_upper - array->col_lower + before;
	uint8_t *p = (uint8_t *)calloc(array->elem_size, new_size + after + 1);
	if (!p) {
		error("Failed to allocate Array.unk28 buffer");
	} else {
		const int original_size = array->col_upper - array->col_lower + 1;
		const int offset_in_bytes = array->elem_size * array->offset;
		const int size_in_bytes = original_size * array->elem_size;
		const int start_offset_in_bytes = array->elem_size * before;
		uint8_t *prev = array->data;
		memcpy(p + start_offset_in_bytes, array->data + offset_in_bytes, size_in_bytes);
		array->data = p;
		array->offset = before;
		array->unk28 = after;
		free(prev);
	}
}

void Array_InsertUpper(VMArray *array, int value) {
	if (array->unk28 == 0) {
		initUnk28(array, 100, 100);
	}
	++array->col_upper;
	--array->unk28;
	if (array->type == VAR_TYPE_CHAR) {
		Array_Set(array, array->col_upper - 1, value);
		Array_Set(array, array->col_upper, 0);
	} else {
		Array_Set(array, array->col_upper, value);
	}
	assert(array->struct_size == 0);
}

int Array_DeleteLower(VMArray *array) {
	error("Unimplemented Array_DeleteLower");
	return 0;
}

int Array_DeleteUpper(VMArray *array) {
	if (array->col_lower > array->col_upper || (array->type == VAR_TYPE_CHAR && array->col_lower == array->col_upper)) {
		return 0;
	}
	const int value = Array_Get(array, array->col_upper);
	Array_Set(array, array->col_upper, 0);
	--array->col_upper;
	++array->unk28;
	return value;
}

int Array_GetStringLength(VMArray *array) {
	if (array->type != VAR_TYPE_CHAR) {
		error("Can't do string operations on non-string array %d", array->handle);
	} else if (array->dimension == 2) {
		error("Accessing [n,n] as [n]");
	} else {
		assert(array->is_key_value == 0);
		int len = 0;
		for (int i = array->col_lower; i <= array->col_upper; ++i) {
			if (Array_Get(array, i) == 0) {
				return len;
			}
			++len;
		}
		error("No EOS on string %d", array->handle);
	}
	return 0;
}

int Array_Copy1(VMContext *c, VMArray *array) {
	if (array->dimension == 1) {
		VMArray *array2 = Array_New(c);
		const int lower = array->col_lower;
		const int upper = array->col_upper;
		Array_Dim(array2, array->type, 1, upper - lower + 1);
		for (int x = lower; x <= upper; ++x) {
			const int val = Array_Get(array, x);
			Array_Set(array2, x, val);
		}
		return array2->handle;
	} else {
		error("Array_Copy1 unimplemented dimension:%d", array->dimension);
	}
	return array->handle;
}

int Array_Range1(VMContext *c, VMArray *array, int start, int end) {
	if (array->dimension == 2) {
		error("Accessing [n,n] as [n]");
	}
	assert(start <= end);
	int lower = start;
	if (lower < array->col_lower) {
		lower = array->col_lower;
	}
	int upper = end;
	if (upper > array->col_upper) {
		upper = array->col_upper;
	}
	VMArray *array2 = Array_New(c);
	Array_Dim(array2, array->type, 1, upper - lower + 1);
	for (int x = lower; x <= upper; ++x) {
		const int val = Array_Get(array, x);
		Array_Set(array2, x - lower + 1, val);
	}
	return array2->handle;
}

int Array_Rand(VMArray *array) {
	int lower = array->col_lower;
	int upper = array->col_upper;
	if (array->type == VAR_TYPE_CHAR) {
		--upper;
	}
	int x = GetRandomNumber(lower, upper);
	if (x == array->unk40) {
		++x;
		if (x > upper) {
			x = upper;
		}
	}
	array->unk40 = x;
	assert(array->struct_size == 0);
	return Array_Get(array, x);
}

const char *ArrayHandle_GetString(VMContext *c, int num) {
	VMArray *array = VM_GetArrayFromHandle(c, num);
	return (const char *)array->data + array->offset;
}

int ArrayHandle_CompareString(VMContext *c, int array1, int array2) {
	VMArray *a1 = VM_GetArrayFromHandle(c, array1);
	checkArrayTypeString(a1);
	VMArray *a2 = VM_GetArrayFromHandle(c, array2);
	checkArrayTypeString(a2);
	const char *s1 = (const char *)a1->data + a1->offset;
	const char *s2 = (const char *)a2->data + a2->offset;
	return strcmp(s1, s2);
}

void ArrayHandle_ConcatString(VMContext *c, int array1, int array2) {
	VMArray *a1 = VM_GetArrayFromHandle(c, array1);
	checkArrayTypeString(a1);
	VMArray *a2 = VM_GetArrayFromHandle(c, array2);
	checkArrayTypeString(a2);
	const int len = Array_GetStringLength(a2);
	initUnk28(a1, 0, len);
	const uint8_t *src = a2->data + a2->offset * a2->elem_size;
	uint8_t *dst = a1->data + (a1->offset + a1->col_upper - 1) * a1->elem_size;
	memcpy(dst, src, len * a1->elem_size);
	a1->col_upper += len;
	a1->unk28 -= len;
}

int ArrayHandle_AddString(VMContext *c, int array1, int array2) {
	VMArray *a1 = VM_GetArrayFromHandle(c, array1);
	checkArrayTypeString(a1);
	const int s1_len = Array_GetStringLength(a1);
	VMArray *a2 = VM_GetArrayFromHandle(c, array2);
	checkArrayTypeString(a2);
	const int s2_len = Array_GetStringLength(a2);
	VMArray *array = Array_New(c);
	Array_Dim(array, VAR_TYPE_CHAR, 1, s1_len + s2_len + 1);
	int x = array->col_lower;
	assert(a1->is_key_value == 0);
	for (int i = a1->col_lower; i < a1->col_upper; ++i) {
		const int val = Array_Get(a1, i);
		Array_Set(array, x++, val);
	}
	for (int i = a2->col_lower; i < a2->col_upper; ++i) {
		const int val = Array_Get(a2, i);
		Array_Set(array, x++, val);
	}
	Array_Set(array, x, 0);
	return array->handle;
}

void ArrayHandle_LowerString(VMContext *c, int array) {
	VMArray *a = VM_GetArrayFromHandle(c, array);
	checkArrayTypeString(a);
	for (char *p = (char *)a->data + a->offset; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

void ArrayHandle_UpperString(VMContext *c, int array) {
	VMArray *a = VM_GetArrayFromHandle(c, array);
	checkArrayTypeString(a);
	for (char *p = (char *)a->data + a->offset; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}

void ArrayHandle_Delete(VMContext *c, int array) {
	if (array != 0) {
		VMArray *a = VM_GetArrayFromHandle(c, array);
		free(a->data);
		a->next_free = c->arrays_next_free;
		c->arrays_next_free = a - c->arrays;
	}
}
