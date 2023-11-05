#!/usr/bin/env python3

import os.path
import struct
import sys

# save sauce source files
DUMP_SAU_FILES = True

# disassemble bytecode
DISASM_BYTECODE = True

# ignore newer opcodes
IGNORE_V1_OPCODES = False

# autorun, mooptreasure, ollofair, sonnyrace
OPCODES_V0 = {
	0x00 : 'nop',
	0x01 : 'breakhere',
	0x93 : 'breakmany',
	0x94 : 'breaktime',
	0x02 : 'jump(i', # jump %d
	0x03 : 'debugprint',
	0x04 : 'end',
	0x05 : 'return',
	0x06 : 'push_int8(b', # push_int8 #%d
	0x07 : 'push_int32(i', # push_int32 #%d
	0x3c : 'push_string(i', # push_string @%d
	0x4c : 'swap',
	0x6c : 'dup',
	0x08 : 'push_local(i', # push_local %d
	0x09 : 'push_me(i', # push_me @%d
	0x0a : 'push_member(i', # push_member @%d
	0x0b : 'push_static_me(i', # push_static_me @%d
	0x0c : 'push_static(i', # push_static @%d
	0x0d : 'pop',
	0x0e : 'pop_local(i', # pop_local %d
	0x0f : 'pop_me(i', # pop_me @%d
	0x10 : 'pop_member(i', # pop_member @%d
	0x11 : 'pop_static_me(i', # pop_static_me @%d
	0x12 : 'pop_static(i', # pop_static @%d
	0x32 : 'push_local[](i', # push_local[] %d
	0x33 : 'push_me1(i', # push_me1 @%d
	0x34 : 'push_member[](i', # push_member[] @%d
	0x35 : 'push_static_me[](i', # push_static_me[] @%d
	0x36 : 'push_static[](i', # push_static[] @%d
	0xb8 : 'push_raw_local[](i', # push_raw_local[] %d
	0x37 : 'pop_local[](i', # pop_local[] %d
	0x38 : 'pop_me[](i', # pop_me[] @%d
	0x39 : 'pop_member[](i', # pop_member[] @%d
	0x3a : 'pop_static_me[](i', # pop_static_me[] @%d
	0x3b : 'pop_static[](i', # pop_static[]
	0x50 : 'push_local[,](i', # push_local[,] %d
	0x51 : 'push_me[,](i', # push_me[,] %d
	0x52 : 'push_member[,](i', # push_member[,] @%d
	0x53 : 'push_static_me[,](i', # push_static_me[,] @%d
	0x54 : 'push_static[,](i', # push_static[,] @%d
	0x55 : 'pop_local[,](i', # pop_local[,] %d
	0x56 : 'pop_me[,](i', # pop_me[,] @%d
	0x57 : 'pop_member[,](i', # pop_member[,] @%d
	0x58 : 'pop_static_me[,](i', # pop_static_me[,] @%d
	0x59 : 'pop_static[,](i', # pop_static[,]
	0x9a : 'push_local[""](i', # push_local[""] %d
	0x9b : 'push_me_map(i', # push_me_map @%d
	0x9c : 'push_member[""](i', # push_member[""] @%d
	0x9d : 'push_static_me[""](i', # push_static_me[""] @%d
	0x9e : 'push_static[""](i', # push_static[""] @%d
	0x9f : 'pop_local[""](i', # pop_local[""] %d
	0xa0 : 'pop_me[""](i', # pop_me[""] @%d
	0xa1 : 'pop_member[""](i', # pop_member[""] @%d
	0xa2 : 'pop_static_me[""](i', # pop_static_me[""] @%d
	0xa3 : 'pop_static[""](i', # pop_static[""]
	0x13 : 'call_me(i', # call_me @%d
	0x14 : 'call_method(i', # call_method @%d
	# 0x1d : 'call_static_me', # unimplemented
	0x15 : 'call_static(i', # call_static @%d
	0x8f : 'call_parent(i', # call_parent @%d
	0x90 : 'start_parent(bi', # start_parent @%d
	0x21 : 'start_me(bi', # start_me @%d
	0x1e : 'start_method(bi', # start_method @%d
	# 0x1f : 'start_static_me' # unimplemented
	0x20 : 'start_static(bi', # start_static @%d
	0x17 : 'new(i', # new @%d
	0x91 : 'new_expr',
	0x76 : 'delete',
	0xad : 'delete_array',
	0x18 : 'add_int',
	0x19 : 'sub_int',
	0x1a : 'mul_int',
	0x1b : 'div_int',
	0x1c : 'neg_int',
	0x48 : 'not_int',
	0x68 : 'min_int',
	0x69 : 'max_int',
	0x6a : 'abs_int',
	0x2a : 'and',
	0x2b : 'or',
	0x2c : 'eq_int',
	0x2d : 'neq_int',
	0x2e : 'leq_int',
	0x2f : 'geq_int',
	0x30 : 'lt_int',
	0x31 : 'gt_int',
	0x6d : 'streq',
	0x28 : 'if_eq(i', # if_eq %d
	0x29 : 'if_neq(i', # if_neq %d
	0xbc : 'iftop_eq(i', # iftop_eq %d
	0xbd : 'iftop_neq(i', # iftop_neq %d
	0x3d : 'add_str',
	0x6e : 'add_chr',
	0xae : 'strcat',
	0x3e : 'syscall(i',
	0x3f : 'fsyscall(i',
	0xb6 : 'fast_syscall(i',
	0xb7 : 'fast_fsyscall(i',
	0x40 : 'dim[]',
	0x4d : 'dim[,]',
	0xb2 : 'dim[int]',
	0x49 : 'debug',
	0xaf : 'assert',
	0x41 : 'quit',
	0x42 : 'col_lower',
	0x43 : 'col_upper',
	0x44 : 'col_size',
	0x5b : 'row_lower',
	0x5c : 'row_upper',
	0x5d : 'row_size',
	0x4a : 'copy1',
	0x4b : 'range1',
	0x5e : 'range2',
	0xaa : 'array_assignref',
	0xb3 : 'array_assignoob',
	0x6f : 'insert_lower',
	0x70 : 'insert_upper',
	0x71 : 'delete_lower',
	0x72 : 'delete_upper',
	0xa5 : 'delete_index',
	0xa6 : 'insert_after',
	0xa7 : 'insert_before',
	0xa8 : 'array_first',
	0xa9 : 'array_last',
	0xb5 : 'array_rand',
	0x98 : 'paste1',
	0x99 : 'paste2',
	0x92 : 'array_find',
	0x45 : 'mod',
	0x46 : 'rand_int',
	0x8d : 'rand_float',
	0x47 : 'strlen',
	0x60 : 'in_array',
	0x61 : 'poppush_array',
	0x62 : 'band',
	0x63 : 'bor',
	0xb4 : 'bnot',
	0x64 : 'stop',
	0x65 : 'stop_me',
	0x66 : 'running',
	0x67 : 'threadid',
	0x6b : 'thread',
	0x95 : 'setthreadid',
	0x96 : 'setthreadorder',
	0x73 : 'class_handle(i', # class_handle @%d
	0x74 : 'class_name', # class_name @%d
	0x75 : 'class_type', # class_type @%d
	0xb9 : 'classname_handle',
	0xbb : 'is_from',
	0x77 : 'itof',
	0x78 : 'ftoi',
	0x79 : 'itos',
	0x7a : 'stoi',
	0x7b : 'ftos',
	0x7c : 'stof',
	0xba : 'format_string',
	0x7d : 'add_float',
	0x7e : 'sub_float',
	0x7f : 'mul_float',
	0x80 : 'div_float',
	0x81 : 'neg_float',
	0x83 : 'min_float',
	0x84 : 'max_float2',
	0x85 : 'abs_float',
	0x86 : 'eq_float',
	0x87 : 'neq_float',
	0x88 : 'leq_float',
	0x89 : 'geq_float',
	0x8a : 'lt_float',
	0x8b : 'gt_float',
	0x8c : 'push_float(f', # push_float #%f
	0x8e : 'start_callback', # start_callback @%d
	0x97 : 'call_callback', # call_callback @%d
	0xa4 : 'map_index',
	0xab : 'check_index',
	0xac : 'check_index2',
	0xb0 : 'gotothread(i',
	0xb1 : 'gotothread(i',
}

