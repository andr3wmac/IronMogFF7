#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct ItemData
{
    uint8_t id = 0xFF;
    std::string name = "";

    bool isValid() { return id != 0xFF; }
};

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

    bool isValid() { return name != ""; }
};

class GameData
{
public:
    static void loadGameData();

    static ItemData getAccessory(uint8_t id);
    static ItemData getArmor(uint8_t id);
    static ItemData getItem(uint8_t id);
    static ItemData getWeapon(uint8_t id);

    static FieldData getField(uint16_t id);
    static ItemData getItemDataFromFieldItemID(uint16_t fieldItemID);
    static ItemData getItemDataFromBattleDropID(uint16_t battleDropID);

    static std::unordered_map<uint8_t, ItemData> accessoryData;
    static std::unordered_map<uint8_t, ItemData> armorData;
    static std::unordered_map<uint8_t, ItemData> itemData;
    static std::unordered_map<uint8_t, ItemData> weaponData;

    static std::unordered_map<uint16_t, FieldData> fieldData;
};

#define ADD_ACCESSORY(ID, NAME) accessoryData[ID] = {ID, NAME};
#define ADD_ARMOR(ID, NAME) armorData[ID] = {ID, NAME};
#define ADD_ITEM(ID, NAME) itemData[ID] = {ID, NAME};
#define ADD_WEAPON(ID, NAME) weaponData[ID] = {ID, NAME};

#define ADD_FIELD(FIELD_ID, NAME) fieldData[FIELD_ID] = {NAME};
#define ADD_FIELD_ITEM(FIELD_ID, OFFSET, ITEM_ID, QUANTITY) \
    { fieldData[FIELD_ID].items.push_back({OFFSET, ITEM_ID, QUANTITY}); }