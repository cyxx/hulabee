#!/usr/bin/env python3

import os.path
import struct
import sys

KEY = 'badgravyday'
SEP = 0xabcdabcd

def read_cstr(f):
        s = b''
        while True:
                c = f.read(1)
                if c == b'\x00':
                        break
                s += c
        return s.decode('ascii')

def decrypt(b, offset, x, key):
	s = ''
	i = 0
	while True:
		code = b[offset + i]
		if code == 0xA:
			break
		if code >= 0x20:
			y = ord(key[i % len(key)]) + (x >> 2) + (i ^ 0xa) + x
			code ^= (y & 0x1f)
		s += chr(code)
		i += 1
	return s

def decode_bytecode(data, size, offsets):
	pass

def decode_sob(f):
	# autoload list?
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	while True:
		sep = struct.unpack('<I', f.read(4))[0]
		if sep == SEP:
			break
	# frameworks?
	count = struct.unpack('<I', f.read(4))[0]
	for i in range(count):
		f.read(4)
	# default member vars?
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	for i in range(count):
		f.read(8)
	# static vars
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('static vars count : %d' % count)
	for i in range(count):
		a = struct.unpack('<I', f.read(4))[0] # type
		b = struct.unpack('<I', f.read(4))[0]
	# code entries
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('code entries count : %d' % count)
	bytecode_offsets = []
	for i in range(count):
		a = struct.unpack('<I', f.read(4))[0] # offset
		b = struct.unpack('<I', f.read(4))[0]
		bytecode_offsets.append(a)
	# local data
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('local data size : %d' % count)
	for i in range(count):
		f.read(1)
	# reference entries
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('reference entries count : %d' % count)
	for i in range(count):
		f.read(28)
	# string entries
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	count = struct.unpack('<I', f.read(4))[0]
	print('string entries count : %d' % count)
	for i in range(count):
		f.read(4)
	# string data
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	size = struct.unpack('<I', f.read(4))[0]
	print('string data size : %d' % count)
	end = f.tell() + size
	num = 0
	while f.tell() < end:
		s = read_cstr(f)
		print('str #%d "%s"' % (num, s))
		num += 1
	# code data
	sep = struct.unpack('<I', f.read(4))[0]
	assert sep == SEP
	bytecode_size = struct.unpack('<I', f.read(4))[0]
	bytecode_data = f.read(bytecode_size)
	decode_bytecode(bytecode_data, bytecode_size, bytecode_offsets)
	signature = struct.unpack('<I', f.read(4))[0]
	if signature == 0x12345678:
		# source code
		data = f.read()
		num = 0
		offset = 0
		while offset < len(data) - 4:
			sep = struct.unpack('<I', data[offset:offset + 4])[0]
			if sep == SEP:
				break
			s = decrypt(data, offset, num, KEY)
			print(s)
			offset += len(s) + 1
			num += 1
	else:
		assert signature == SEP
		signature = struct.unpack('<I', f.read(4))[0]
		assert signature == SEP

for arg in sys.argv[1:]:
	size = os.path.getsize(arg)
	with open(arg, 'rb') as f:
		while f.tell() < size:
			decode_sob(f)