# bubbleblast
OPCODES_V1 = {
	0x61 : 'poppush_array(i',
	0xbe : 'eq_struct(b',
	0xbf : 'neq_struct(b',
	0xc0 : 'suspend_thread', # suspend_thread @%d
	0xc1 : 'resume_thread', # resume_thread @%d
	0xc2 : 'lshf_int',
	0xc3 : 'rshf_int',
	0xc4 : 'between_float',
	0xc5 : 'between_int',
	0xc6 : 'regeq',
	0xc7 : 'regmatch',
}

# wordspiral
OPCODES_V2 = {
	0xc8 : 'push_const_string(i', # push_const_string #%d
	0xcb : 'push_string_handle(i', # push_string_handle #%d
	0xcc : 'delete[*]',
	0xcd : 'format_string2',
	0xce : 'sin',
	0xcf : 'cos',
	0xd0 : 'tan',
}

KEY = 'badgravyday'
SEP = 0xabcdabcd

class ClassTypeInfo:
	def __init__(self, name = ''):
		self.name = name
		self.methods = {}
		self.members = {}

VAR_TYPES = {
	0: 'NONE',
	1: 'INT1',
	2: 'INT2',
	3: 'INT4',
	4: 'BYTE',
	5: 'CHAR',
	6: 'INT16',
	7: 'INT32',
	8: 'FLOAT',
	9: 'OBJECT',
	10: 'STRUCT'
}

