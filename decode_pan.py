#!/usr/bin/env python3
#
# Extract/Decode Hulabee Entertainment .pan files
#

import os
import os.path
import struct
import sys

# use original asset filenames if present
USE_PAN_FILENAMES = True

def rand16_gen(r):
	return (r * 0x6255 + 0x3619) & 0xFFFF

def descramble(name):
	print('name:%s' % name)
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
		d[t[i]] = bytes([i])
	return d

ASSET_INI = 0
ASSET_FFIMG = 1
ASSET_ACAN = 2
ASSET_BYTECODE = 3
ASSET_RIFF = 4
ASSET_ID3 = 7
ASSET_STRINGS = 8
ASSET_JPEG = 9
ASSET_PNG = 15
ASSET_VORBIS = 17

ASSET_EXTENSIONS = {
	ASSET_INI: 'ini',
	ASSET_FFIMG: 'img',
	ASSET_ACAN: 'can',
	ASSET_BYTECODE: 'bin',
	ASSET_RIFF: 'wav',
	ASSET_ID3: 'mp3',
	ASSET_STRINGS: 'txt',
	ASSET_JPEG: 'jpg',
	ASSET_PNG: 'png',
	ASSET_VORBIS: 'ogg',
}

class AssetPan:
	def __init__(self, f):
		self.id = struct.unpack("<I", f.read(4))[0]
		self.type = struct.unpack("<I", f.read(4))[0]
		self.offset = struct.unpack("<I", f.read(4))[0]
		self.size = struct.unpack("<I", f.read(4))[0]
		f.read(16) # hash
	def dump(self, f, start, alphabet, fname):
		f.seek(self.offset + start)
		o = open(fname, 'wb')
		b = f.read(self.size)
		for i in range(len(b)):
			o.write(alphabet[b[i]])
		o.close()

def decode_pan(f, filesize, alphabet, dname, bundle):
	assert f.read(4) == b'NAPA'
	size = struct.unpack("<I", f.read(4))[0]
	count = struct.unpack("<I", f.read(4))[0]
	version = struct.unpack("<I", f.read(4))[0]
	print('size:%d count:%d version:%d' % (size, count, version))
	assert version == 3 or version == 5
	if version == 5:
		flags = struct.unpack("<I", f.read(4))[0]
		print('flags:0x%x' % flags)
	f.read(512) # RSA signature ?
	f.read(512) # RSA public key ?
	assets = [ AssetPan(f) for i in range(count) ]
	for i, asset in enumerate(assets):
		fname = '%s-%04d.%s' % (bundle, i, ASSET_EXTENSIONS.get(asset.type, 'dat'))
		start = (assets[i + 1].offset if (i + 1 < len(assets)) else filesize) - (asset.offset + asset.size)
		if start > 1:
			f.seek(asset.offset)
			name = f.read(start)
			try:
				name = name[:-1].decode('ascii')
				print('asset:%d type:%d filename:%s' % (i, asset.type, name))
				if USE_PAN_FILENAMES:
					fname = name
			except:
				start = 0
		asset.dump(f, start, alphabet, os.path.join(dname, fname))
		offset = asset.offset + asset.size

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
	size = os.path.getsize(arg)
	f = open(arg, 'rb')
	decode_pan(f, size, alphabet, name, bundle)
