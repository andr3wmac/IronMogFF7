#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct FieldItemData
{
    uint32_t offset = 0;
    uint16_t id = 0;
    uint8_t quantity = 0;
};

struct FieldData
{
    std::string name = "";
    std::vector<FieldItemData> items;
    std::vector<FieldItemData> materia;

    bool isValid() { return name != ""; }
};

class GameData
{
public:
    static void loadGameData();

    static std::string getAccessoryName(uint8_t id);
    static std::string getArmorName(uint8_t id);
    static std::string getItemName(uint8_t id);
    static std::string getWeaponName(uint8_t id);
    static std::string getMateriaName(uint8_t id);

    static FieldData getField(uint16_t id);
    static std::string getNameFromFieldScriptID(uint16_t fieldScriptID);
    static std::string getNameFromBattleDropID(uint16_t battleDropID);

    static std::unordered_map<uint8_t, std::string> accessoryNames;
    static std::unordered_map<uint8_t, std::string> armorNames;
    static std::unordered_map<uint8_t, std::string> itemNames;
    static std::unordered_map<uint8_t, std::string> weaponNames;
    static std::unordered_map<uint8_t, std::string> materiaNames;

    static std::unordered_map<uint16_t, FieldData> fieldData;
};

#define ADD_ACCESSORY(ID, NAME) accessoryNames[ID] = NAME;
#define ADD_ARMOR(ID, NAME) armorNames[ID] = NAME;
#define ADD_ITEM(ID, NAME) itemNames[ID] = NAME;
#define ADD_WEAPON(ID, NAME) weaponNames[ID] = NAME;
#define ADD_MATERIA(ID, NAME) materiaNames[ID] = NAME;

#define ADD_FIELD(FIELD_ID, NAME) fieldData[FIELD_ID] = {NAME};

#define ADD_FIELD_ITEM(FIELD_ID, OFFSET, ITEM_ID, QUANTITY) \
    { fieldData[FIELD_ID].items.push_back({OFFSET, ITEM_ID, QUANTITY}); }

#define ADD_FIELD_MATERIA(FIELD_ID, OFFSET, MAT_ID) \
    { fieldData[FIELD_ID].materia.push_back({OFFSET, MAT_ID, 1}); }
