#!/usr/bin/env python
#
# Extract/Decode Hulabee Entertainment .pan files
#

import os
import os.path
import struct
import sys

def rand16_gen(r):
	return (r * 0x6255 + 0x3619) & 0xFFFF

def descramble(name):
	print 'name:%s' % name
	t = [ i for i in range(256) ]
	hash = 0
	for i, c in enumerate(name):
		d = ord(c)
		hash += (d << (i & 15)) + d
	r = rand16_gen(hash)
	count = ((r * 10 + 0x8000) >> 16) + 10
	for i in range(count):
		for j in range(256):
			r = rand16_gen(r);
			k = ((r << 8) - r + 0x8000) >> 16;
			assert k >= 0 and k <= 255
			t[j], t[k] = t[k], t[j]
	d = [ 0 for i in range(256) ]
	for i in range(256):
		d[t[i]] = chr(i)
	return d

ASSET_INI = 0
ASSET_FFIMG = 1
ASSET_ACAN = 2
ASSET_BYTECODE = 3
ASSET_RIFF = 4
ASSET_ID3 = 7
ASSET_STRINGS = 8
ASSET_JPEG = 9
ASSET_UNKNOWN = 10

ASSET_EXTENSIONS = {
	ASSET_INI: 'ini',
	ASSET_FFIMG: 'img',
	ASSET_ACAN: 'can',
	ASSET_BYTECODE: 'bin',
	ASSET_RIFF: 'wav',
	ASSET_ID3: 'mp3',
	ASSET_STRINGS: 'txt',
	ASSET_JPEG: 'jpg',
	ASSET_UNKNOWN: 'dat',
}

class AssetPan:
	def __init__(self, f):
		self.id = struct.unpack("<I", f.read(4))[0]
		self.type = struct.unpack("<I", f.read(4))[0]
		self.offset = struct.unpack("<I", f.read(4))[0]
		self.size = struct.unpack("<I", f.read(4))[0]
		f.read(16) # hash
	def dump(self, f, align, alphabet, fname):
		f.seek(self.offset + align)
		o = open(fname, 'wb')
		b = bytearray(f.read(self.size))
		for i in range(len(b)):
			o.write(alphabet[b[i]])
		o.close()

def decode_pan(f, alphabet, dir, bundle):
	assert f.read(4) == 'NAPA'
	size = struct.unpack("<I", f.read(4))[0]
	count = struct.unpack("<I", f.read(4))[0]
	version = struct.unpack("<I", f.read(4))[0]
	print 'size:%d count:%d version:%d' % (size, count, version)
	assert version == 3 or version == 5
	if version == 5:
		flags = struct.unpack("<I", f.read(4))[0]
	f.read(512) # RSA signature ?
	f.read(512) # RSA public key ?
	assets = []
	for i in range(count):
		assets.append(AssetPan(f))
	for i, asset in enumerate(assets):
		align = 1
		if asset.type == ASSET_BYTECODE:
			f.seek(asset.offset)
			sob = f.read(32)
			for i, c in enumerate(sob):
				if ord(c) == 0:
					sob = sob[:i]
					break
			print 'bytecode:%s' % sob
			align = len(sob) + 1 - 32
		fname = '%s-%04d.%s' % (bundle, i, ASSET_EXTENSIONS[asset.type])
		asset.dump(f, align, alphabet, os.path.join(dir, fname))

for arg in sys.argv[1:]:
	name = os.path.basename(arg)
	i = name.rindex('-')
	assert i != -1
	bundle = os.path.splitext(name[i + 1:])[0]
	name = name[:i].lower()
	alphabet = descramble(name)
	try:
		os.mkdir(name)
	except:
		pass
	f = open(arg, 'rb')
	decode_pan(f, alphabet, name, bundle)
