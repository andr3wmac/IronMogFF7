#
# ff7.world - Final Fantasy VII world event script handling
#
# Copyright (C) Christian Bauer <www.cebix.net>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#

import struct
import math
import zlib

from . import lzss

def _enum(**enums):
    return type('Enum', (), enums)

# Some selected opcodes
Op = _enum(
    CLEAR = 0x100,
    PUSHI = 0x110,
    JUMP = 0x200,
    JUMPZ = 0x201,
    WINDOW = 0x324,
    MES = 0x325,
    ASK = 0x326
)

# Find the size (number of 16-bit values) of the given instruction.
def instructionSize(op):
    if op > 0x100 and op < 0x200:  # push
        return 2
    elif op in [Op.JUMP, Op.JUMPZ]:  # jump
        return 2
    else:
        return 1

# World map TXZ file
class WorldMap:

    # Parse the map from an open file object.
    def __init__(self, fileobj):

        # Read the file data
        cmpData = fileobj.read()

        # Decompress the file
        compressedSize = struct.unpack_from("<L", cmpData)[0]
        self.data = bytearray(lzss.decompress(cmpData[4:4 + compressedSize]))

        # Find the script section
        offset = struct.unpack_from("<L", self.data, 0x14)[0]
        size = struct.unpack_from("<L", self.data, 0x18)[0] - offset

        self.scriptStart = offset + 4
        self.scriptEnd = self.scriptStart + size

    # Return the script code as a list of 16-bit values.
    def getScript(self):
        script = []

        # Convert code after the entry table until we find a
        # null opcode or reach the end of the data
        offset = self.scriptStart + 0x400
        while offset < self.scriptEnd:
            op = struct.unpack_from("<H", self.data, offset)[0]
            offset += 2

            if op == 0:
                break

            script.append(op)

            for i in range(instructionSize(op) - 1):
                script.append(struct.unpack_from("<H", self.data, offset)[0])
                offset += 2

        return script

    # Insert script code back into the data.
    def setScript(self, script):
        offset = self.scriptStart + 0x400
        for w in script:
            struct.pack_into("<H", self.data, offset, w)
            offset += 2

    # Write the map to a file object, truncating the file.
    def writeToFile(self, fileobj):

        # Compress the map data
        cmpData = lzss.compress(self.data)

        # Write to file
        fileobj.seek(0)
        fileobj.truncate()
        fileobj.write(struct.pack("<L", len(cmpData)))
        fileobj.write(cmpData)

# Map opcodes to mnemonics
opcodes = {
    0x015: "neg",
    0x017: "not",
    0x030: "*",
    0x040: "+",
    0x041: "-",
    0x050: "<<",
    0x051: ">>",
    0x060: "<",
    0x061: ">",
    0x062: "<=",
    0x063: ">=",
    0x070: "==",
    0x071: "!=",
    0x080: "&",
    0x0a0: "|",
    0x0b0: "&&",
    0x0c0: "||",
    0x0e0: "store",
    0x100: "clear",
    0x200: "jump",
    0x201: "jumpz",
    0x203: "return",
    0x324: "window",
    0x325: "mes",
    0x326: "ask"
}

# Disassemble script code.
def disassemble(script):
    s = ""

    i = 0
    while i < len(script):
        s += "%04x: " % i

        op = script[i]
        i += 1

        if op == Op.PUSHI:
            s += "push #%04x" % script[i]
            i += 1
        elif op in [0x114, 0x115, 0x116]:
            s += "push bit %d/%04x" % (op & 3, script[i])
            i += 1
        elif op in [0x118, 0x119, 0x11a]:
            s += "push byte %d/%04x" % (op & 3, script[i])
            i += 1
        elif op in [0x11c, 0x11d, 0x11e]:
            s += "push halfword %d/%04x" % (op & 3, script[i])
            i += 1
        elif op > 0x100 and op < 0x200 and op & 3 == 3:
            s += "push special %04x" % script[i]
            i += 1
        elif op == Op.JUMP:
            s += "%s %04x" % (opcodes[op], script[i])
            i += 1
        elif op == Op.JUMPZ:
            s += "%s %04x" % (opcodes[op], script[i])
            i += 1
        elif op > 0x203 and op < 0x300:
            s += "exec %03x" % op
        else:
            try:
                s += opcodes[op]
            except KeyError:
                s += "<%04x>" % op

        s += "\n"

    return s

