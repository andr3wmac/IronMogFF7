#
# ff7.field - Final Fantasy VII field map and script handling
#
# Copyright (C) Christian Bauer <www.cebix.net>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#

import struct

from . import lzss
from . import ff7text


def _enum(**enums):
    return type('Enum', (), enums)


# Field map file sections
Section = _enum(EVENT = 0, WALKMESH = 1,
                TILEMAP = 2, CAMERA = 3,
                TRIGGER = 4, ENCOUNTER = 5,
                MODEL = 6, NUM_SECTIONS = 7)


# Field map data file
class MapData:

    # Parse the field map from an open file object.
    def __init__(self, fileobj):

        # Read the file data
        data = fileobj.read()

        # Decompress the file
        compressedSize = struct.unpack_from("<L", data)[0]
        data = lzss.decompress(data[4:4 + compressedSize])

        # Parse the pointer table
        numSections = 7
        tableSize = numSections * 4

        pointers = struct.unpack_from("<%dL" % numSections, data)

        self.basePointer = pointers[0]
        pointers += (self.basePointer + len(data) - tableSize, )  # dummy pointer to determine end of last section

        # Extract the section data (assumption: the pointers are in
        # ascending order, so the size of each section equals the difference
        # between adjacent pointers)
        self.sections = []
        self.sectionStarts = []
        for i in range(len(pointers) - 1):
            start = pointers[i] - self.basePointer + tableSize
            end = pointers[i + 1] - self.basePointer + tableSize
            assert end >= start

            self.sections.append(data[start:end])
            self.sectionStarts.append(start)

    # Retrieve the event section data.
    def getEventSection(self):
        return EventSection(self.sections[Section.EVENT])
    
    def getTriggerSection(self):
        return TriggerSection(self.sections[Section.TRIGGER])

    def getTriggerSectionStart(self):
        return self.sectionStarts[Section.TRIGGER]

    def getModelSection(self):
        return ModelSection(self.sections[Section.MODEL])

    # Replace the event section data.
    def setEventSection(self, event):
        data = event.getData()

        # Align section size to multiple of four
        if len(data) % 4:
            data.extend(b'\0' * (4 - len(data) % 4))

        self.sections[Section.EVENT] = data

    # Write the map to a file object, truncating the file.
    def writeToFile(self, fileobj):
        mapData = bytearray()

        # Create the pointer table
        pointer = self.basePointer
        for data in self.sections:
            mapData.extend(struct.pack("<L", pointer))
            pointer += len(data)

        # Append the sections
        for data in self.sections:
            mapData.extend(data)

        # Compress the map data
        cmpData = lzss.compress(mapData)

        # Write to file
        fileobj.seek(0)
        fileobj.truncate()
        fileobj.write(struct.pack("<L", len(cmpData)))
        fileobj.write(cmpData)


# Field map event section
class EventSection:

    # Create an EventSection object from binary data.
    def __init__(self, data):

        # Parse the section header
        headerSize = 32
        self.version, numActors, self.numModels, stringTableOffset, numExtra, self.scale, self.creator, self.mapName = struct.unpack_from("<HBBHHH6x8s8s", data)
        offset = headerSize

        self.creator = self.creator.rstrip(b'\0').decode(encoding = "sjis", errors = "backslashreplace")
        self.mapName = self.mapName.rstrip(b'\0').decode(encoding = "sjis", errors = "backslashreplace")

        # Read the actor names
        self.actorNames = []
        for i in range(numActors):
            name = struct.unpack_from("8s", data, offset)[0]
            offset += 8

            name = name.rstrip(b'\0').decode(encoding = "sjis", errors = "backslashreplace")
            self.actorNames.append(name)

        # Read the extra block (music/tutorial) offset table
        extraOffsets = []
        for i in range(numExtra):
            extraOffset = struct.unpack_from("<L", data, offset)[0]
            offset += 4

            extraOffsets.append(extraOffset)

        extraOffsets.append(len(data))  # dummy offset to determine end of last extra block

        # Read the actor script entry tables (32 entries per actor)
        self.actorScripts = []
        self.scriptEntryAddresses = set()
        for i in range(numActors):
            scripts = list(struct.unpack_from("<32H", data, offset))
            offset += 64

            self.actorScripts.append(scripts)
            self.scriptEntryAddresses |= set(scripts)

        # Read the script code (assumptions: the script data immediately
        # follows the actor script offset table, and the start of the string
        # table marks the end of the script data)
        self.scriptBaseAddress = offset
        self.scriptCode = bytearray(data[offset:stringTableOffset])

        if (len(self.scriptCode) + self.scriptBaseAddress) in self.scriptEntryAddresses:
            self.scriptCode.append(Op.RET)  # the SNW_W field has (unused) pointers after the end of the code

        # The default script of each actor continues after the first RET
        # instruction. In order to include the following code in control
        # flow analyses we add a 33rd element to each script entry table
        # which points to the instruction after the first RET of the
        # default script.
        for i in range(numActors):
            defaultScript = self.actorScripts[i][0]

            codeOffset = defaultScript - self.scriptBaseAddress

            while codeOffset < len(self.scriptCode):
                if self.scriptCode[codeOffset] == Op.RET:
                    entry = codeOffset + self.scriptBaseAddress + 1
                    self.actorScripts[i].append(entry)
                    self.scriptEntryAddresses.add(entry)
                    break
                else:
                    codeOffset += instructionSize(self.scriptCode, codeOffset)

        # Also look for double-RET instructions in regular scripts and
        # add pseudo entry points after them
        for i in range(numActors):
            for j in range(1, 32):
                codeOffset = self.actorScripts[i][j] - self.scriptBaseAddress

                while codeOffset < (len(self.scriptCode) - 2):
                    if self.scriptCode[codeOffset] == Op.RET and self.scriptCode[codeOffset + 1] == Op.RET:
                        entry = codeOffset + self.scriptBaseAddress + 2

                        if entry not in self.scriptEntryAddresses:
                            self.actorScripts[i].append(entry)
                            self.scriptEntryAddresses.add(entry)

                        codeOffset += 2
                    else:
                        codeOffset += instructionSize(self.scriptCode, codeOffset)

                        if (codeOffset + self.scriptBaseAddress) in self.scriptEntryAddresses:
                            break  # stop at next script

        # Read the string offset table
        offset = stringTableOffset
        offset += 2  # the first two bytes are supposed to indicate the number of strings, but this is totally unreliable
        firstOffset = struct.unpack_from("<H", data, offset)[0]
        numStrings = firstOffset // 2 - 1  # determine the number of strings by the first offset instead

        stringOffsets = []
        for i in range(numStrings):
            stringOffsets.append(struct.unpack_from("<H", data, offset)[0])
            offset += 2

        # Read the strings (assumption: each string is 0xff-terminated; we
        # don't use the offsets to calculate string sizes because the
        # strings may overlap, and the offsets may not be in ascending
        # order)
        self.stringData = []
        self.stringWithOffsetData = []
        for o in stringOffsets:
            start = stringTableOffset + o
            end = data.find(b'\xff', start)
            self.stringData.append(data[start:end + 1])
            self.stringWithOffsetData.append((start, end - start, data[start:end + 1]))

        # Read the extra blocks (assumptions: offsets are in ascending order
        # and there is no other data between or after the extra blocks, so
        # the size of each block is the difference between adjacent offsets)
        self.extras = []
        self.extrasWithOffsets = []
        for i in range(numExtra):
            start = extraOffsets[i]
            end = extraOffsets[i + 1]
            assert end >= start

            self.extras.append(data[start:end])
            self.extrasWithOffsets.append({"offset": start, "data": data[start:end]})

    # Return the list of all strings as unicode objects.
    def getStrings(self, japanese = False):
        return [ff7text.decodeField(s, japanese) for s in self.stringData]
    
    # Return a tuple of all offsets and strings as unicode objects
    def getStringsWithOffsets(self, japanese = False):
        return [(stroffset, strlen, ff7text.decodeField(s, japanese)) for stroffset, strlen, s in self.stringWithOffsetData]

    # Replace the entire string list.
    def setStrings(self, stringList, japanese = False):
        self.stringData = [ff7text.encode(s, True, japanese) for s in stringList]

    # Return the list of extra data blocks.
    def getExtras(self):
        return self.extras
    
    def getExtrasWithOffsets(self):
        return self.extrasWithOffsets

    # Replace an extra data block.
    def setExtra(self, index, data):
        self.extras[index] = data

    # Encode event section to binary data and return it.
    def getData(self):
        version = 0x0502
        numActors = len(self.actorNames)
        numExtras = len(self.extras)
        numStrings = len(self.stringData)

        headerSize = 32
        actorNamesSize = numActors * 8
        extraOffsetsSize = numExtras * 4
        scriptTablesSize = numActors * 32 * 2
        scriptCodeSize = len(self.scriptCode)

        stringTableOffset = 32 + actorNamesSize + extraOffsetsSize + scriptTablesSize + scriptCodeSize

        # Create the string table
        stringOffsets = b""
        stringTable = b""

        offset = 2 + numStrings * 2
        for string in self.stringData:
            stringOffsets += struct.pack("<H", offset)
            stringTable += string
            offset += len(string)

        assert numStrings <= 256  # string IDs in MES/ASK/MPNAM commands are one byte only
        stringTable = struct.pack("<H", numStrings & 0xff) + stringOffsets + stringTable

        # Align string table size so the extra blocks are 32-bit aligned
        align = stringTableOffset + len(stringTable)
        if align % 4:
            stringTable += bytes([0]) * (4 - align % 4)

        stringTableSize = len(stringTable)

        # Write the header
        data = bytearray()
        data.extend(struct.pack("<HBBHHH6x8s8s", version, numActors, self.numModels, stringTableOffset, numExtras, self.scale, bytes(self.creator, "sjis"), bytes(self.mapName, "sjis")))

        # Write the actor names
        for name in self.actorNames:
            data.extend(struct.pack("8s", bytes(name, "sjis")))

        # Write the extra block offset table
        offset = stringTableOffset + stringTableSize
        for extra in self.extras:
            data.extend(struct.pack("<L", offset))
            offset += len(extra)

        # Write the actor script entry tables
        for scripts in self.actorScripts:
            for i in range(32):
                data.extend(struct.pack("<H", scripts[i]))

        # Write the script code
        data.extend(self.scriptCode)

        # Write the string table
        data.extend(stringTable)

        # Write the extra blocks
        for extra in self.extras:
            data.extend(extra)

        return data
    
    def findGroupAndScript(self, addr):
        for i, sub in enumerate(self.actorScripts):
            for j in range(len(sub) - 1):
                if sub[j] < addr < sub[j + 1]:
                    return i, j
        return 0, 0

