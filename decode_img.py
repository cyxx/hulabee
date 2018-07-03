#!/usr/bin/env python

from PIL import Image
import os
import struct
import sys

def decode_img(f, fname):
	header = f.read(12)
	if header[:8] == 'FFIMGAMI':
		unk = struct.unpack('<I', header[8:12])[0]
		image = None
		while True:
			tag = f.read(4)
			if not tag:
				break
			size = struct.unpack('<I', f.read(4))[0]
			print 'tag %s size %d' % (tag, size)
			if tag == 'DAEH':
				assert size == 24
				buf = f.read(size)
				compression = struct.unpack('<I', buf[0:4])[0]
				assert compression == 1
				w = struct.unpack('<I', buf[4:8])[0]
				h = struct.unpack('<I', buf[8:12])[0]
				b1 = struct.unpack('<I', buf[12:16])[0]
				assert b1 == 0x80 or b1 == 0x8000
				b2 = struct.unpack('<I', buf[16:20])[0]
				assert b2 == 0 or b2 == 0x4010
				assert buf[20:24] == 'ENON'
				if b1 == 0x80:
					image = Image.new('P', (w, h))
				else:
					# RGB565
					break
			elif tag == 'TULC':
				assert size == 256 * 4
				rgba = f.read(size)
				palette = []
				for i in xrange(0, 1024, 4):
					palette.append(ord(rgba[i + 1]))
					palette.append(ord(rgba[i + 2]))
					palette.append(ord(rgba[i + 3]))
				image.putpalette(palette)
			elif tag == 'ATAD':
				image.putdata(f.read(size))
			else:
				break
		if image is not None:
			image.save(os.path.splitext(fname)[0] + '.png')

for arg in sys.argv[1:]:
	f = open(arg)
	decode_img(f, arg)
