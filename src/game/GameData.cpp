#include "GameData.h"
#include "game/MemoryOffsets.h"

static ItemData gInvalidItem = { 0xFF, "" };
static FieldData gInvalidField = { "" };

std::unordered_map<uint8_t, ItemData> GameData::accessoryData;
std::unordered_map<uint8_t, ItemData> GameData::armorData;
std::unordered_map<uint8_t, ItemData> GameData::itemData;
std::unordered_map<uint8_t, ItemData> GameData::weaponData;

std::unordered_map<uint16_t, FieldData> GameData::fieldData;

ItemData GameData::getAccessory(uint8_t id)
{
    if (accessoryData.count(id) == 0)
    {
        return gInvalidItem;
    }

    return accessoryData[id];
}

ItemData GameData::getArmor(uint8_t id)
{
    if (armorData.count(id) == 0)
    {
        return gInvalidItem;
    }

    return armorData[id];
}

ItemData GameData::getItem(uint8_t id)
{
    if (itemData.count(id) == 0)
    {
        return gInvalidItem;
    }

    return itemData[id];
}

ItemData GameData::getWeapon(uint8_t id)
{
    if (weaponData.count(id) == 0)
    {
        return gInvalidItem;
    }

    return weaponData[id];
}

FieldData GameData::getField(uint16_t id)
{
    if (fieldData.count(id) == 0)
    {
        return gInvalidField;
    }

    return fieldData[id];
}

ItemData GameData::getItemDataFromFieldItemID(uint16_t fieldItemID)
{
    /*
    Field Item ID Conversion:
        0   + X = Items
        128 + X = Weapons
        256 + X = Armor
        288 + X = Accessories
    */

    if (fieldItemID < 128)
    {
        return getItem((uint8_t)fieldItemID);
    }
    else if (fieldItemID < 256)
    {
        return getWeapon((uint8_t)(fieldItemID - 128));
    }
    else if (fieldItemID < 288)
    {
        return getArmor((uint8_t)(fieldItemID - 256));
    }
    else
    {
        return getAccessory((uint8_t)(fieldItemID - 288));
    }
}

ItemData GameData::getItemDataFromBattleDropID(uint16_t battleDropID)
{
    std::pair<uint8_t, uint16_t> data = unpackDropID(battleDropID);
    if (data.first == DropType::Accessory)
    {
        return getAccessory(data.second);
    }
    if (data.first == DropType::Armor)
    {
        return getArmor(data.second);
    }
    if (data.first == DropType::Item)
    {
        return getItem(data.second);
    }
    if (data.first == DropType::Weapon)
    {
        return getWeapon(data.second);
    }
    return gInvalidItem;
}