class Range:
    def __init__(self, data, offset):
        self.left, self.top, self.right, self.bottom = struct.unpack_from("<hhhh", data, offset)
        self._size = 8  # bytes consumed

class Vertex:
    def __init__(self, data, offset):
        self.x, self.z, self.y = struct.unpack_from("<hhh", data, offset)
        self._size = 6  # bytes consumed

class Exit:
    def __init__(self, data, offset):
        self.exit_line = [Vertex(data, offset), Vertex(data, offset + 6)]
        self.destination = Vertex(data, offset + 12)
        self.fieldID, = struct.unpack_from("<H", data, offset + 18)
        self.dir, self.dir_copy1, self.dir_copy2, self.dir_copy3 = struct.unpack_from("<BBBB", data, offset + 20)
        self._size = 24  # total size

class Trigger:
    def __init__(self, data, offset):
        self.trigger_line = [Vertex(data, offset), Vertex(data, offset + 6)]
        self.background_parameter, self.background_state, self.behavior, self.soundID = \
            struct.unpack_from("<BBBB", data, offset + 12)
        self._size = 16  # total size

class Arrow:
    def __init__(self, data, offset):
        self.positionX, self.positionZ, self.positionY, self.type = struct.unpack_from("<iiiI", data, offset)
        self._size = 16  # total size

class TriggerSection:
    def __init__(self, data):
        offset = 0

        # name[9]
        self.name, = struct.unpack_from("<9s", data, offset)
        offset += 9

        self.control, = struct.unpack_from("<B", data, offset)
        offset += 1

        self.cameraFocusHeight, = struct.unpack_from("<h", data, offset)
        offset += 2

        self.cameraRange = Range(data, offset)
        offset += self.cameraRange._size

        self.bg_layer1_flag, self.bg_layer2_flag, self.bg_layer3_flag, self.bg_layer4_flag = \
            struct.unpack_from("<BBBB", data, offset)
        offset += 4

        (
            self.bg_layer3_width, self.bg_layer3_height,
            self.bg_layer4_width, self.bg_layer4_height,
            self.bg_layer3_x_related, self.bg_layer3_y_related,
            self.bg_layer4_x_related, self.bg_layer4_y_related,
            self.bg_layer3_x_multiplier_related, self.bg_layer3_y_multiplier_related,
            self.bg_layer4_x_multiplier_related, self.bg_layer4_y_multiplier_related
        ) = struct.unpack_from("<hhhhhhhhhhhh", data, offset)
        offset += 24  # 12 * 2-byte ints

        self.unused = struct.unpack_from("<8s", data, offset)[0]
        offset += 8

        #print("OFFSET TO LIST: " + str(offset));

        # Exit doors[12]
        self.doors = []
        for _ in range(12):
            exit_obj = Exit(data, offset)
            self.doors.append(exit_obj)
            offset += exit_obj._size

        #print("OFFSET AFTER LIST: " + str(offset));

        # Trigger triggers[12]
        self.triggers = []
        for _ in range(12):
            trig = Trigger(data, offset)
            self.triggers.append(trig)
            offset += trig._size

        # display_arrow[12]
        self.display_arrows = list(struct.unpack_from("<12B", data, offset))
        offset += 12

        # Arrow arrows[12]
        self.arrows = []
        for _ in range(12):
            arr = Arrow(data, offset)
            self.arrows.append(arr)
            offset += arr._size

class FieldModel:
    def __init__(self, data, offset):
        self.faceID, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.numBones, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.numParts, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.numAnimations, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.unknown0, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.isNPC, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.unknown1, = struct.unpack_from("<B", data, offset)
        offset += 1
        self.globalModelID, = struct.unpack_from("<B", data, offset)
        offset += 1

class ModelSection:
    def __init__(self, data):
        offset = 0

        self.size, = struct.unpack_from("<h", data, offset)
        offset += 2

        self.modelCount, = struct.unpack_from("<h", data, offset)
        offset += 2

        self.models = []
        for _ in range(self.modelCount):
            arr = FieldModel(data, offset)
            self.models.append(arr)
            offset += 8

