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
    kernelDataFile = ff7.game.retrieveFile(discPath, "INIT", "KERNEL.BIN")
    kernelBin = ff7.kernel.Archive(kernelDataFile)

    # Extract shop price lists
    shopData = ff7.menu.ShopMenuData(ff7.game.retrieveFile(discPath, "MENU", "SHOPMENU.MNU"))

    # Extract all string lists
    for index, numStrings, compressed, transDir, transFileName in ff7.data.kernelStringData:
        stringList = ff7.kernel.StringList(kernelBin.getFile(9, index).getData(), numStrings, ff7.game.isJapanese(version))
        lines = stringList.getStrings()

        # Item Names
        if (index == 10):
            gen.write_line("// Items", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue

                item_id = idx
                item_price = shopData.item_prices[item_id]

                gen.write_line("addItem(" + str(idx) + ", \"" + item_name + "\", " + str(item_price) + ");", 4)  
                gen.item_names.append(item_name)
            gen.write_line("")

        # Weapon Names
        if (index == 11):
            gen.write_line("// Weapons", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                item_id = idx + 128
                item_price = shopData.item_prices[item_id]

                gen.write_line("addWeapon(" + str(idx) + ", \"" + item_name + "\", " + str(item_price) + ");", 4)  
                gen.item_names.append(item_name)
            gen.write_line("")

        # Armor Names
        if (index == 12):
            gen.write_line("// Armor", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                item_id = idx + 256
                item_price = shopData.item_prices[item_id]

                gen.write_line("addArmor(" + str(idx) + ", \"" + item_name + "\", " + str(item_price) + ");", 4)  
                gen.item_names.append(item_name)
            gen.write_line("")

        # Accessory Names
        if (index == 13):
            gen.write_line("// Accessories", 4)
            for idx in range(0, len(lines)):
                item_name = lines[idx]
                if (item_name == ""):
                    continue
                
                item_id = idx + 288
                item_price = shopData.item_prices[item_id]

                gen.write_line("addAccessory(" + str(idx) + ", \"" + item_name + "\", " + str(item_price) + ");", 4)   
                gen.item_names.append(item_name)
            gen.write_line("")

        # Materia Names
        if (index == 14):
            gen.write_line("// Materia", 4)
            for idx in range(0, len(lines)):
                materia_name = lines[idx]
                if (materia_name == ""):
                    continue

                materia_id = idx
                materia_price = shopData.materia_prices[materia_id]
                
                gen.write_line("addMateria(" + str(idx) + ", \"" + materia_name + "\", " + str(materia_price) + ");", 4) 
                gen.item_names.append(materia_name) 
            gen.write_line("")

def outputOther(gen, discPath, version):
    # Enemy Skills
    # TODO: we can pull this data from kernel, instead its hardcoded from inspection with Scarlet.
    # Format: Name, Target Flags, MP Cost, Index.
    gen.write_line("addESkill(\"Frog Song\", 0b00000011, 5, 0);", 4)
    gen.write_line("addESkill(\"L4 Suicide\", 0b00000111, 10, 1);", 4)
    gen.write_line("addESkill(\"Magic Hammer\", 0b00000011, 3, 2);", 4)
    gen.write_line("addESkill(\"White Wind\", 0b00000101, 34, 3);", 4)
    gen.write_line("addESkill(\"Big Guard\", 0b00000101, 56, 4);", 4)
    gen.write_line("addESkill(\"Angel Whisper\", 0b00000001, 50, 5);", 4)
    gen.write_line("addESkill(\"Dragon Force\", 0b00000001, 19, 6);", 4)
    gen.write_line("addESkill(\"Death Force\", 0b00000001, 3, 7);", 4)
    gen.write_line("addESkill(\"Flame Thrower\", 0b00000011, 10, 8);", 4)
    gen.write_line("addESkill(\"Laser\", 0b00000011, 16, 9);", 4)
    gen.write_line("addESkill(\"Matra Magic\", 0b00000111, 8, 10);", 4)
    gen.write_line("addESkill(\"Bad Breath\", 0b00000111, 58, 11);", 4)
    gen.write_line("addESkill(\"Beta\", 0b00000111, 35, 12);", 4)
    gen.write_line("addESkill(\"Aqualung\", 0b00000111, 34, 13);", 4)
    gen.write_line("addESkill(\"Trine\", 0b00000111, 20, 14);", 4)
    gen.write_line("addESkill(\"Magic Breath\", 0b00000111, 75, 15);", 4)
    gen.write_line("addESkill(\"????\", 0b00000011, 3, 16);", 4)
    gen.write_line("addESkill(\"Goblin Punch\", 0b00000011, 0, 17);", 4)
    gen.write_line("addESkill(\"Chocobuckle\", 0b00000011, 3, 18);", 4)
    gen.write_line("addESkill(\"L5 Death\", 0b00000111, 22, 19);", 4)
    gen.write_line("addESkill(\"Death Sentence\", 0b00000011, 10, 20);", 4)
    gen.write_line("addESkill(\"Roulette\", 0b11000111, 6, 21);", 4)
    gen.write_line("addESkill(\"Shadow Flare\", 0b00000011, 100, 22);", 4)
    gen.write_line("addESkill(\"Pandora’s Box\", 0b00000111, 110, 23);", 4)
    gen.write_line("")

def unpack_ushort(byteA, byteB):
    value = byteA | (byteB << 8)
    if value >= 0x8000: value -= 0x10000
    return value

def outputFields(gen, discPath, version):

    # Handle all map files
    for map in ff7.data.fieldMaps(version):
        # Get the event data
        mapData = ff7.field.MapData(ff7.game.retrieveFile(discPath, "FIELD", map + ".DAT"))
        event = mapData.getEventSection()
        code = event.scriptCode
        baseAddress = event.scriptBaseAddress

        fieldStrings = {}
        fieldOffsets = {}
        id = 0
        for stroffset, strlen, string in event.getStringsWithOffsets(ff7.game.isJapanese(version)):
            fieldStrings[id] = string
            fieldOffsets[id] = (stroffset, strlen)
            id += 1

        fieldID = fieldIDTable[map.lower()]
        gen.write_line("addField(" + fieldID + ", \"" + map.lower() + "\");", 4)

        last_x = 0
        last_y = 0
        last_z = 0

        offset = 0
        while offset < len(code):
            addr = offset + baseAddress
            groupIndex, scriptIndex = event.findGroupAndScript(addr)

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
                gen.write_line("addFieldScriptItem(" + fieldID + ", " + str(groupIndex) + ", " + str(scriptIndex) + ", " + f"0x{addr:X}" + ", " + str(item_id) + ", " + str(item_quantity) + ");", 4)

            if (mnemonic == "smtra"):
                materia_id = values[4]
                gen.write_line("addFieldScriptMateria(" + fieldID + ", " + str(groupIndex) + ", " + str(scriptIndex) + ", " + f"0x{addr:X}" + ", " + str(materia_id) + ");", 4)

            if (mnemonic == "mes"):
                windowIndex = values[2]
                string = fieldStrings[values[3]]

                # We use the Turtle Paradise newsletter for in game hints
                if ("Turtle Paradise" in string or "Turtles Paradise" in string or "Turtle's Paradise" in string) and ("No." in string or "Number" in string):
                    stroffset, strlen = fieldOffsets[values[3]]
                    gen.write_line("addFieldScriptMessage(" + fieldID + ", " + str(groupIndex) + ", " + str(scriptIndex) + ", " + str(windowIndex) + ", " + f"0x{addr:X}" + ", " + f"0x{stroffset:X}" + ", " + str(strlen) + ");", 4)

                # Ensures the string contains an item/materia/etc name wrapped in quotes
                match = next((word for word in gen.item_names if f'"{word}"' in string), None)
                if match:
                    stroffset, strlen = fieldOffsets[values[3]]
                    gen.write_line("addFieldScriptMessage(" + fieldID + ", " + str(groupIndex) + ", " + str(scriptIndex) + ", " + str(windowIndex) + ", " +  f"0x{addr:X}" + ", " + f"0x{stroffset:X}" + ", " + str(strlen) + ");", 4)

            if (mnemonic == "menu" and len(values) == 5):
                # Shop
                if values[3] == 8:
                    gen.write_line("addFieldScriptShop(" + fieldID + ", "+ str(groupIndex) + ", " + str(scriptIndex) + ", " + f"0x{addr:X}" + ", " + str(values[4]) + ");", 4)

            if (mnemonic == "batle"):
                if values[2] == 0:
                    encounterID = values[3] | (values[4] << 8)
                    print(map.lower() + " Encounter: " + str(encounterID))
                    gen.write_line("addFieldScriptBattle(" + fieldID + ", "+ str(groupIndex) + ", " + str(scriptIndex) + ", " + f"0x{addr:X}" + ", " + str(encounterID) + ");", 4)
                else:
                    print("Field script battle uses memory fetch: " + str(values))

            offset += size

        # World Map Exits
        triggers = mapData.getTriggerSection()
        section_start = mapData.getTriggerSectionStart()
        #clean_name = triggers.name.decode('utf-8').rstrip('\x00')

        door_index = 0
        for door in triggers.doors:
            if (door.fieldID > 0 and door.fieldID < 65):
                final_offset = section_start + 56 + (door_index * 24)

                gen.write_line("addFieldWorldExit(" + fieldID + ", " + f"0x{final_offset:X}" + ", " + str(door_index) + ", " + f"0x{door.fieldID:X}" + ");", 4)
            door_index += 1

        # Field Models
        modelSection = mapData.getModelSection()
        globalModelIDs = []
        for model in modelSection.models:
            globalModelIDs.append(model.globalModelID)
        gen.write_line("addFieldModels(" + fieldID + ", {" + ",".join(str(x) for x in globalModelIDs) + "});", 4)    

        # Field Encounters
        encounterSection = mapData.getEncounterSection()
        encounterSectionStart = mapData.getEncounterSectionStart()

        table0Str = "{}"
        if any(encounterSection.table0.encounters):
            table0Str = "{" + ",".join(str(x) for x in encounterSection.table0.encounters) + "}"

        table1Str = "{}"
        if any(encounterSection.table1.encounters):
            table1Str = "{" + ",".join(str(x) for x in encounterSection.table1.encounters) + "}"
        
        if not table0Str == "{}" or not table1Str == "{}":
            gen.write_line("addFieldEncounters(" + fieldID + ", " + f"0x{encounterSectionStart:X}" + ", " + table0Str + ", " + table1Str + ");", 4)    

    gen.write_line("")

def outputWorldMap(gen, discPath, version):

    # The center positions were generated by calculating average position for all the triangles with this trigger.
    # This was done with a modified ff7-terraform. However, its unable to produce proper jump locations for the scripts,
    # and seems to produce wrong jumps for certain entrances. Therefore, the jump addresses were all determined
    # by analyzing memory at runtime.

    gen.write_line("addWorldMapEntrance(0x5436, 0x01, \"Slum Outskirts\", 185586, 120034);", 4)
    gen.write_line("addWorldMapEntrance(0x546c, 0x02, \"Kalm\", 201654, 112270);", 4)
    gen.write_line("addWorldMapEntrance(0x54a2, 0x03, \"Chocobo Farm\", 239232, 137553);", 4)
    gen.write_line("addWorldMapEntrance(0x558c, 0x04, \"Mithril Mine\", 214151, 153662);", 4)
    gen.write_line("addWorldMapEntrance(0x55c2, 0x05, \"Mithril Mine\", 209488, 157306);", 4)
    gen.write_line("addWorldMapEntrance(0x55f8, 0x06, \"Base of Fort Condor\", 203336, 169536);", 4)
    gen.write_line("addWorldMapEntrance(0x562e, 0x07, \"Under Junon\", 169850, 146365);", 4)
    gen.write_line("addWorldMapEntrance(0x5670, 0x08, \"Temple of the Ancients\", 169981, 182279);", 4)
    gen.write_line("addWorldMapEntrance(0x56a6, 0x09, \"Old Man's House\", 198726, 133446);", 4)
    gen.write_line("addWorldMapEntrance(0x56dc, 0x0A, \"Weapon Seller\", 129280, 173309);", 4)
    gen.write_line("addWorldMapEntrance(0x5712, 0x0B, \"Mideel\", 220157, 201688);", 4)
    gen.write_line("addWorldMapEntrance(0x5768, 0x0C, \"Materia Cave\", 274268, 140114);", 4)
    gen.write_line("addWorldMapEntrance(0x579e, 0x0D, \"Costa del Sol\", 140866, 124836);", 4)
    gen.write_line("addWorldMapEntrance(0x57e0, 0x0E, \"Mt. Corel\", 112985, 121747);", 4)
    gen.write_line("addWorldMapEntrance(0x5816, 0x0F, \"North Corel\", 109337, 125402);", 4)
    gen.write_line("addWorldMapEntrance(0x5866, 0x10, \"Corel Desert\", 120383, 154118);", 4)
    gen.write_line("addWorldMapEntrance(0x5884, 0x11, \"Jungle\", 112914, 182638);", 4)
    gen.write_line("addWorldMapEntrance(0x58ba, 0x12, \"Cosmo Canyon\", 87246, 170237);", 4)
    gen.write_line("addWorldMapEntrance(0x58f0, 0x13, \"Nibelheim\", 92529, 136292);", 4)
    gen.write_line("addWorldMapEntrance(0x5926, 0x14, \"Rocket Town\", 83786, 120735);", 4)
    gen.write_line("addWorldMapEntrance(0x597c, 0x15, \"???\", 102194, 141174);", 4)
    gen.write_line("addWorldMapEntrance(0x59b2, 0x16, \"Materia Cave\", 116260, 109023);", 4)
    gen.write_line("addWorldMapEntrance(0x59e8, 0x17, \"Wutai\", 37926, 88476);", 4)
    gen.write_line("addWorldMapEntrance(0x5a90, 0x18, \"Materia Cave\", 53675, 128295);", 4)
    gen.write_line("addWorldMapEntrance(0x5ac6, 0x19, \"Bone Village\", 156389, 89288);", 4)
    gen.write_line("addWorldMapEntrance(0x5afc, 0x1A, \"Corel Valley Cave\", 158239, 71427);", 4)
    gen.write_line("addWorldMapEntrance(0x5b32, 0x1B, \"Icicle Inn\", 129009, 72362);", 4)
    gen.write_line("addWorldMapEntrance(0x5b68, 0x1C, \"Mystery House\", 146135, 70381);", 4)
    gen.write_line("addWorldMapEntrance(0x5b9e, 0x1D, \"Materia Cave\", 263725, 13193);", 4)
    gen.write_line("addWorldMapEntrance(0x5238, 0x20, \"Marshes\", 215007, 152937);", 4)
    gen.write_line("addWorldMapEntrance(0x5286, 0x22, \"Wilderness\", 52951, 156334);", 4)
    gen.write_line("addWorldMapEntrance(0x5bd4, 0x2B, \"Nibelheim\", 92389, 135817);", 4)
    gen.write_line("addWorldMapEntrance(0x5c0a, 0x2C, \"Mt. Nibel\", 94085, 125897);", 4)
    gen.write_line("addWorldMapEntrance(0x5c40, 0x2E, \"Mt. Nibel\", 88079, 132446);", 4)
    gen.write_line("addWorldMapEntrance(0x5c76, 0x2F, \"Icicle Inn\", 129068, 71982);", 4)
    gen.write_line("addWorldMapEntrance(0x5cbc, 0x30, \"Great Glacier\", 136858, 53523);", 4)
    gen.write_line("addWorldMapEntrance(0x5cda, 0x39, \"Corel Valley\", 162994, 86357);", 4)
    gen.write_line("addWorldMapEntrance(0x5d10, 0x3A, \"Forgotten Capital\", 162051, 81137);", 4)
    gen.write_line("")

def outputBattles(gen, discPath, version):
    sceneBin = ff7.scene.Archive(ff7.game.retrieveFile(discPath, "BATTLE", "SCENE.BIN"))

    # Process all scenes
    for i in range(sceneBin.numScenes()):
        scene = sceneBin.getScene(i)

        hasValidEnemies = False
        enemyLevels = ""
        enemies = scene.getEnemies(ff7.game.isJapanese(version))
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

        gen.write_line("addBattleScene(" + str(i) + ", " + str(scene.enemyIDs[0]) + ", " + str(scene.enemyIDs[1]) + ", " + str(scene.enemyIDs[2]) + enemyLevels + ");", 4)

        formations = scene.getFormations()
        formationIndex = 0
        for formation in formations:
            formationID = (i * 4) + formationIndex

            noEscape = "false"
            if not formation.canEscape():
                noEscape = "true"

            enemyIDs = ", ".join(map(str, formation.enemyIDs))
            battleArenaIDs = ", ".join(map(str, formation.battleArenaCandidates))
            gen.write_line("addBattleFormation(" + str(i) + ", " + str(formationID) + ", " + noEscape + ", {" + enemyIDs + "}, {" + battleArenaIDs + "});", 4)
            formationIndex += 1

    gen.write_line("")

def outputShops(gen, discPath, verison):
    shopData = ff7.menu.ShopMenuData(ff7.game.retrieveFile(discPath, "MENU", "SHOPMENU.MNU"))

    shopID = 0
    for shop in shopData.shops:
        items = []
        for item in shop.items:
            if item.isMateria:
                items.append(item.id + 512)
            else:
                items.append(item.id)

        shopItems = ", ".join(map(str, items))
        gen.write_line("addShop(" + str(shopID) + ", {" + shopItems + "});", 4)
        shopID += 1

    gen.write_line("")

def outputBosses(gen, discPath, version):
    sceneBin = ff7.scene.Archive(ff7.game.retrieveFile(discPath, "BATTLE", "SCENE.BIN"))

    # Based on: https://pastebin.com/2QGwVma3
    bosses = {
        "Guard Scorpion": [81], 
        "Air Buster": [91],
        "Aps": [96], 
        "Turks:Reno": [103], 
        "Sample:H0512": [114],
        "Hundred Gunner": [115], 
        "Heli Gunner": [115],
        "Rufus": [116],
        "Dark Nation": [116],
        "Motor Ball": [117], 
        "Bottomswell": [120],
        "Jenova‧BIRTH": [122],
        "Dyne": [131],
        "Turks:Reno": [134],
        "Turks:Rude": [134],
        "Gi Nattak": [138],
        "Materia Keeper": [148],
        "Palmer": [150],
        "Red Dragon": [163],
        "Demons Gate": [161],
        "Jenova‧LIFE": [167],
        "Schizo(Right)": [185],
        "Schizo(Left)": [185],
        "Jenova‧DEATH": [185],
        "Eagle Gun": [202],
        "Carry Armor": [195],
        "Turks:Reno": [201],
        "Turks:Rude": [201],
        "Turks:Rude": [204],
        "Lost Number": [144],
        "Rapps": [154],
        "Gorkii": [155],
        "Shake": [156],
        "Chekhov": [156],
        "Staniv": [156],
        "Godo": [157],
        "Diamond Weapon": [245],
        "Turks:Elena": [210],
        "Turks:Reno": [210],
        "Turks:Rude": [210],
        "Hojo": [214],
        "Helletic Hojo": [215],
        "Lifeform-Hojo N": [216],
        "Ultimate Weapon": [70],
        "Emerald Weapon": [246],
        "Ruby Weapon": [245],
        "Jenova‧SYNTHESIS": [227],
        "Bizarro‧Sephiroth": [228],
        "Safer‧Sephiroth": [231],
        "Sephiroth": [231],
    }

    for name, scenes in bosses.items():
        foundEnemy = False

        elementTypes = []
        elementRates = []
        enemyID = 0

        for i in scenes:
            scene = sceneBin.getScene(i)

            enemies = scene.getEnemies(ff7.game.isJapanese(version))
            for j in range(0, len(enemies)):
                enemy = enemies[j]
                id = scene.enemyIDs[j]

                if enemy.name.strip() == name:
                    foundEnemy = True
                    elementTypes = enemy.elementTypes
                    elementRates = enemy.elementRates
                    enemyID = id

        if not foundEnemy:
            print("Did not find boss: " + name)

        final_name = name.replace("‧", "-")
        scenes_string = ", ".join(str(s) for s in scenes)

        uint64_elemTypes = f"0x{bytes(elementTypes).hex().upper()}ULL"
        uint64_elemRates = f"0x{bytes(elementRates).hex().upper()}ULL"

        gen.write_line("addBoss(\"" + final_name + "\", " + str(enemyID) + ", { " + scenes_string + " }, " + uint64_elemTypes + ", " + uint64_elemRates + ");", 4)

    gen.write_line("")

def outputModel(modelName, model):
    parts_strings = []
    for i in range(0, len(model.parts)):
        part = model.parts[i]
        model_part_string = "{" + str(len(part.quad_color_tex)) + ", " + str(len(part.tri_color_tex)) + ", " + str(len(part.quad_mono_tex)) + ", " + str(len(part.tri_mono_tex)) + ", " + str(len(part.tri_mono)) + ", " + str(len(part.quad_mono)) + ", " + str(len(part.tri_color)) + ", " + str(len(part.quad_color)) + "}"
        parts_strings.append(model_part_string)

    parts_string = ", ".join(parts_strings)
    gen.write_line("addModel(\"" + modelName + "\", " + str(model.poly_count) + ", {" + parts_string +  "});", 4)

def outputModelFromField(discPath, fieldFile, modelIndex, modelName):
    models = ff7.models.loadModelsFromBSX(ff7.game.retrieveFile(discPath, "FIELD", fieldFile), discPath)
    if modelIndex >= len(models):
        print("Model index exceeds model count from field file: " + fieldFile)
        return
    
    outputModel(modelName, models[modelIndex])

def outputModels(gen, discPath, version):
    # Global Models
    modelFiles = {"BARRET":     "BALLET.BCX", 
                  "CID":        "CID.BCX", 
                  "CLOUD":      "CLOUD.BCX", 
                  "AERITH":     "EARITH.BCX", 
                  "CAITSITH":   "KETCY.BCX", 
                  "REDXIII":    "RED.BCX", 
                  "TIFA":       "TIFA.BCX", 
                  "VINCENT":    "VINCENT.BCX", 
                  "YUFFIE":     "YUFI.BCX"}
    
    for modelName, modelFile in modelFiles.items():
        model = ff7.models.loadModelFromBCX(ff7.game.retrieveFile(discPath, "FIELD", modelFile))
        outputModel(modelName, model)

    # Field Specific Models
    outputModelFromField(discPath, "MD8_2.BSX", 1, "AERITH_INTRO")
    outputModelFromField(discPath, "MRKT1.BSX", 4, "CLOUD_DRESS")
    outputModelFromField(discPath, "MRKT2.BSX", 3, "AERITH_DRESS")
    outputModelFromField(discPath, "COLNE_4.BSX", 5, "TIFA_DRESS")
    outputModelFromField(discPath, "MD0.BSX", 4, "CLOUD_SWORD")
    outputModelFromField(discPath, "MTCRL_3.BSX", 3, "BARRET_COREL")
    outputModelFromField(discPath, "ITHOS.BSX", 9, "CLOUD_WHEELCHAIR")
    outputModelFromField(discPath, "MD8_5.BSX", 1, "CLOUD_PARACHUTE")
    outputModelFromField(discPath, "MD8_5.BSX", 2, "TIFA_PARACHUTE")
    outputModelFromField(discPath, "MD8_5.BSX", 3, "BARRET_PARACHUTE")
    outputModelFromField(discPath, "MD8_5.BSX", 4, "REDXIII_PARACHUTE")
    outputModelFromField(discPath, "MD8_5.BSX", 5, "CID_PARACHUTE")
    outputModelFromField(discPath, "FSHIP_12.BSX", 5, "YUFFIE_PARACHUTE")

    # Clouds model is slightly different on the world map for some reason, this was extracted from memory.
    gen.write_line("addModel(\"CLOUD_WORLD\", 378, {{0, 0, 0, 0, 0, 0, 12, 6}, {0, 0, 0, 0, 0, 0, 6, 27}, {2, 4 + 2, 0, 0, 0, 0, 148 + 2, 12 + 1}, {0, 0, 0, 0, 0, 0, 10, 9}, {0, 0, 0, 0, 0, 0, 0, 14}, {0, 0, 0, 0, 0, 0, 0, 6}, {0, 0, 0, 0, 0, 0, 10, 9}, {0, 0, 0, 0, 0, 0, 0, 14}, {0, 0, 0, 0, 0, 0, 0, 6}, {0, 0, 0, 0, 0, 0, 8, 4}, {0, 0, 0, 0, 0, 0, 4, 14}, {0, 0, 0, 0, 0, 0, 2, 7}, {0, 0, 0, 0, 0, 0, 8, 4}, {0, 0, 0, 0, 0, 0, 4, 14}, {0, 0, 0, 0, 0, 0, 2, 7}});", 4)
    gen.write_line("")

    # Battle Models
    battleModelFiles = {"BARRET":   "BARRETT.LZS", 
                        "CID":      "CID.LZS", 
                        "CLOUD":    "CLOUD.LZS", 
                        "HICLOUD":  "HICLOUD.LZS",
                        "AERITH":   "EARITH.LZS",  
                        "CAITSITH": "KETCY.LZS", 
                        "REDXIII":  "RED13.LZS",  
                        "TIFA":     "TIFA.LZS", 
                        "VINCENT":  "VINSENT.LZS", 
                        "YUFFIE":   "YUFI.LZS"}

    # These were determined with memory inspection at runtime. Part of it is skeleton data, but the rest is unknown for now.
    battleModelHeaderSizes = {
        "BARRET":   636, 
        "CID":      668,
        "CLOUD":    652,
        "HICLOUD":  652,
        "AERITH":   712,
        "CAITSITH": 812, 
        "REDXIII":  804,
        "TIFA":     700, 
        "VINCENT":  840,
        "YUFFIE":   688
    }

    for modelName, modelFile in battleModelFiles.items():
        battleModel = ff7.models.loadModelFromLZS(ff7.game.retrieveFile(discPath, "ENEMY6", modelFile))

        part_strings = []
        for part in battleModel.parts:
            totalSize = 0
            totalSize += 4 + len(part.vertices) * 8
            totalSize += 4 + len(part.tri_mono_tex) * 16
            totalSize += 4 + len(part.quad_mono_tex) * 20
            totalSize += 4 + len(part.tri_color) * 20
            totalSize += 4 + len(part.quad_color) * 24

            part_strings.append("{" + str(totalSize) + ", " + str(len(part.vertices)) + ", " + str(len(part.tri_mono_tex)) + ", " + str(len(part.quad_mono_tex)) + ", " + str(len(part.tri_color)) + ", " + str(len(part.quad_color)) + "}")
            
        parts_string = ", ".join(part_strings)
        gen.write_line("addBattleModel(\"" + modelName + "\", " + str(battleModelHeaderSizes[modelName]) + ", {" + parts_string + "});", 4)

discPath = sys.argv[1]

if not os.path.isdir(discPath):
    raise EnvironmentError("'%s' is not a directory" % discPath)

# Check that this is a FF7 disc
version, discNumber, execFileName = ff7.game.checkDisc(discPath)

gen = GameDataGenerator()
gen.open_file()
gen.write_header()

outputInventory(gen, discPath, version)
outputOther(gen, discPath, version)
outputFields(gen, discPath, version)
outputWorldMap(gen, discPath, version)
outputShops(gen, discPath, version)
outputBattles(gen, discPath, version)
outputBosses(gen, discPath, version)
outputModels(gen, discPath, version)

gen.write_footer()
gen.close_file()