#
# ff7 - Utility package for working with Final Fantasy VII data
#
# Copyright (C) Christian Bauer <www.cebix.net>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#

__author__ = "Christian Bauer <www.cebix.net>"
__version__ = "1.4"

import os
import re
import gzip
import zlib
import struct
import io

from . import game
from . import lzss
from . import binlz
from . import ff7text
from . import kernel
from . import field
from . import tutorial
from . import scene
from . import world
from . import data
from . import cd
from . import models
from . import menu

# Decompress an 8-bit string from GZIP format.
def decompressGzip(data):
    buffer = io.BytesIO(data)
    zipper = gzip.GzipFile(fileobj = buffer, mode = "rb")
    return zipper.read()

# Compress an 8-bit string to GZIP format.
def compressGzip(data):
    buffer = io.BytesIO()
    zipper = zlib.compressobj(zlib.Z_BEST_COMPRESSION, zlib.DEFLATED, -zlib.MAX_WBITS, 6, 0)  # memlevel = 6 seems to produce smaller output

    buffer.write(b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x00")
    buffer.write(zipper.compress(data))
    buffer.write(zipper.flush())
    buffer.write(struct.pack("<L", zlib.crc32(data) & 0xffffffff))
    buffer.write(struct.pack("<L", len(data)))

    return buffer.getvalue()

# Decompress an 8-bit string from LZSS format.
def decompressLzss(data):
    return lzss.decompress(data)

# Compress an 8-bit string to LZSS format.
def compressLzss(data):
    return lzss.compress(data)

# Decode FF7 kernel text string to unicode string.
def decodeKernelText(data, japanese = False):
    return ff7text.decodeKernel(data, japanese)

# Decode FF7 field text string to unicode string.
def decodeFieldText(data, japanese = False):
    return ff7text.decodeField(data, japanese)

# Encode unicode string to FF7 kernel text string.
def encodeKernelText(text, japanese = False):
    return ff7text.encode(text, False, japanese)

# Encode unicode string to FF7 field text string.
def encodeFieldText(text, japanese = False):
    return ff7text.encode(text, True, japanese)

# Calculate the extent of a unicode string.
def textExtent(text, metrics):
    return ff7text.extent(text, metrics)
