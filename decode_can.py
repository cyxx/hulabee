#!/usr/bin/env python3

from PIL import Image
import os
import struct
import sys

def read_palette(f, offset, next_offset):
	assert next_offset - offset == 1024
	f.seek(offset)
	rgba = f.read(1024)
	palette = []
	for i in range(0, 1024, 4):
		palette.append(rgba[i + 2])
		palette.append(rgba[i + 1])
		palette.append(rgba[i + 0])
		# assert rgba[i + 3] == 0
	return palette

def decode_can(f, size, fname):
	header = f.read(0x20)
	assert header[:4] == b'NACA'
	version = struct.unpack('<I', header[4:8])[0]
	assert version == 5
	count = struct.unpack('<I', header[0x18:0x1C])[0]
	entries = []
	for x in range(count):
		offset = struct.unpack('<I', f.read(4))[0]
		num    = struct.unpack('<I', f.read(4))[0]
		entries.append((offset, num))
	palette = struct.unpack('<I', f.read(4))[0]
	num     = struct.unpack('<I', f.read(4))[0]
	while num > palette:
		palette = num
		num     = struct.unpack('<I', f.read(4))[0]
	assert num < palette
	offsets = []
	while f.tell() < entries[0][0]:
		offset = struct.unpack('<I', f.read(4))[0]
		offsets.append(offset)
	palette = read_palette(f, palette, offsets[0])
	for i, offset in enumerate(offsets):
		sz = (offsets[i + 1] if (i + 1 < len(offsets)) else size) - offset
		f.seek(offset)
		decode_bin(f, sz, palette, fname, i)

def decode_bin(f, size, palette, fname, num):
	w = struct.unpack('<I', f.read(4))[0]
	h = struct.unpack('<I', f.read(4))[0]
	if w == 0 or h == 0:
		return
	c = f.read(4)
	if c != b'8LRX':
		print('Unhandled compression ' + str(c))
		return
	image = Image.new('P', (w, h))
	size = struct.unpack('<I', f.read(4))[0]
	data = f.read(size)
	scanlines = [ struct.unpack('<I', data[y * 4:(y + 1) * 4])[0] for y in range(h) ]
	buf = [ 0 ] * (w * h)
	dst = 0
	for y in range(h):
		next_offset = scanlines[y + 1] if (y + 1 < h) else len(data)
		offset = scanlines[y]
		while offset < next_offset:
			code = data[offset]
			offset += 1
			if code == 0:
				count = data[offset]
				offset += 1
				dst += count
			elif code == 1:
				count = data[offset]
				offset += 1
				color = data[offset]
				offset += 1
				for x in range(count):
					buf[dst + x] = color
				dst += count
			elif code == 2 or code == 4:
				count = data[offset]
				for x in range(count):
					buf[dst + x] = data[offset + 1 + x]
				dst += count
				offset += 1 + count
		assert offset == next_offset
	image.putpalette(palette)
	image.putdata(buf)
	image.save(os.path.splitext(fname)[0] + '-%04x.png' % num)

for arg in sys.argv[1:]:
	size = os.path.getsize(arg)
	f = open(arg, 'rb')
	decode_can(f, size, arg)