# Mnemonic and operand length for each script opcode
opcodes = [
    # 0x00..0x07
    ("ret", 0),    ("req", 2),    ("reqsw", 2),  ("reqew", 2),  ("preq", 2),   ("prqsw", 2),  ("prqew", 2),  ("retto", 1),

    # 0x08..0x0f
    ("join", 1),   ("split", 14), ("sptye", 5),  ("gptye", 5),  ("", -1),      ("", -1),      ("dskcg", 1),  ("spcal", 0),

    # 0x10..0x17
    ("skip", 1),   ("lskip", 2),  ("back", 1),   ("lback", 2),  ("if", 5),     ("lif", 6),    ("if2", 7),    ("lif2", 8),

    # 0x18..0x1f
    ("if2", 7),    ("lif2", 8),   ("", -1),      ("", -1),      ("", -1),      ("", -1),      ("", -1),      ("", -1),

    # 0x20..0x27
    ("mgame", 10), ("tutor", 1),  ("btmd2", 4),  ("btrlt", 2),  ("wait", 2),   ("nfade", 8),  ("blink", 1),  ("bgmovie", 1),

    # 0x28..0x2f
    ("kawai", 0),  ("kawiw", 0),  ("pmova", 1),  ("slip", 1),   ("bgdph", 4),  ("bgscr", 6),  ("wcls!", 1),  ("wsizw", 9),

    # 0x30..0x37
    ("key!", 3),   ("keyon", 3),  ("keyof", 3),  ("uc", 1),     ("pdira", 1),  ("ptura", 3),  ("wspcl", 4),  ("wnumb", 7),

    # 0x38..0x3f
    ("sttim", 5),  ("gold+", 5),  ("gold-", 5),  ("chgld", 3),  ("hmpmx", 0),  ("hmpmx", 0),  ("mhmmx", 0),  ("hmpmx", 0),

    # 0x40..0x47
    ("mes", 2),    ("mpara", 4),  ("mpra2", 5),  ("mpnam", 1),  ("", -1),      ("mp+", 4),    ("", -1),      ("mp-", 4),

    # 0x48..0x4f
    ("ask", 6),    ("menu", 3),   ("menu", 1),   ("btltb", 1),  ("", -1),      ("hp+", 4),    ("", -1),      ("hp-", 4),

    # 0x50..0x57
    ("wsize", 9),  ("wmove", 5),  ("wmode", 3),  ("wrest", 1),  ("wclse", 1),  ("wrow", 2),   ("gwcol", 6),  ("swcol", 6),

    # 0x58..0x5f
    ("stitm", 4),  ("dlitm", 4),  ("ckitm", 4),  ("smtra", 6),  ("dmtra", 7),  ("cmtra", 9),  ("shake", 7),  ("wait", 0),

    # 0x60..0x67
    ("mjump", 9),  ("scrlo", 1),  ("scrlc", 4),  ("scrla", 5),  ("scr2d", 5),  ("scrcc", 0),  ("scr2dc", 8), ("scrlw", 0),

    # 0x68..0x6f
    ("scr2dl", 8), ("mpdsp", 1),  ("vwoft", 6),  ("fade", 8),   ("fadew", 0),  ("idlck", 3),  ("lstmp", 2),  ("scrlp", 5),

    # 0x70..0x77
    ("batle", 3),  ("btlon", 1),  ("btlmd", 2),  ("pgtdr", 3),  ("getpc", 3),  ("pxyzi", 7),  ("plus!", 3),  ("pls2!", 4),

    # 0x78..0x7f
    ("mins!", 3),  ("mns2!", 4),  ("inc!", 2),   ("inc2!", 2),  ("dec!", 2),   ("dec2!", 2),  ("tlkon", 1),  ("rdmsd", 2),

    # 0x80..0x87
    ("set", 3),    ("set2", 4),   ("biton", 3),  ("bitof", 3),  ("bitxr", 3),  ("plus", 3),   ("plus2", 4),  ("minus", 3),

    # 0x88..0x8f
    ("mins2", 4),  ("mul", 3),    ("mul2", 4),   ("div", 3),    ("div2", 4),   ("remai", 3),  ("rema2", 4),  ("and", 3),

    # 0x90..0x97
    ("and2", 4),   ("or", 3),     ("or2", 4),    ("xor", 3),    ("xor2", 4),   ("inc", 2),    ("inc2", 2),   ("dec", 2),

    # 0x98..0x9f
    ("dec2", 2),   ("randm", 2),  ("lbyte", 3),  ("hbyte", 4),  ("2byte", 5),  ("setx", 6),   ("getx", 6),   ("srchx", 10),

    # 0xa0..0xa7
    ("pc", 1),     ("char", 1),   ("dfanm", 2),  ("anime", 2),  ("visi", 1),   ("xyzi", 10),  ("xyi", 8),    ("xyz", 8),

    # 0xa8..0xaf
    ("move", 5),   ("cmove", 5),  ("mova", 1),   ("tura", 3),   ("animw", 0),  ("fmove", 5),  ("anime", 2),  ("anim!", 2),

    # 0xb0..0xb7
    ("canim", 4),  ("canm!", 4),  ("msped", 3),  ("dir", 2),    ("turnr", 5),  ("turn", 5),   ("dira", 1),   ("gtdir", 3),

    # 0xb8..0xbf
    ("getaxy", 4), ("getai", 3),  ("anim!", 2),  ("canim", 4),  ("canm!", 4),  ("asped", 3),  ("", -1),      ("cc", 1),

    # 0xc0..0xc7
    ("jump", 10),  ("axyzi", 7),  ("lader", 14), ("ofstd", 11), ("ofstw", 0),  ("talkR", 2),  ("slidR", 2),  ("solid", 1),

    # 0xc8..0xcf
    ("prtyp", 1),  ("prtym", 1),  ("prtye", 3),  ("prtyq", 2),  ("membq", 2),  ("mmb+-", 2),  ("mmblk", 1),  ("mmbuk", 1),

    # 0xd0..0xd7
    ("line", 12),  ("linon", 1),  ("mpjpo", 1),  ("sline", 15), ("sin", 9),    ("cos", 9),    ("tlkR2", 3),  ("sldR2", 3),

    # 0xd8..0xdf
    ("pmjmp", 2),  ("pmjmp", 0),  ("akao2", 14), ("fcfix", 1),  ("ccanm", 3),  ("animb", 0),  ("turnw", 0),  ("mppal", 10),

    # 0xe0..0xe7
    ("bgon", 3),   ("bgoff", 3),  ("bgrol", 2),  ("bgrol", 2),  ("bgclr", 2),  ("stpal", 4),  ("ldpal", 4),  ("cppal", 4),

    # 0xe8..0xef
    ("rtpal", 6),  ("adpal", 9),  ("mppal", 9),  ("stpls", 4),  ("ldpls", 4),  ("cppal", 7),  ("rtpal", 7),  ("adpal", 10),

    # 0xf0..0xf7
    ("music", 1),  ("se", 4),     ("akao", 13),  ("musvt", 1),  ("musvm", 1),  ("mulck", 1),  ("bmusc", 1),  ("chmph", 3),

    # 0xf8..0xff
    ("pmvie", 1),  ("movie", 0),  ("mvief", 2),  ("mvcam", 1),  ("fmusc", 1),  ("cmusc", 5),  ("chmst", 2),  ("gmovr", 0),
]


# Mnemonic and operand length for SPCAL sub-opcodes
specialOpcodes = {
    0xf5: ("arrow", 1),
    0xf6: ("pname", 4),
    0xf7: ("gmspd", 2),
    0xf8: ("smspd", 2),
    0xf9: ("flmat", 0),
    0xfa: ("flitm", 0),
    0xfb: ("btlck", 1),
    0xfc: ("mvlck", 1),
    0xfd: ("spcnm", 2),
    0xfe: ("rsglb", 0),
    0xff: ("clitm", 0),
}


# Some selected opcodes (flow control and text/window-related)
Op = _enum(
    RET = 0x00, RETTO = 0x07, SPCAL = 0x0f, SKIP = 0x10,
    LSKIP = 0x11, BACK = 0x12, LBACK = 0x13, IF = 0x14,
    LIF = 0x15, IF2 = 0x16, LIF2 = 0x17, IF2U = 0x18,
    LIF2U = 0x19, KAWAI = 0x28, WSIZW = 0x2f, KEYQ = 0x30,
    KEYON = 0x31, KEYOFF = 0x32, WSPCL = 0x36, MES = 0x40,
    MPNAM = 0x43, ASK = 0x48, WSIZE = 0x50, WREST = 0x53,
    PRTYQ = 0xcb, MEMBQ = 0xcc, GMOVR = 0xff,
    SPCNM = 0x0ffd,
)


