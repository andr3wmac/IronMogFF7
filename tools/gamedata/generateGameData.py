import os
import sys
import ff7
from ff7.field import opcodes, specialOpcodes, Op, fieldIDTable

class GameDataGenerator:
    def __init__(self, filepath="GameDataGenerated.cpp"):
        self.filepath = filepath
        self.file = None
        self.item_names = []

    def open_file(self):
        self.file = open(self.filepath, "w", encoding="utf-8")

    def close_file(self):
        if self.file:
            self.file.close()

    def write_line(self, line, indent=0):
        indent_str = " " * indent
        self.file.write(f"{indent_str}{line}\n")

    def write_header(self):
        self.write_line("// Auto-generated game data file", 0)
        self.write_line("#include \"GameData.h\"", 0)
        self.write_line("", 0)
        self.write_line("void GameData::loadGameData()", 0)
        self.write_line("{", 0)

    def write_footer(self):
        self.write_line("}", 0)

def outputInventory(gen, discPath, version):
    # Retrieve the kernel data file
    kernelDataFile = ff7.retrieveFile(discPath, "INIT", "KERNEL.BIN")
    kernelBin = ff7.kernel.Archive(kernelDataFile)

    # Extract all string lists
    for index, numStrings, compressed, transDir, transFileName in ff7.data.kernelStringData:
        stringList = ff7.kernel.StringList(kernelBin.getFile(9, index).getData(), numStrings, ff7.isJapanese(version))
        lines = stringList.getStrings()

        # Item Names
        if (index == 10):
            gen.write_line("// Items", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue

                gen.write_line("ADD_ITEM(" + str(idx) + ", \"" + item_name + "\");", 4)  
                gen.item_names.append(item_name)
            gen.write_line("")

        # Weapon Names
        if (index == 11):
            gen.write_line("// Weapons", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                gen.write_line("ADD_WEAPON(" + str(idx) + ", \"" + item_name + "\");", 4)  
                gen.item_names.append(item_name)
            gen.write_line("")

        # Armor Names
        if (index == 12):
            gen.write_line("// Armor", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                gen.write_line("ADD_ARMOR(" + str(idx) + ", \"" + item_name + "\");", 4)  
                gen.item_names.append(item_name)
            gen.write_line("")

        # Accessory Names
        if (index == 13):
            gen.write_line("// Accessories", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                gen.write_line("ADD_ACCESSORY(" + str(idx) + ", \"" + item_name + "\");", 4)   
                gen.item_names.append(item_name)
            gen.write_line("")

        # Materia Names
        if (index == 14):
            gen.write_line("// Materia", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                gen.write_line("ADD_MATERIA(" + str(idx) + ", \"" + item_name + "\");", 4) 
                gen.item_names.append(item_name) 
            gen.write_line("")

def outputOther(gen, discPath, version):
    # Enemy Skills
    # The uint64_t numbers were extracted from memory at runtime.
    gen.write_line("ADD_ESKILL(\"Frog Song\", 3298534884608);", 4)
    gen.write_line("ADD_ESKILL(\"L4 Suicide\", 7696581396993);", 4)
    gen.write_line("ADD_ESKILL(\"Magic Hammer\", 3298534884098);", 4)
    gen.write_line("ADD_ESKILL(\"White Wind\", 5497558147587);", 4)
    gen.write_line("ADD_ESKILL(\"Big Guard\", 5497558153220);", 4)
    gen.write_line("ADD_ESKILL(\"Angel Whisper\", 1099511640581);", 4)
    gen.write_line("ADD_ESKILL(\"Dragon Force\", 1099511632646);", 4)
    gen.write_line("ADD_ESKILL(\"Death Force\", 1099511628551);", 4)
    gen.write_line("ADD_ESKILL(\"Flame Thrower\", 3298534885896);", 4)
    gen.write_line("ADD_ESKILL(\"Laser\", 3298534887433);", 4)
    gen.write_line("ADD_ESKILL(\"Matra Magic\", 7696581396490);", 4)
    gen.write_line("ADD_ESKILL(\"Bad Breath\", 7696581409291);", 4)
    gen.write_line("ADD_ESKILL(\"Beta\", 7696581403404);", 4)
    gen.write_line("ADD_ESKILL(\"Aqualung\", 7696581403149);", 4)
    gen.write_line("ADD_ESKILL(\"Trine\", 7696581399566);", 4)
    gen.write_line("ADD_ESKILL(\"Magic Breath\", 570646534834959);", 4)
    gen.write_line("ADD_ESKILL(\"????\", 3298534884112);", 4)
    gen.write_line("ADD_ESKILL(\"Goblin Punch\", 3298534883345);", 4)
    gen.write_line("ADD_ESKILL(\"Chocobuckle\", 3298534884114);", 4)
    gen.write_line("ADD_ESKILL(\"L5 Death\", 7696581400083);", 4)
    gen.write_line("ADD_ESKILL(\"Death Sentence\", 3298534885908);", 4)
    gen.write_line("ADD_ESKILL(\"Roulette\", 218802813928981);", 4)
    gen.write_line("ADD_ESKILL(\"Shadow Flare\", 566248488330262);", 4)
    gen.write_line("ADD_ESKILL(\"Pandora’s Box\", 570646534843927);", 4)
    gen.write_line("")

def outputFields(gen, discPath, version):

    # Handle all map files
    for map in ff7.data.fieldMaps(version):
        # Get the event data
        mapData = ff7.field.MapData(ff7.retrieveFile(discPath, "FIELD", map + ".DAT"))
        event = mapData.getEventSection()
        code = event.scriptCode
        baseAddress = event.scriptBaseAddress

        fieldStrings = {}
        fieldOffsets = {}
        id = 0
        for stroffset, strlen, string in event.getStringsWithOffsets(ff7.isJapanese(version)):
            fieldStrings[id] = string
            fieldOffsets[id] = (stroffset, strlen)
            id += 1

        fieldID = fieldIDTable[map.lower()]
        gen.write_line("ADD_FIELD(" + fieldID + ", \"" + map.lower() + "\");", 4)

        offset = 0
        while offset < len(code):
            addr = offset + baseAddress

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

            if (mnemonic == "stitm"):
                item_id = values[3] | (values[4] << 8)
                item_quantity = values[5]

                gen.write_line("ADD_FIELD_ITEM(" + fieldID + ", " + f"0x{addr:X}" + ", " + str(item_id) + ", " + str(item_quantity) + ");", 4)

            if (mnemonic == "smtra"):
                materia_id = values[4]
                gen.write_line("ADD_FIELD_MATERIA(" + fieldID + ", " + f"0x{addr:X}" + ", " + str(materia_id) + ");", 4)

            if (mnemonic == "mes"):
                string = fieldStrings[values[3]]

                # Ensures the string contains an item/materia/etc name wrapped in quotes
                match = next((word for word in gen.item_names if f'"{word}"' in string), None)
                if match:
                    #print(match + ": " + string)
                    stroffset, strlen = fieldOffsets[values[3]]
                    #print(str(stroffset) + " " + str(strlen) + ": " + string)
                    gen.write_line("ADD_FIELD_MESSAGE(" + fieldID + ", " + f"0x{addr:X}" + ", " + f"0x{stroffset:X}" + ", " + str(strlen) + ");", 4)
                
                #print(values)

            if (mnemonic == "menu" and len(values) == 5):
                # Shop
                if values[3] == 8:
                    gen.write_line("ADD_FIELD_SHOP(" + fieldID + ", " + f"0x{addr:X}" + ", " + str(values[4]) + ");", 4)

            offset += size

        # World Map Exits
        triggers = mapData.getTriggerSection()
        section_start = mapData.getTriggerSectionStart()
        #clean_name = triggers.name.decode('utf-8').rstrip('\x00')

        door_index = 0
        for door in triggers.doors:
            if (door.fieldID > 0 and door.fieldID < 65):
                final_offset = section_start + 56 + (door_index * 24)

                gen.write_line("ADD_FIELD_WORLD_EXIT(" + fieldID + ", " + f"0x{final_offset:X}" + ", " + str(door_index) + ", " + f"0x{door.fieldID:X}" + ");", 4)
            door_index += 1

    gen.write_line("")

def outputWorldMap(gen, discPath, version):

    # We can generate this data from world map data however the layout on disc does not end up matching
    # the pattern we find in memory at runtime. For this reason we use a hardcoded table that was 
    # extracted and parsed from memory at runtime.

    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5436, 0x01, 185570, 120050);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x546c, 0x02, 202382, 111542);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x54a2, 0x03, 244049, 132736);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x558c, 0x04, 219198, 148615);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x55c2, 0x05, 206458, 160336);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x55f8, 0x06, 202304, 170568);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x562e, 0x07, 170941, 145274);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5670, 0x08, 165895, 186365);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x56a6, 0x09, 198982, 133190);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x56dc, 0x0A, 124157, 178432);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5712, 0x0B, 218072, 203773);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5768, 0x0C, 271186, 143196);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x579e, 0x0D, 141220, 124482);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x57e0, 0x0E, 113555, 121177);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5816, 0x0F, 109018, 125721);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5866, 0x10, 121350, 153151);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5884, 0x11, 108910, 186642);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x58ba, 0x12, 88317, 169166);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x58f0, 0x13, 95332, 133489);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5926, 0x14, 87967, 116554);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x597c, 0x15, 100214, 143154);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x59b2, 0x16, 117215, 108068);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x59e8, 0x17, 39324, 87078);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5a90, 0x18, 54567, 127403);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5ac6, 0x19, 163016, 82661);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5afc, 0x1A, 161539, 68127);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5b32, 0x1B, 129706, 71665);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5b68, 0x1C, 144109, 72407);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5b9e, 0x1D, 267145, 9773);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5238, 0x20, 218473, 149471);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5286, 0x22, 49838, 159447);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5bd4, 0x2B, 94857, 133349);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5c0a, 0x2C, 93129, 126853);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5c40, 0x2E, 83294, 137231);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5c76, 0x2F, 129326, 71724);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5cbc, 0x30, 135443, 54938);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5cda, 0x39, 160085, 89266);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5d10, 0x3A, 163057, 80131);", 4)
    gen.write_line("")

discPath = sys.argv[1]

if not os.path.isdir(discPath):
    raise EnvironmentError("'%s' is not a directory" % discPath)

# Check that this is a FF7 disc
version, discNumber, execFileName = ff7.checkDisc(discPath)

gen = GameDataGenerator()
gen.open_file()
gen.write_header()

outputInventory(gen, discPath, version)
outputOther(gen, discPath, version)
outputFields(gen, discPath, version)
outputWorldMap(gen, discPath, version)

gen.write_footer()
gen.close_file()