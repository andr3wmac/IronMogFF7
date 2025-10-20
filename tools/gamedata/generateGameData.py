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
    gen.write_line("ADD_ESKILL(\"Magic Breath\", 7696581413647);", 4)
    gen.write_line("ADD_ESKILL(\"????\", 3298534884112);", 4)
    gen.write_line("ADD_ESKILL(\"Goblin Punch\", 3298534883345);", 4)
    gen.write_line("ADD_ESKILL(\"Chocobuckle\", 3298534884114);", 4)
    gen.write_line("ADD_ESKILL(\"L5 Death\", 7696581400083);", 4)
    gen.write_line("ADD_ESKILL(\"Death Sentence\", 3298534885908);", 4)
    gen.write_line("ADD_ESKILL(\"Roulette\", 218802813928981);", 4)
    gen.write_line("ADD_ESKILL(\"Shadow Flare\", 3298534908950);", 4)
    gen.write_line("ADD_ESKILL(\"Pandora’s Box\", 7696581422615);", 4)
    gen.write_line("")

def unpack_ushort(byteA, byteB):
    value = byteA | (byteB << 8)
    if value >= 0x8000: value -= 0x10000
    return value

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
        #print("FIELD: " + map.lower())

        last_x = 0
        last_y = 0
        last_z = 0

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

            if (mnemonic == "xyzi"):
                last_x = unpack_ushort(values[4], values[5])
                last_y = unpack_ushort(values[6], values[7])
                last_z = unpack_ushort(values[8], values[9])

            if (mnemonic == "stitm"):
                item_id = values[3] | (values[4] << 8)
                item_quantity = values[5]

                gen.write_line("ADD_FIELD_ITEM(" + fieldID + ", " + f"0x{addr:X}" + ", " + str(item_id) + ", " + str(item_quantity) + ", " + str(last_x) + ", " + str(last_y) + ", " + str(last_z) + ");", 4)

            if (mnemonic == "smtra"):
                materia_id = values[4]
                gen.write_line("ADD_FIELD_MATERIA(" + fieldID + ", " + f"0x{addr:X}" + ", " + str(materia_id) + ");", 4)

            if (mnemonic == "mes"):
                string = fieldStrings[values[3]]

                # We use the Turtle Paradise newsletter for in game hints
                if ("Turtle Paradise" in string or "Turtles Paradise" in string or "Turtle's Paradise" in string) and ("No." in string or "Number" in string):
                    stroffset, strlen = fieldOffsets[values[3]]
                    gen.write_line("ADD_FIELD_MESSAGE(" + fieldID + ", " + f"0x{addr:X}" + ", " + f"0x{stroffset:X}" + ", " + str(strlen) + ");", 4)

                # Ensures the string contains an item/materia/etc name wrapped in quotes
                match = next((word for word in gen.item_names if f'"{word}"' in string), None)
                if match:
                    stroffset, strlen = fieldOffsets[values[3]]
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

        # Field Models
        modelSection = mapData.getModelSection()
        globalModelIDs = []
        for model in modelSection.models:
            globalModelIDs.append(model.globalModelID)
        gen.write_line("ADD_FIELD_MODELS(" + fieldID + ", " + ", ".join(str(x) for x in globalModelIDs) + ");", 4)    

    gen.write_line("")

