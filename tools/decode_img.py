#!/usr/bin/env python3

from PIL import Image
import os
import struct
import sys

def decode_img(f, fname):
	header = f.read(12)
	if header[:8] == b'FFIMGAMI':
		unk = struct.unpack('<I', header[8:12])[0]
		image = None
		rgb555 = False
		while True:
			tag = f.read(4)
			if not tag:
				break
			size = struct.unpack('<I', f.read(4))[0]
			# print 'tag %s size %d' % (tag, size)
			if tag == b'DAEH':
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
				assert buf[20:24] == b'ENON'
				if b1 == 0x80:
					image = Image.new('P', (w, h))
				else:
					rgb555 = True
					image = Image.new('RGB', (w, h))
			elif tag == b'TULC':
				assert size == 256 * 4
				rgba = f.read(size)
				palette = []
				for i in range(0, 1024, 4):
					palette.append(rgba[i + 1])
					palette.append(rgba[i + 2])
					palette.append(rgba[i + 3])
				image.putpalette(palette)
			elif tag == b'ATAD':
				if rgb555:
					print('convert rgb555')
					data = []
					for i in range(0, size, 2):
						color = struct.unpack('<H', f.read(2))[0]
						r = (color >> 10) << 3
						g = ((color >> 5) & 0x1f) << 3
						b = (color & 0x1f) << 3
						data.append((b << 16) | (g << 8) | r)
					image.putdata(data)
				else:
					image.putdata(f.read(size))
			else:
				break
		if image is not None:
			image.save(os.path.splitext(fname)[0] + '.png')

for arg in sys.argv[1:]:
	with open(arg, 'rb') as f:
		decode_img(f, arg)