class FunctionEntry:
    def __init__(self, data, offset_base):
        # Read header and offset (both uint16le)
        self.header, self.offset = struct.unpack_from("<HH", data, offset_base)

        extra = (self.header >> 14) & 0x3 

        
        self.realOffset = offset_base
        self.realSize = 0

        self.type = self.header >> 14
        if (self.type == 0):
            self.id = self.header & 0xFF
        elif (self.type == 1):
            self.id = self.header & 0xFF
        elif (self.type == 2):
            #print("Extra: " + str(extra))
            self.id = self.header & 0xF
            meshCoords = (self.header >> 4) & 0x3FF
            self.x = math.floor(meshCoords / 36)
            self.y = meshCoords % 36

        # Calculate actual file offset for opcodes
        opcode_offset = 0x400 + self.offset * 2
        self.opcodes = []

        opStack = []
        in_chocobo = False
        was_choco = False

        # Read opcodes until 0x0203 (inclusive)
        while True:
            opcode, = struct.unpack_from("<H", data, opcode_offset)
            self.opcodes.append(opcode)

            if opcode == 0x0318:
                fieldID = opStack[-3]
                #print(fieldID)
                if (fieldID == 3):
                    in_chocobo = True
                    was_choco = True
            else:
                opStack.append(opcode)

            opcode_offset += 2
            self.realSize += 2
            if opcode == 0x0203:
                if in_chocobo:
                    in_chocobo = False
                    break
                else:
                    if (was_choco):
                        last_opcode, = struct.unpack_from("<H", data, opcode_offset)
                        #print("Last OpCode: " + str(last_opcode))
                        #print("Header Value: " + str(self.header))
                    break
        
        if was_choco:
            for opcode in self.opcodes:
                if (opcode >= 0x204 and opcode <= 0x22F):
                    print("CONTAINS FUNCTION CALL!")
                print(hex(opcode))

        self._size = 4  # header + offset = 4 bytes (not counting opcode block)

class WorldMapEV:

    # Parse the map from an open file object.
    def __init__(self, fileobj):

        # Read the file data
        self.data = fileobj.read()

        print("LENGTH OF EV DATA: " + str(len(self.data)))

        # Decompress the file
        #compressedSize = struct.unpack_from("<L", cmpData)[0]
        #self.data = bytearray(lzss.decompress(cmpData[4:4 + compressedSize]))
        #print(compressedSize)
        #print(len(self.data))

        self.dummyfn, = struct.unpack_from("<I", self.data, 0)
        self.functions = []

        offset = 4  # Skip dummyfn
        fn_offset = 0
        for _ in range(0xFF):  # 255 entries
            fn = FunctionEntry(self.data, offset)
            #if (fn.type == 2):
            fn.realOffset = fn_offset
            fn_offset += fn.realSize
            self.functions.append(fn)
            offset += fn._size  # advance by 4 bytes per entry

class WorldBin:
    def __init__(self, fileobj):
        cmpData = fileobj.read()

        (size,) = struct.unpack_from("<I", cmpData, 0)
        self.data = zlib.decompress(cmpData[8:], 16 | 15)
        if len(self.data) != size:
            print("Failed to decompress world bin data.")
            return
        
        encountersOffset = 0x1D9E8

        self.encounterTables = []
        for r in range(16):
            region = []

            for s in range(4):
                enc_set = []

                table_offset = encountersOffset + (r * 128) + (s * 32)
                set_enabled = struct.unpack_from("<B", self.data, table_offset)[0]
                
                if set_enabled == 1:
                    for i in range(14):
                        data_offset = table_offset + 2 + (i * 2)
                        encounter = struct.unpack_from("<H", self.data, data_offset)[0]
                        enc_set.append(encounter)

                region.append(enc_set)

            self.encounterTables.append(region)