#
# Terminology:
# - An "address" is the offset of a script instruction relative to the start
#   of the event section of the field map.
# - An "offset" refers to a relative location within the script code block,
#   and is used to refer to script code bytes within the bytearray which
#   holds the script code.
# - The "base address" of the script code block is the address of the script
#   instruction with offset 0.
#
# For example, if the script code block starts at byte 0x1234 within the
# event section, then the first instruction of the script is at address
# 0x1234, offset 0.
#


# Basic block of the control flow graph
class BasicBlock:
    def __init__(self):

        # List of offsets of the instructions which make up the block
        self.instructions = []

        # Set of addresses of succeeding blocks (zero for exit blocks,
        # one for sequential control flow or unconditional jumps, two
        # or more for conditional branches)
        self.succ = set()


# Find the size of the instruction at the given offset in a script code block.
def instructionSize(code, offset):
    op = code[offset]
    size = opcodes[op][1] + 1

    if op == Op.SPCAL:

        # First operand byte is sub-opcode
        subOp = code[offset + 1]
        size = specialOpcodes[subOp][1] + 2

    elif op == Op.KAWAI:

        # Variable size given by first operand byte
        size = code[offset + 1]

    return size


# If the instruction at the given offset is a jump or branch instruction,
# return the jump target offset. Otherwise, return None.
def targetOffset(code, offset):
    op = code[offset]

    if op == Op.SKIP:
        return offset + code[offset + 1] + 1
    elif op == Op.LSKIP:
        return offset + (code[offset + 1] | (code[offset + 2] << 8)) + 1
    elif op == Op.BACK:
        return offset - code[offset + 1]
    elif op == Op.LBACK:
        return offset - (code[offset + 1] | (code[offset + 2] << 8))
    if op == Op.IF:
        return offset + code[offset + 5] + 5
    elif op == Op.LIF:
        return offset + (code[offset + 5] | (code[offset + 6] << 8)) + 5
    elif op in (Op.IF2, Op.IF2U):
        return offset + code[offset + 7] + 7
    elif op in (Op.LIF2, Op.LIF2U):
        return offset + (code[offset + 7] | (code[offset + 8] << 8)) + 7
    elif op in (Op.KEYQ, Op.KEYON, Op.KEYOFF):
        return offset + code[offset + 3] + 3
    elif op in (Op.PRTYQ, Op.MEMBQ):
        return offset + code[offset + 2] + 2
    else:
        return None


# Return true if the instruction at the given offset halts the control flow.
def isExit(code, offset):
    return code[offset] in (Op.RET, Op.RETTO, Op.GMOVR)

# Return true if the instruction at the given offset is an unconditional jump.
def isJump(code, offset):
    return code[offset] in (Op.SKIP, Op.LSKIP, Op.BACK, Op.LBACK)

# Return true if the instruction at the given offset is a conditional branch.
def isBranch(code, offset):
    return code[offset] in (Op.IF, Op.LIF, Op.IF2, Op.LIF2, Op.IF2U, Op.LIF2U,
                            Op.KEYQ, Op.KEYON, Op.KEYOFF, Op.PRTYQ, Op.MEMBQ)


# Build and return the control flow graph, a dictionary mapping addresses to
# BasicBlock objects.
def buildCFG(code, baseAddress, entryAddresses):

    # Find the addresses of the leaders, starting with the supplied set of
    # entry addresses
    leaders = set(entryAddresses)

    offset = 0
    while offset < len(code):
        nextOffset = offset + instructionSize(code, offset)

        # Instructions following exit points are leaders
        if isExit(code, offset):
            leaders.add(nextOffset + baseAddress)
        else:
            target = targetOffset(code, offset)

            # Targets of jump and branches, and the instructions following
            # a jump or branch, are leaders
            if target is not None:
                leaders.add(target + baseAddress)
                leaders.add(nextOffset + baseAddress)

        offset = nextOffset

    # For each leader, assemble the corresponding basic block, building
    # the graph
    graph = {}

    for leader in leaders:
        addr = leader
        offset = addr - baseAddress

        # If the last instruction of the code is a jump, there will be
        # a bogus leader pointing after the end of the code, which we
        # need to skip
        if offset >= len(code):
            continue

        block = BasicBlock()

        while True:

            # Append one instruction
            size = instructionSize(code, offset)
            block.instructions.append(offset)

            addr += size
            offset += size

            # Stop when reaching another leader, or before going outside the
            # code section
            if (addr in leaders) or (offset >= len(code)):
                break

        # Examine the last instruction of the block to determine the
        # block's successors
        assert len(block.instructions) > 0
        lastInstruction = block.instructions[-1]

        if isJump(code, lastInstruction):      # one successor: the jump target
            block.succ = set([targetOffset(code, lastInstruction) + baseAddress])
        elif isBranch(code, lastInstruction):  # two successors: the branch target and the next instruction
            if offset >= len(code):
                raise IndexError("Control flow reaches end of script code")
            block.succ = set([targetOffset(code, lastInstruction) + baseAddress, addr])
        elif isExit(code, lastInstruction):    # no successors
            block.succ = set()
        else:                                  # one successor: the next instruction
            if offset >= len(code):
                raise IndexError("Control flow reaches end of script code")
            block.succ = set([addr])

        # Add the block to the graph
        graph[leader] = block

    return graph


# Determine all possible paths through a control flow graph starting at a given
# entry point, ignoring any cycles.
#
# This function returns a list of paths, each path being a list of instruction
# addresses.
def findPaths(graph, entryAddress, path = []):
    path = path + [entryAddress]

    succ = graph[entryAddress].succ
    if not succ:
        return [path]  # exit reached

    paths = []
    for addr in succ:
        if addr not in path:
            paths += findPaths(graph, addr, path)

    if paths:
        return paths
    else:
        return [path]  # cycle reached


# Remove instructions from the blocks of a code flow graph, only keeping those
# in the specified list. The passed-in graph is modified by this function.
# SPCAL 2-byte opcodes which should be kept can be specified as 0x0fxx.
def filterInstructions(graph, code, keep):
    for block in list(graph.values()):
        newInstructions = []

        for offset in block.instructions:
            op = code[offset]
            if op == Op.SPCAL:
                op = (op << 8) | code[offset + 1]

            if op in keep:
                newInstructions.append(offset)

        block.instructions = newInstructions


# Recursively find all possible exits from a given block which lie
# outside of a specified address range.
def possibleExitsFrom(graph, block, minAddr, maxAddr, consideredBlocks = set()):
    exits = set()

    if block in consideredBlocks:
        return exits
    else:
        consideredBlocks.add(block)

    for succ in block.succ:
        if succ >= minAddr and succ < maxAddr:
            exits |= possibleExitsFrom(graph, graph[succ], minAddr, maxAddr, consideredBlocks)
        else:
            exits.add(succ)

    return exits


# Reduce a (filtered) graph in order to lower the number of paths to examine
# for cases where we're only interested in the possible sequence of
# instructions. The passed-in graph is modified by this function.
def reduce(graph, entryAddresses):

    while True:
        nothingChanged = True

        # Eliminate the condition from simple 'if c then b' constructs by
        # assuming that the inner block is always executed
        for blockAddr, block in graph.items():
            if len(block.succ) == 2:
                sortedSuccs = sorted(list(block.succ))
                innerAddr = sortedSuccs[0]
                exitAddr = sortedSuccs[1]

                innerBlock = graph[innerAddr]
                if possibleExitsFrom(graph, innerBlock, innerAddr, exitAddr) == set([exitAddr]):