def vartype(t):
	suffix = ''
	if (t & 0x10000) != 0:
		t -= 0x10000
		if (t & 0x100) != 0:
			t -= 0x100
			suffix = '[,]'
		else:
			suffix = '[]'
	if (t & 0x200000) != 0:
		t -= 0x200000
	return VAR_TYPES.get(t, 'UNK') + suffix

class CodeEntry:
	def __init__(self, f):
		self.offset = struct.unpack('<I', f.read(4))[0]
		self.flags = struct.unpack('<I', f.read(4))[0]

class MemberVariable:
	def __init__(self, f):
		self.type = struct.unpack('<I', f.read(4))[0]
		x = struct.unpack('<I', f.read(4))[0]
		assert x == 0
		vartype(self.type)

REFERENCE_TYPE_CLASS = 2
REFERENCE_TYPE_METHOD = 3
REFERENCE_TYPE_MEMBER = 4
REFERENCE_TYPE_STATIC = 5
REFERENCE_TYPE_ENUM = 6

class Reference:
	def __init__(self, f):
		self.type = struct.unpack('<I', f.read(4))[0]
		self.flags = struct.unpack('<I', f.read(4))[0]
		self.class_index = struct.unpack('<I', f.read(4))[0]
		self.name_index = struct.unpack('<I', f.read(4))[0]
		unk10 = struct.unpack('<I', f.read(4))[0]
		assert unk10 == 0
		self.data_index = struct.unpack('<I', f.read(4))[0]
		unk18 = struct.unpack('<I', f.read(4))[0]
		assert unk18 == 0

class StaticVariable:
	def __init__(self, f):
		self.type = struct.unpack('<I', f.read(4))[0]
		f.read(4)

def read_cstr(f):
	s = b''
	while True:
		c = f.read(1)
		if c == b'\x00':
			break
		s += c
	return s.decode('ascii', errors='ignore')

def h(key, line, pos, code):
	if code >= 0x20:
		code ^= (ord(key[pos % len(key)]) + (line >> 2) + (pos ^ 0xa) + line) & 0x1f
	return code

def decrypt(b, line, key):
	return ''.join(chr(h(key, line, i, code)) for i, code in enumerate(b))

def decode_bytecode(data, size, strings, methods, classes, references, parent_index):
	entry = 0
	offset = 0
	while offset < size:
		if offset in methods:
			print('// %s flags:0x%x' % (methods[offset][0], methods[offset][1]))
		op = data[offset]
		s = '[%04x] (%02x) ' % (offset, op)
		offset += 1
		insn = OPCODES_V1.get(op) if not IGNORE_V1_OPCODES else None
		if not insn:
			insn = OPCODES_V0.get(op)
		if not insn:
			insn = OPCODES_V2.get(op)
		if not insn:
			print('Unknown opcode 0x%02x' % op)
			continue
		assert insn
		args = insn.find('(')
		if args != -1:
			s += insn[:args]
			for arg in insn[args + 1:]:
				if arg == 'b':
					num = data[offset]
					s += ' %d' % num
					offset += 1
				elif arg == 'i':
					num = struct.unpack('<i', data[offset:offset + 4])[0]
					s += ' %d' % num
					offset += 4
				elif arg == 'f':
					num = struct.unpack('<f', data[offset:offset + 4])[0]
					s += ' %f' % num
					offset += 4
			# special handling
			if op == 0x3c: # push_string
				s += ' /* %s */' % strings[num]
			elif op in (0x13, 0x21): # call_me, start_me
				ref = references[num - 1]
				assert ref.class_index == 1
				s += ' /* :%s */' % strings[ref.name_index]
			elif op in (0x15, 0x20): # call_static, start_static
				ref = references[num - 1]
				s += ' /* %s:%s */' % (classes[ref.class_index].name, strings[ref.name_index])
			elif op == 0x8f: # call_parent
				ref = references[num - 1]
				assert parent_index != -1
				s += ' /* %s:%s */' % (classes[parent_index].name, classes[parent_index].methods.get(num))
			elif op in (0x17, 0x73): # new, class_handle
				s += ' /* %s */' % classes[num].name
		else:
			s += insn
		print(s)

