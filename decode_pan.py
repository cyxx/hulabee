#!/usr/bin/env python3
#
# Extract/Decode Hulabee Entertainment .pan files and Beep Industries .gg files
#

import base64
import hashlib
import os
import os.path
import struct
import sys

# use original asset filenames if present
USE_ASSET_FILENAMES = True

# verify MD5 hash of assets
VERIFY_ASSET_HASH = True

# verify RSA signature of the pan file header
VERIFY_PAN_SIGNATURE = True

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
	d = [ 0 ] * 256
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
	ASSET_BYTECODE: 'sob',
	ASSET_RIFF: 'wav',
	ASSET_ID3: 'mp3',
	ASSET_STRINGS: 'txt',
	ASSET_JPEG: 'jpg',
	ASSET_PNG: 'png',
	ASSET_VORBIS: 'ogg',
}

def bswap_digest(d):
	for x in range(0, len(d), 4):
		d[x], d[x + 1], d[x + 2], d[x + 3] = d[x + 3], d[x + 2], d[x + 1], d[x]

class AssetPan:
	def __init__(self, f):
		self.id = struct.unpack("<I", f.read(4))[0]
		self.type = struct.unpack("<I", f.read(4))[0]
		self.offset = struct.unpack("<I", f.read(4))[0]
		self.size = struct.unpack("<I", f.read(4))[0]
		self.hash = bytearray(f.read(16))
		bswap_digest(self.hash)
	def dump(self, f, start, alphabet, fname):
		f.seek(self.offset + start)
		b = f.read(self.size)
		if VERIFY_ASSET_HASH:
			h = hashlib.md5(b)
			assert self.hash == h.digest()
		with open(fname, 'wb') as o:
			for x in b:
				o.write(alphabet[x])
			o.close()

# 4 bytes per header field, 512 bytes per signature longint
PAN_HEADER_SIZE_V5 = 5 * 4 + 2 * 512

# 4 bytes per field, 16 bytes for hash
PAN_ENTRY_SIZE = 4 * 4 + 16

def decode_lint(data, offset):
	count = struct.unpack('<H', data[offset:offset + 2])[0]
	offset += 2
	return (int.from_bytes(data[offset:offset + count * 2], 'little'), (count + 1) * 2)

class RsaSignature:
	def __init__(self, f):
		self.signature = base64.b64decode(f.read(512))
		self.publickey = base64.b64decode(f.read(512))
	def verify(self, f, count):
		signature, size = decode_lint(self.signature, 0)
		offset = 0
		pubkeyexp, size = decode_lint(self.publickey, offset)
		offset += size
		pubkeymod, size = decode_lint(self.publickey, offset)
		offset += size
		modbitlen = struct.unpack('<H', self.publickey[offset:offset + 2])[0]
		offset += 2
		modbytelen = struct.unpack('<H', self.publickey[offset:offset + 2])[0]
		# PKCS1 - block type : LE16, padding (\xff), zero byte, digest (RIPEMD-160)
		message = pow(signature, pubkeyexp, pubkeymod)
		H1 = message.to_bytes(modbytelen, 'big')
		size = PAN_HEADER_SIZE_V5 + count * PAN_ENTRY_SIZE
		f.seek(0)
		data = bytearray(f.read(size))
		# exclude signature data from the hash (including public key)
		data[20:1044] = [ 0 ] * 1024
		try:
			h = hashlib.new('ripemd160')
			h.update(data)
			H2 = h.digest()
			assert H1[-20:] == H2
		except:
			pass

def read_cstr(f):
	s = b''
	while True:
		c = f.read(1)
		if c == b'\x00':
			break
		s += c
	return s.decode('ascii', errors='ignore')

def decode_pan(f, alphabet, dirname, bundle):
	assert f.read(4) == b'NAPA'
	size = struct.unpack("<I", f.read(4))[0]
	count = struct.unpack("<I", f.read(4))[0]
	version = struct.unpack("<I", f.read(4))[0]
	print('size:%d count:%d version:%d' % (size, count, version))
	assert version == 3 or version == 5
	if version == 5:
		flags = struct.unpack("<I", f.read(4))[0]
		print('flags:0x%x' % flags)
		# &1: read external signature public key
	sign = RsaSignature(f)
	assets = [ AssetPan(f) for i in range(count) ]
	if VERIFY_PAN_SIGNATURE and version == 5:
		sign.verify(f, count)
	assets = [ asset for asset in assets if asset.size != 0 ]
	for i, asset in enumerate(assets):
		f.seek(asset.offset)
		assetname = read_cstr(f)
		if assetname and USE_ASSET_FILENAMES:
			print('asset:%d type:%d filename:%s' % (i, asset.type, assetname))
			filename = assetname.replace('/', '_')
		else:
			filename = '%s-%04d.%s' % (bundle, i, ASSET_EXTENSIONS.get(asset.type, 'dat'))
		asset.dump(f, len(assetname) + 1, alphabet, os.path.join(dirname, filename))

GG_HEADER_SIZE = 64

class AssetGg:
	def __init__(self, f):
		self.size = struct.unpack("<I", f.read(4))[0]
		self.id = struct.unpack("<I", f.read(4))[0]
		self.type = struct.unpack("<I", f.read(4))[0]
		self.payload = struct.unpack("<I", f.read(4))[0]
		self.unk10 = struct.unpack("<I", f.read(4))[0]
		self.unk14 = struct.unpack("<I", f.read(4))[0]
		self.hash = bytearray(f.read(16))
		bswap_digest(self.hash)
		print('asset size:%d %d id:%d type:%d' % (self.size, self.payload, self.id, self.type))
		assert self.unk10 == 0 and self.unk14 == 0
		assert self.size > self.payload
		for i in range(7):
			zero = struct.unpack("<I", f.read(4))[0]
			assert zero == 0

def decode_gg(f, alphabet, dirname, bundle, total):
	while f.tell() + 8 < total:
		tag1 = struct.unpack("<I", f.read(4))[0]
		assert tag1 == 0x74648225
		tag2 = struct.unpack("<I", f.read(4))[0]
		assert tag2 == 0x83547502
		asset = AssetGg(f)
		skip = asset.size - asset.payload - GG_HEADER_SIZE
		assert skip > 0
		f.seek(skip, os.SEEK_CUR)
		b = f.read(asset.payload)
		if VERIFY_ASSET_HASH:
			h = hashlib.md5(b)
			assert asset.hash == h.digest()
		filename = '%s-%04d.%s' % (bundle, asset.id, ASSET_EXTENSIONS.get(asset.type, 'dat'))
		with open(os.path.join(dirname, filename), 'wb') as o:
			for x in b:
				o.write(alphabet[x])

for arg in sys.argv[1:]:
	name = os.path.basename(arg)
	i = name.rindex('-')
	assert i != -1
	bundle, ext = os.path.splitext(name[i + 1:])
	name = name[:i].lower()
	alphabet = descramble(name)
	try:
		os.mkdir(name)
	except:
		pass
	with open(arg, 'rb') as f:
		if ext == '.gg':
			total = os.path.getsize(arg)
			decode_gg(f, alphabet, name, bundle, total)
		else:
			decode_pan(f, alphabet, name, bundle)