#                    print "eliminating %s -> %04x" % (map(hex, list(block.succ)), innerAddr)
                    block.succ = set([innerAddr])
                    nothingChanged = False

        if nothingChanged:
            break
                
    while True:
        nothingChanged = True

        # Skip blocks with no (filtered) instructions as long as it reduces
        # the number of paths
        for blockAddr, block in graph.items():
            newSucc = set()

            for addr in block.succ:
                succBlock = graph[addr]
                if not succBlock.instructions:
                    newSucc |= succBlock.succ
                else:
                    newSucc |= set([addr])

            newSucc.discard(blockAddr)  # remove simple cycles

            if newSucc != block.succ and len(newSucc) < 3:  # avoid excessive branching
#                print "reducing %s -> %s" % (map(hex, list(block.succ)), map(hex, list(newSucc)))
                block.succ = newSucc
                nothingChanged = False

        if nothingChanged:
            break

    while True:
        nothingChanged = True

        # Remove orphaned blocks
        referencedBlocks = set(entryAddresses)
        for block in list(graph.values()):
            referencedBlocks |= block.succ

        for addr in list(graph.keys())[:]:
            if addr not in referencedBlocks:
#                print "deleting %04x" % addr
                del graph[addr]
                nothingChanged = False

        if nothingChanged:
            break


# Dissasemble script code, optionally printing labels before instructions.
# The 'baseAddress' specifies the (virtual) start address of the first
# script instruction.
def disassemble(code, baseAddress = 0, labels = []):
    s = ""

    offset = 0
    while offset < len(code):
        addr = offset + baseAddress

        firstLabel = True
        for labelText, labelOffset in labels:
            if labelOffset == addr:
                if firstLabel:
                    s += '\n'
                    firstLabel = False

                s += "%s:" % labelText
                s += '\n'

        format = "%04x: "
        values = (addr, )

        op = code[offset]
        offset += 1

        mnemonic, size = opcodes[op]

        if op == Op.SPCAL:  # first operand byte is sub-opcode
            subOp = code[offset]
            offset += 1
            mnemonic, size = specialOpcodes[subOp]
        elif op == Op.KAWAI:  # variable size given by first operand byte
            size = code[offset] - 1

        if size < 0:  # illegal opcode
            mnemonic = "<%02x>" % op
            size = 0

        format += "%s"
        values += (mnemonic, )
        for i in range(offset, offset + size):
            format += " %02x"
            values += (code[i], )

        s += format % values
        s += '\n'

        offset += size

    return s