def decode_sob(f, fname):
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	a = struct.unpack('<I', f.read(4))[0]
	b = struct.unpack('<I', f.read(4))[0]
	c = struct.unpack('<I', f.read(4))[0]
	d = struct.unpack('<I', f.read(4))[0]
	assert a == 0 and b == 0 and c == 1 and d == 0
	unk10 = struct.unpack('<I', f.read(4))[0]
	# frameworks
	parent_index = -1
	count = struct.unpack('<I', f.read(4))[0]
	assert count == 0 or count == 1
	for i in range(count):
		parent_index = struct.unpack('<I', f.read(4))[0]
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	# autoload list
	count = struct.unpack('<I', f.read(4))[0]
	for i in range(count):
		f.read(4)
	# default member vars
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('members vars count : %d' % count)
	member_vars = [ MemberVariable(f) for i in range(count) ]
	# static vars
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('static vars count : %d' % count)
	static_vars = [ StaticVariable(f) for i in range(count) ]
	# code entries
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('code entries count : %d' % count)
	code_entries = [ CodeEntry(f) for i in range(count) ]
	# local data
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('local data size : %d' % count)
	for i in range(count):
		x = f.read(1)
	# reference entries
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('reference entries count : %d' % count)
	references = [ Reference(f) for i in range(count) ]
	# string entries
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('string entries count : %d' % count)
	for i in range(count):
		f.read(4) # offset in string data
	# string data
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	size = struct.unpack('<I', f.read(4))[0]
	print('string data size : %d' % count)
	bytecode_strings = [ '' ] # array is 1-based
	end = f.tell() + size
	num = 0
	while f.tell() < end:
		s = read_cstr(f)
		print('str #%d "%s"' % (num, s))
		bytecode_strings.append(s)
		num += 1
	bytecode_classes = {}
	# dump references
	for i, ref in enumerate(references):
		name = bytecode_strings[ref.name_index]
		print('Ref #%d type:%d flags:0x%x class_index:%d data_index:%d name:%s' % (i, ref.type, ref.flags, ref.class_index, ref.data_index, name))
		if ref.class_index in bytecode_classes:
			class_typeinfo = bytecode_classes[ref.class_index]
		else:
			class_typeinfo = ClassTypeInfo()
			bytecode_classes[ref.class_index] = class_typeinfo
		if ref.type == REFERENCE_TYPE_CLASS:
			class_typeinfo.name = name
		elif ref.type == REFERENCE_TYPE_METHOD:
			class_typeinfo.methods[ref.data_index] = name
		elif ref.type == REFERENCE_TYPE_MEMBER:
			class_typeinfo.members[ref.data_index] = name
	# code data
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	bytecode_size = struct.unpack('<I', f.read(4))[0]
	bytecode_data = f.read(bytecode_size)
	if DISASM_BYTECODE:
		class_name = bytecode_classes[1].name
		methods = { e.offset : (class_name + ':' + bytecode_classes[1].methods.get(i + 1, ''), e.flags) for i, e in enumerate(code_entries) }
		decode_bytecode(bytecode_data, bytecode_size, bytecode_strings, methods, bytecode_classes, references, parent_index)
	signature = struct.unpack('<I', f.read(4))[0]
	if signature == 0x12345678:
		# source code
		data = f.read()
		if DUMP_SAU_FILES:
			line = 0
			offset = 0
			while offset <= len(data) - 4:
				sep = struct.unpack('<I', data[offset:offset + 4])[0]
				if sep == SEP:
					f.seek((offset + 4) - len(data), os.SEEK_END)
					break
				end = data.find(0xA, offset)
				assert end != -1
				s = decrypt(data[offset:end], line, KEY)
				if line == 0:
					assert s[0] == '$'
					fname = os.path.splitext(fname)[0] + '-' + s[1:]
					o = open(fname, 'w')
				o.write(s + '\n')
				offset = end + 1
				line += 1
	else:
		assert signature == SEP
		signature = struct.unpack('<I', f.read(4))[0]
		assert signature == SEP

for arg in sys.argv[1:]:
	size = os.path.getsize(arg)
	with open(arg, 'rb') as f:
		while f.tell() < size:
			decode_sob(f, arg)
