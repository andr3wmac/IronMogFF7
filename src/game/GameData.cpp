#include "GameData.h"

static ItemData gInvalidItem = { 0xFF, "" };

std::unordered_map<uint8_t, ItemData> GameData::accessoryData;
std::unordered_map<uint8_t, ItemData> GameData::armorData;
std::unordered_map<uint8_t, ItemData> GameData::itemData;
std::unordered_map<uint8_t, ItemData> GameData::weaponData;

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