fieldIDTable = {'ancnt1': '0x0286',
 'ancnt2': '0x0287',
 'ancnt3': '0x0288',
 'ancnt4': '0x0289',
 'anfrst_1': '0x026C',
 'anfrst_2': '0x026D',
 'anfrst_3': '0x026E',
 'anfrst_4': '0x026F',
 'anfrst_5': '0x0270',
 'astage_a': '0x01E4',
 'astage_b': '0x01E5',
 'bigwheel': '0x01E8',
 'blackbg1': '0x005D',
 'blackbg2': '0x005E',
 'blackbg3': '0x005F',
 'blackbg4': '0x0060',
 'blackbg5': '0x0061',
 'blackbg6': '0x0062',
 'blackbg7': '0x0063',
 'blackbg8': '0x0064',
 'blackbg9': '0x0065',
 'blackbga': '0x0066',
 'blackbgb': '0x0067',
 'blackbgc': '0x0068',
 'blackbgd': '0x0069',
 'blackbge': '0x006A',
 'blackbgf': '0x006B',
 'blackbgg': '0x006C',
 'blackbgh': '0x006D',
 'blackbgi': '0x006E',
 'blackbgj': '0x006F',
 'blackbgk': '0x0070',
 'blin1': '0x00EA',
 'blin2': '0x00EB',
 'blin2_i': '0x00EC',
 'blin3_1': '0x00ED',
 'blin59': '0x00EE',
 'blin60_1': '0x00EF',
 'blin60_2': '0x00F0',
 'blin61': '0x00F1',
 'blin62_1': '0x00F2',
 'blin62_2': '0x00F3',
 'blin62_3': '0x00F4',
 'blin63_1': '0x00F5',
 'blin63_t': '0x00F6',
 'blin64': '0x00F7',
 'blin65_1': '0x00F8',
 'blin65_2': '0x00F9',
 'blin66_1': '0x00FA',
 'blin66_2': '0x00FB',
 'blin66_3': '0x00FC',
 'blin66_4': '0x00FD',
 'blin66_5': '0x00FE',
 'blin66_6': '0x00FF',
 'blin671b': '0x0101',
 'blin673b': '0x0104',
 'blin67_1': '0x0100',
 'blin67_2': '0x0102',
 'blin67_3': '0x0103',
 'blin67_4': '0x0105',
 'blin68_1': '0x0106',
 'blin68_2': '0x0107',
 'blin69_1': '0x0108',
 'blin69_2': '0x0109',
 'blin70_1': '0x010A',
 'blin70_2': '0x010B',
 'blin70_3': '0x010C',
 'blin70_4': '0x010D',
 'blinele': '0x00E8',
 'blinst_1': '0x00E5',
 'blinst_2': '0x00E6',
 'blinst_3': '0x00E7',
 'blue_1': '0x0280',
 'blue_2': '0x0281',
 'bonevil': '0x0269',
 'bonevil2': '0x0304',
 'bugin1a': '0x021D',
 'bugin1b': '0x021E',
 'bugin1c': '0x021F',
 'bugin2': '0x0220',
 'bugin3': '0x0221',
 'bwhlin': '0x01E9',
 'bwhlin2': '0x01EA',
 'canon_1': '0x02E4',
 'canon_2': '0x02E5',
 'cargoin': '0x008A',
 'chorace': '0x01FD',
 'chorace2': '0x01FE',
 'chrin_1a': '0x00B6',
 'chrin_1b': '0x00B7',
 'chrin_2': '0x00B8',
 'chrin_3a': '0x00B9',
 'chrin_3b': '0x00BA',
 'church': '0x00B5',
 'clsin2_1': '0x01F6',
 'clsin2_2': '0x01F7',
 'clsin2_3': '0x01F8',
 'colne_1': '0x00CE',
 'colne_2': '0x00CF',
 'colne_3': '0x00D0',
 'colne_4': '0x00D1',
 'colne_5': '0x00D2',
 'colne_6': '0x00D3',
 'colne_b1': '0x00D4',
 'colne_b3': '0x00D5',
 'coloin1': '0x01F4',
 'coloin2': '0x01F5',
 'coloss': '0x01F3',
 'condor1': '0x0161',
 'condor2': '0x0162',
 'convil_1': '0x0163',
 'convil_2': '0x0164',
 'convil_3': '0x0165',
 'convil_4': '0x0166',
 'corel1': '0x01D4',
 'corel2': '0x01D5',
 'corel3': '0x01D6',
 'corelin': '0x01E3',
 'cos_btm': '0x020D',
 'cos_btm2': '0x020E',
 'cos_top': '0x021C',
 'cosin1': '0x0211',
 'cosin1_1': '0x0212',
 'cosin2': '0x0213',
 'cosin3': '0x0214',
 'cosin4': '0x0215',
 'cosin5': '0x0216',
 'cosmin2': '0x0217',
 'cosmin3': '0x0218',
 'cosmin4': '0x0219',
 'cosmin6': '0x021A',
 'cosmin7': '0x021B',
 'cosmo': '0x020F',
 'cosmo2': '0x0210',
 'crater_1': '0x02BC',
 'crater_2': '0x02BD',
 'crcin_1': '0x01FF',
 'crcin_2': '0x0200',
 'datiao_1': '0x0250',
 'datiao_2': '0x0251',
 'datiao_3': '0x0252',
 'datiao_4': '0x0253',
 'datiao_5': '0x0254',
 'datiao_6': '0x0255',
 'datiao_7': '0x0256',
 'datiao_8': '0x0257',
 'del1': '0x01B9',
 'del12': '0x01BA',
 'del2': '0x01BB',
 'del3': '0x01C1',
 'delinn': '0x01BC',
 'delmin1': '0x01BE',
 'delmin12': '0x01BF',
 'delmin2': '0x01C0',
 'delpb': '0x01BD',
 'desert1': '0x01E1',
 'desert2': '0x01E2',
 'dummy': '0x000',
 'dyne': '0x01E0',
 'ealin_1': '0x00BC',
 'ealin_12': '0x00BD',
 'ealin_2': '0x00BE',
 'eals_1': '0x00BB',
 'eleout': '0x00E9',
 'elevtr1': '0x0079',
 'elm': '0x014F',
 'elm_i': '0x0149',
 'elm_wa': '0x0148',
 'elmin1_1': '0x014D',
 'elmin1_2': '0x014E',
 'elmin2_1': '0x0150',
 'elmin2_2': '0x0151',
 'elmin3_1': '0x0152',
 'elmin3_2': '0x0153',
 'elmin4_1': '0x0155',
 'elmin4_2': '0x0156',
 'elminn_1': '0x014B',
 'elminn_2': '0x014C',
 'elmpb': '0x014A',
 'elmtow': '0x0154',
 'fallp': '0x0301',
 'farm': '0x0157',
 'fr_e': '0x015B',
 'frcyo': '0x0159',
 'frmin': '0x0158',
 'fship_1': '0x0042',
 'fship_12': '0x0043',
 'fship_2': '0x0044',
 'fship_22': '0x0045',
 'fship_23': '0x0046',
 'fship_24': '0x0047',
 'fship_25': '0x0048',
 'fship_26': '0x0308',
 'fship_3': '0x0049',
 'fship_4': '0x004A',
 'fship_42': '0x004B',
 'fship_5': '0x004C',
 'gaia_1': '0x02B1',
 'gaia_2': '0x02B4',
 'gaia_31': '0x02B6',
 'gaia_32': '0x02B7',
 'gaiafoot': '0x02AE',
 'gaiin_1': '0x02B2',
 'gaiin_2': '0x02B3',
 'gaiin_3': '0x02B5',
 'gaiin_4': '0x02B8',
 'gaiin_5': '0x02B9',
 'gaiin_6': '0x02BA',
 'gaiin_7': '0x02BB',
 'games': '0x01F9',
 'games_1': '0x01FA',
 'games_2': '0x01FB',
 'ghotel': '0x01EB',
 'ghotin_1': '0x01EC',
 'ghotin_2': '0x01EE',
 'ghotin_3': '0x01EF',
 'ghotin_4': '0x01ED',
 'gidun_1': '0x0222',
 'gidun_2': '0x0223',
 'gidun_3': '0x0225',
 'gidun_4': '0x0224',
 'gldelev': '0x0201',
 'gldgate': '0x01F1',
 'gldinfo': '0x01F2',
 'gldst': '0x01F0',
 'gninn': '0x020A',
 'gnmk': '0x0205',
 'gnmkf': '0x0204',
 'gomin': '0x020B',
 'gon_i': '0x0209',
 'gon_wa1': '0x0207',
 'gon_wa2': '0x0208',
 'gongaga': '0x0206',
 'gonjun1': '0x0202',
 'gonjun2': '0x0203',
 'goson': '0x020C',
 'hekiga': '0x0284',
 'hideway1': '0x0247',
 'hideway2': '0x0248',
 'hideway3': '0x0249',
 'hill': '0x004D',
 'hill2': '0x0303',
 'holu_1': '0x02AF',
 'holu_2': '0x02B0',
 'hyou1': '0x0292',
 'hyou10': '0x02A8',
 'hyou11': '0x02A9',
 'hyou12': '0x02AA',
 'hyou13_1': '0x02AB',
 'hyou13_2': '0x02AC',
 'hyou14': '0x02AD',
 'hyou2': '0x0293',
 'hyou3': '0x0294',
 'hyou4': '0x0297',
 'hyou5_1': '0x0298',
 'hyou5_2': '0x0299',
 'hyou5_3': '0x029A',
 'hyou5_4': '0x029B',
 'hyou6': '0x029C',
 'hyou7': '0x02A4',
 'hyou8_1': '0x02A5',
 'hyou8_2': '0x02A6',
 'hyou9': '0x02A7',
 'hyoumap': '0x029D',
 'icedun_1': '0x0295',
 'icedun_2': '0x0296',
 'ithill': '0x02CC',
 'ithos': '0x02D0',
 'itmin1': '0x02D1',
 'itmin2': '0x02D2',
 'itown12': '0x02C9',
 'itown1a': '0x02C8',
 'itown1b': '0x02CA',
 'itown2': '0x02CB',
 'itown_i': '0x02CE',
 'itown_m': '0x02CF',
 'itown_w': '0x02CD',
 'jail1': '0x01D7',
 'jail2': '0x01D9',
 'jail3': '0x01DE',
 'jail4': '0x01DF',
 'jailin1': '0x01D8',
 'jailin2': '0x01DB',
 'jailin3': '0x01DC',
 'jailin4': '0x01DD',
 'jailpb': '0x01DA',
 'jet': '0x01E6',
 'jetin1': '0x01E7',
 'jtempl': '0x0258',
 'jtemplb': '0x0259',
 'jtemplc': '0x0307',
 'jtmpin1': '0x025A',
 'jtmpin2': '0x025B',
 'jumin': '0x01B1',
 'jun_a': '0x0176',
 'jun_i1': '0x016D',
 'jun_i2': '0x0177',
 'jun_m': '0x016E',
 'jun_w': '0x0175',
 'jun_wa': '0x016C',
 'junair': '0x0180',
 'junair2': '0x0181',
 'junbin1': '0x018C',
 'junbin12': '0x018D',
 'junbin21': '0x018E',
 'junbin22': '0x018F',
 'junbin3': '0x0190',
 'junbin4': '0x0191',
 'junbin5': '0x0192',
 'jundoc1a': '0x017E',
 'jundoc1b': '0x017F',
 'junele1': '0x0184',
 'junele2': '0x0187',
 'junin1': '0x0182',
 'junin1a': '0x0183',
 'junin2': '0x0185',
 'junin3': '0x0186',
 'junin4': '0x0188',
 'junin5': '0x0189',
 'junin6': '0x018A',
 'junin7': '0x018B',
 'juninn': '0x0178',
 'junmin1': '0x016F',
 'junmin2': '0x0170',
 'junmin3': '0x0171',
 'junmin4': '0x017C',
 'junmin5': '0x017D',
 'junmon': '0x0193',
 'junon': '0x0167',
 'junone2': '0x019B',
 'junone22': '0x0305',
 'junone3': '0x019C',
 'junone4': '0x019D',
 'junone5': '0x019E',
 'junone6': '0x019F',
 'junone7': '0x01A0',
 'junonl1': '0x0172',
 'junonl2': '0x0173',
 'junonl3': '0x0174',
 'junonr1': '0x0168',
 'junonr2': '0x0169',
 'junonr3': '0x016A',
 'junonr4': '0x016B',
 'junpb_1': '0x0179',
 'junpb_2': '0x017A',
 'junpb_3': '0x017B',
 'junsbd1': '0x0194',
 'kuro_1': '0x025C',
 'kuro_10': '0x0266',
 'kuro_11': '0x0267',
 'kuro_12': '0x0268',
 'kuro_2': '0x025D',
 'kuro_3': '0x025E',
 'kuro_4': '0x025F',
 'kuro_5': '0x0260',
 'kuro_6': '0x0261',
 'kuro_7': '0x0262',
 'kuro_8': '0x0263',
 'kuro_82': '0x0264',
 'kuro_9': '0x0265',
 'las0_1': '0x02E8',
 'las0_2': '0x02E9',
 'las0_3': '0x02EA',
 'las0_4': '0x02EB',
 'las0_5': '0x02EC',
 'las0_6': '0x02ED',
 'las0_7': '0x02EE',
 'las0_8': '0x02EF',
 'las1_1': '0x02F0',
 'las1_2': '0x02F1',
 'las1_3': '0x02F2',
 'las1_4': '0x02F3',
 'las2_1': '0x02F4',
 'las2_2': '0x02F5',
 'las2_3': '0x02F6',
 'las2_4': '0x02F7',
 'las3_1': '0x02F8',
 'las3_2': '0x02F9',
 'las3_3': '0x02FA',
 'las4_0': '0x02FB',
 'las4_1': '0x02FC',
 'las4_2': '0x02FD',
 'las4_3': '0x02FE',
 'las4_4': '0x02FF',
 'las4_42': '0x0309',
 'lastmap': '0x0300',
 'life': '0x02D3',
 'life2': '0x02D4',
 'losin1': '0x0277',
 'losin2': '0x0278',
 'losin3': '0x0279',
 'losinn': '0x027C',
 'loslake1': '0x027D',
 'loslake2': '0x027E',
 'loslake3': '0x027F',
 'lost1': '0x0276',
 'lost2': '0x027A',
 'lost3': '0x027B',
 'm_endo': '0x0302',
 'md0': '0x00E1',
 'md1_1': '0x0075',
 'md1_2': '0x0076',
 'md1stin': '0x0074',
 'md8_1': '0x0085',
 'md8_2': '0x0086',
 'md8_3': '0x0087',
 'md8_32': '0x02E3',
 'md8_4': '0x0088',
 'md8_5': '0x02DB',
 'md8_52': '0x030B',
 'md8_6': '0x02DC',
 'md8_b1': '0x02DD',
 'md8_b2': '0x02DE',
 'md8brdg': '0x0089',
 'md8brdg2': '0x02E2',
 'md_e1': '0x02E6',
 'mds5_1': '0x00B1',
 'mds5_2': '0x00AD',
 'mds5_3': '0x00AC',
 'mds5_4': '0x00AB',
 'mds5_5': '0x00AA',
 'mds5_dk': '0x00B0',
 'mds5_i': '0x00B3',
 'mds5_m': '0x00B4',
 'mds5_w': '0x00B2',
 'mds6_1': '0x00BF',
 'mds6_2': '0x00C0',
 'mds6_22': '0x00C1',
 'mds6_3': '0x00C2',
 'mds7': '0x0097',
 'mds7_im': '0x0098',
 'mds7_w1': '0x0094',
 'mds7_w2': '0x0095',
 'mds7_w3': '0x0096',
 'mds7pb_1': '0x009A',
 'mds7pb_2': '0x009B',
 'mds7plr1': '0x009C',
 'mds7plr2': '0x009D',
 'mds7st1': '0x0090',
 'mds7st2': '0x0091',
 'mds7st3': '0x0092',
 'mds7st32': '0x0093',
 'mds7st33': '0x030D',
 'midgal': '0x030E',
 '5min1_1': '0x00AE',
 '5min1_2': '0x00AF',
 '7min1': '0x0099',
 'mkt_ia': '0x00C6',
 'mkt_m': '0x00C8',
 'mkt_mens': '0x00C5',
 'mkt_s1': '0x00C9',
 'mkt_s2': '0x00CA',
 'mkt_s3': '0x00CB',
 'mkt_w': '0x00C4',
 'mktinn': '0x00C7',
 'mktpb': '0x00CC',
 'mogu_1': '0x01FC',
 'move_d': '0x02A3',
 'move_f': '0x02A0',
 'move_i': '0x029F',
 'move_r': '0x02A1',
 'move_s': '0x029E',
 'move_u': '0x02A2',
 'mrkt1': '0x00CD',
 'mrkt2': '0x00C3',
 'mrkt3': '0x00D6',
 'mrkt4': '0x00DE',
 'mtcrl_0': '0x01CA',
 'mtcrl_1': '0x01CB',
 'mtcrl_2': '0x01CC',
 'mtcrl_3': '0x01CD',
 'mtcrl_4': '0x01CE',
 'mtcrl_5': '0x01CF',
 'mtcrl_6': '0x01D0',
 'mtcrl_7': '0x01D1',
 'mtcrl_8': '0x01D2',
 'mtcrl_9': '0x01D3',
 'mtnvl2': '0x0137',
 'mtnvl3': '0x0138',
 'mtnvl4': '0x0139',
 'mtnvl5': '0x013A',
 'mtnvl6': '0x013B',
 'mtnvl6b': '0x013C',
 'ncoin1': '0x01C5',
 'ncoin2': '0x01C6',
 'ncoin3': '0x01C7',
 'ncoinn': '0x01C8',
 'ncorel': '0x01C2',
 'ncorel2': '0x01C3',
 'ncorel3': '0x01C4',
 'niv_cl': '0x0114',
 'niv_ti1': '0x011E',
 'niv_ti2': '0x011F',
 'niv_ti3': '0x0120',
 'niv_ti4': '0x0121',
 'niv_w': '0x010E',
 'nivgate': '0x0117',
 'nivgate2': '0x0118',
 'nivgate3': '0x0119',
 'nivgate4': '0x0310',
 'nivinn_1': '0x0111',
 'nivinn_2': '0x0112',
 'nivinn_3': '0x0113',
 'nivl': '0x011A',
 'nivl_2': '0x011B',
 'nivl_3': '0x011C',
 'nivl_4': '0x011D',
 'nivl_b1': '0x0122',
 'nivl_b12': '0x0123',
 'nivl_b2': '0x0124',
 'nivl_b22': '0x0125',
 'nivl_e1': '0x0126',
 'nivl_e2': '0x0127',
 'nivl_e3': '0x0128',
 'nmkin_1': '0x0078',
 'nmkin_2': '0x007A',
 'nmkin_3': '0x007B',
 'nmkin_4': '0x007C',
 'nmkin_5': '0x007D',
 'nrthmk': '0x0077',
 'nvdun1': '0x013D',
 'nvdun2': '0x013E',
 'nvdun3': '0x013F',
 'nvdun31': '0x0140',
 'nvdun4': '0x0141',
 'nvmin1_1': '0x010F',
 'nvmin1_2': '0x0110',
 'nvmkin1': '0x0142',
 'nvmkin21': '0x0143',
 'nvmkin22': '0x0144',
 'nvmkin23': '0x0145',
 'nvmkin31': '0x0146',
 'nvmkin32': '0x0147',
 'onna_1': '0x00D7',
 'onna_2': '0x00D8',
 'onna_3': '0x00D9',
 'onna_4': '0x00DA',
 'onna_5': '0x00DB',
 'onna_52': '0x00DC',
 'onna_6': '0x00DD',
 'pass': '0x023B',
 'pillar_1': '0x009E',
 'pillar_2': '0x009F',
 'pillar_3': '0x00A0',
 'prisila': '0x01AF',
 'psdun_1': '0x015D',
 'psdun_2': '0x015E',
 'psdun_3': '0x015F',
 'psdun_4': '0x0160',
 'q_1': '0x0058',
 'q_2': '0x0059',
 'q_3': '0x005A',
 'q_4': '0x005B',
 'q_5': '0x005C',
 'rckt': '0x022D',
 'rckt2': '0x0227',
 'rckt3': '0x0228',
 'rckt32': '0x0306',
 'rcktbas1': '0x0231',
 'rcktbas2': '0x0232',
 'rcktin1': '0x0233',
 'rcktin2': '0x0234',
 'rcktin3': '0x0235',
 'rcktin4': '0x0236',
 'rcktin5': '0x0237',
 'rcktin6': '0x0238',
 'rcktin7': '0x0239',
 'rcktin8': '0x023A',
 'rkt_i': '0x022A',
 'rkt_w': '0x0229',
 'rktinn1': '0x022B',
 'rktinn2': '0x022C',
 'rktmin1': '0x022F',
 'rktmin2': '0x0230',
 'rktsid': '0x022E',
 'roadend': '0x00E2',
 'rootmap': '0x008F',
 'ropest': '0x01C9',
 'sandun_1': '0x0274',
 'sandun_2': '0x0275',
 'sango1': '0x0271',
 'sango2': '0x0272',
 'sango3': '0x0273',
 '4sbwy_1': '0x00A4',
 '4sbwy_2': '0x00A5',
 '4sbwy_22': '0x02DF',
 '4sbwy_3': '0x00A6',
 '4sbwy_4': '0x00A7',
 '4sbwy_5': '0x00A8',
 '4sbwy_6': '0x00A9',
 'sea': '0x0056',
 'semkin_1': '0x01A4',
 'semkin_2': '0x01A5',
 'semkin_3': '0x01A7',
 'semkin_4': '0x01A8',
 'semkin_5': '0x01A9',
 'semkin_6': '0x01AA',
 'semkin_7': '0x01AB',
 'semkin_8': '0x01A6',
 'seto1': '0x0226',
 'ship_1': '0x01B4',
 'ship_2': '0x01B5',
 'shpin_2': '0x01B7',
 'shpin_22': '0x01B6',
 'shpin_3': '0x01B8',
 'sichi': '0x015C',
 'sinbil_1': '0x00E3',
 'sinbil_2': '0x00E4',
 'sinin1_1': '0x0129',
 'sinin1_2': '0x012A',
 'sinin2_1': '0x012B',
 'sinin2_2': '0x012C',
 'sinin3': '0x012D',
 'sininb1': '0x012E',
 'sininb2': '0x012F',
 'sininb31': '0x0130',
 'sininb32': '0x0131',
 'sininb33': '0x0132',
 'sininb34': '0x030C',
 'sininb35': '0x030F',
 'sininb36': '0x0311',
 'sininb41': '0x0133',
 'sininb42': '0x0134',
 'sininb51': '0x0135',
 'sininb52': '0x0136',
 'sky': '0x0057',
 'slfrst_1': '0x026A',
 'slfrst_2': '0x026B',
 'smkin_1': '0x0080',
 'smkin_2': '0x0081',
 'smkin_3': '0x0082',
 'smkin_4': '0x0083',
 'smkin_5': '0x0084',
 'sninn_1': '0x028B',
 'sninn_2': '0x028C',
 'sninn_b1': '0x028D',
 'snmayor': '0x0291',
 'snmin1': '0x028F',
 'snmin2': '0x0290',
 'snow': '0x028E',
 'snw_w': '0x028A',
 'southmk1': '0x007E',
 'southmk2': '0x007F',
 'spgate': '0x01A1',
 'spipe_1': '0x01A2',
 'spipe_2': '0x01A3',
 'startmap': '0x0041',
 'subin_1a': '0x0195',
 'subin_1b': '0x0196',
 'subin_2a': '0x0197',
 'subin_2b': '0x0198',
 'subin_3': '0x0199',
 'subin_4': '0x019A',
 'tin_1': '0x008B',
 'tin_2': '0x008C',
 'tin_3': '0x008D',
 'tin_4': '0x008E',
 '5tower': '0x024A',
 'trackin': '0x0115',
 'trackin2': '0x0116',
 'trap': '0x015A',
 'trnad_1': '0x02BE',
 'trnad_2': '0x02BF',
 'trnad_3': '0x02C0',
 'trnad_4': '0x02C1',
 'trnad_51': '0x02C2',
 'trnad_52': '0x02C3',
 'trnad_53': '0x02C4',
 'tunnel_1': '0x00A1',
 'tunnel_2': '0x00A2',
 'tunnel_3': '0x00A3',
 'tunnel_4': '0x02E0',
 'tunnel_5': '0x02E1',
 'tunnel_6': '0x030A',
 'ujun_w': '0x01B0',
 'ujunon1': '0x01AC',
 'ujunon2': '0x01AD',
 'ujunon3': '0x01AE',
 'ujunon4': '0x01B2',
 'ujunon5': '0x01B3',
 'uta_im': '0x0240',
 'uta_wa': '0x023F',
 'utapb': '0x0244',
 'utmin1': '0x0241',
 'utmin2': '0x0242',
 'uttmpin1': '0x024C',
 'uttmpin2': '0x024D',
 'uttmpin3': '0x024E',
 'uttmpin4': '0x024F',
 'uutai1': '0x0243',
 'uutai2': '0x024B',
 'wcrimb_1': '0x00DF',
 'wcrimb_2': '0x00E0',
 'white1': '0x0282',
 'white2': '0x0283',
 'whitebg1': '0x0071',
 'whitebg2': '0x0072',
 'whitebg3': '0x0073',
 'whitein': '0x0285',
 'wm0': '0x001',
 'wm1': '0x002',
 'wm10': '0x00B',
 'wm11': '0x00C',
 'wm12': '0x00D',
 'wm13': '0x00E',
 'wm14': '0x00F',
 'wm15': '0x010',
 'wm16': '0x011',
 'wm17': '0x012',
 'wm18': '0x013',
 'wm19': '0x014',
 'wm2': '0x003',
 'wm20': '0x015',
 'wm21': '0x016',
 'wm22': '0x017',
 'wm23': '0x018',
 'wm24': '0x019',
 'wm25': '0x01A',
 'wm26': '0x01B',
 'wm27': '0x01C',
 'wm28': '0x01D',
 'wm29': '0x01E',
 'wm3': '0x004',
 'wm30': '0x01F',
 'wm31': '0x020',
 'wm32': '0x021',
 'wm33': '0x022',
 'wm34': '0x023',
 'wm35': '0x024',
 'wm36': '0x025',
 'wm37': '0x026',
 'wm38': '0x027',
 'wm39': '0x028',
 'wm4': '0x005',
 'wm40': '0x029',
 'wm41': '0x02A',
 'wm42': '0x02B',
 'wm43': '0x02C',
 'wm44': '0x02D',
 'wm45': '0x02E',
 'wm46': '0x02F',
 'wm47': '0x030',
 'wm48': '0x031',
 'wm49': '0x032',
 'wm5': '0x006',
 'wm50': '0x033',
 'wm51': '0x034',
 'wm52': '0x035',
 'wm53': '0x036',
 'wm54': '0x037',
 'wm55': '0x038',
 'wm56': '0x039',
 'wm57': '0x03A',
 'wm58': '0x03B',
 'wm59': '0x03C',
 'wm6': '0x007',
 'wm60': '0x03D',
 'wm61': '0x03E',
 'wm62': '0x03F',
 'wm63': '0x040',
 'wm7': '0x008',
 'wm8': '0x009',
 'wm9': '0x00A',
 'woa_1': '0x02C5',
 'woa_2': '0x02C6',
 'woa_3': '0x02C7',
 'xmvtes': '0x02E7',
 'yougan': '0x023C',
 'yougan2': '0x023D',
 'yougan3': '0x023E',
 'yufy1': '0x0245',
 'yufy2': '0x0246',
 'zcoal_1': '0x02D8',
 'zcoal_2': '0x02D9',
 'zcoal_3': '0x02DA',
 'zmind1': '0x02D5',
 'zmind2': '0x02D6',
 'zmind3': '0x02D7',
 'ztruck': '0x0312',
 'zz1': '0x004E',
 'zz2': '0x004F',
 'zz3': '0x0050',
 'zz4': '0x0051',
 'zz5': '0x0052',
 'zz6': '0x0053',
 'zz7': '0x0054',
 'zz8': '0x0055'}