def outputWorldMap(gen, discPath, version):

    # The center positions were generated by calculating average position for all the triangles with this trigger.
    # This was done with a modified ff7-terraform. However, its unable to produce proper jump locations for the scripts,
    # and seems to produce wrong jumps for certain entrances. Therefore, the jump addresses were all determined
    # by analyzing memory at runtime.

    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5436, 0x01, \"Slum Outskirts\", 185586, 120034);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x546c, 0x02, \"Kalm\", 201654, 112270);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x54a2, 0x03, \"Chocobo Farm\", 239232, 137553);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x558c, 0x04, \"Mithril Mine\", 214151, 153662);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x55c2, 0x05, \"Mithril Mine\", 209488, 157306);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x55f8, 0x06, \"Base of Fort Condor\", 203336, 169536);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x562e, 0x07, \"Under Junon\", 169850, 146365);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5670, 0x08, \"Temple of the Ancients\", 169981, 182279);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x56a6, 0x09, \"Old Man's House\", 198726, 133446);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x56dc, 0x0A, \"Weapon Seller\", 129280, 173309);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5712, 0x0B, \"Mideel\", 220157, 201688);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5768, 0x0C, \"Materia Cave\", 274268, 140114);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x579e, 0x0D, \"Costa del Sol\", 140866, 124836);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x57e0, 0x0E, \"Mt. Corel\", 112985, 121747);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5816, 0x0F, \"North Corel\", 109337, 125402);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5866, 0x10, \"Corel Desert\", 120383, 154118);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5884, 0x11, \"Jungle\", 112914, 182638);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x58ba, 0x12, \"Cosmo Canyon\", 87246, 170237);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x58f0, 0x13, \"Nibelheim\", 92529, 136292);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5926, 0x14, \"Rocket Town\", 83786, 120735);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x597c, 0x15, \"???\", 102194, 141174);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x59b2, 0x16, \"Materia Cave\", 116260, 109023);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x59e8, 0x17, \"Wutai\", 37926, 88476);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5a90, 0x18, \"Materia Cave\", 53675, 128295);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5ac6, 0x19, \"Bone Village\", 156389, 89288);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5afc, 0x1A, \"Corel Valley Cave\", 158239, 71427);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5b32, 0x1B, \"Icicle Inn\", 129009, 72362);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5b68, 0x1C, \"Mystery House\", 146135, 70381);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5b9e, 0x1D, \"Materia Cave\", 263725, 13193);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5238, 0x20, \"Marshes\", 215007, 152937);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5286, 0x22, \"Wilderness\", 52951, 156334);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5bd4, 0x2B, \"Nibelheim\", 92389, 135817);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5c0a, 0x2C, \"Mt. Nibel\", 94085, 125897);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5c40, 0x2E, \"Mt. Nibel\", 88079, 132446);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5c76, 0x2F, \"Icicle Inn\", 129068, 71982);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5cbc, 0x30, \"Great Glacier\", 136858, 53523);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5cda, 0x39, \"Corel Valley\", 162994, 86357);", 4)
    gen.write_line("ADD_WORLDMAP_ENTRANCE(0x5d10, 0x3A, \"Forgotten Capital\", 162051, 81137);", 4)
    gen.write_line("")

def outputBattles(gen, discPath, version):
    sceneBin = ff7.scene.Archive(ff7.retrieveFile(discPath, "BATTLE", "SCENE.BIN"))

    # Process all scenes
    for i in range(sceneBin.numScenes()):
        scene = sceneBin.getScene(i)

        hasValidEnemies = False
        enemyLevels = ""
        enemies = scene.getEnemies(ff7.isJapanese(version))
        for enemy in enemies:
            if len(enemy.name) >= 2 and ord(enemy.name[0]) == 0x00EA and ord(enemy.name[1]) == 0x00FA:
                hasValidEnemies = False
                break

            if (enemy.name != "" and enemy.level != 255):
                enemyLevels += ", " + str(enemy.level)
                hasValidEnemies = True
            else:
                enemyLevels += ", 255"
        
        if not hasValidEnemies:
            continue

        gen.write_line("ADD_BATTLE_SCENE(" + str(i) + ", " + str(scene.enemyID0) + ", " + str(scene.enemyID1) + ", " + str(scene.enemyID2) + enemyLevels + ");", 4)

        formations = scene.getFormations()
        formationIndex = 0
        for formation in formations:
            formationID = (i * 4) + formationIndex

            noEscape = "false"
            if not formation.canEscape():
                noEscape = "true"

            enemyIDs = ", ".join(map(str, formation.enemyIDs))
            gen.write_line("ADD_BATTLE_FORMATION(" + str(formationID) + ", " + str(i) + ", " + noEscape + ", " + enemyIDs + ");", 4)
            formationIndex += 1

    gen.write_line("")

def outputModels(gen, discPath, version):
    modelNames = ["BALLET", "CID", "CLOUD", "EARITH", "KETCY", "RED", "TIFA", "VINCENT", "YUFI"]

    for modelName in modelNames:
        cloudBCX = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", modelName + ".BCX"))

        model = cloudBCX["model"]
        for i in range(0, len(model["parts"])):
            part = model["parts"][i]
            gen.write_line("ADD_MODEL_PART(\"" + modelName + "\", " + str(i) + ", " + str(len(part["quad_color_tex"])) + ", " + str(len(part["tri_color_tex"])) + ", " + str(len(part["quad_mono_tex"])) + ", " + str(len(part["tri_mono_tex"])) + ", " + str(len(part["tri_mono"])) + ", " + str(len(part["quad_mono"])) + ", " + str(len(part["tri_color"])) + ", " + str(len(part["quad_color"])) + ");", 4)

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
outputBattles(gen, discPath, version)
outputModels(gen, discPath, version)

gen.write_footer()
gen.close_file()