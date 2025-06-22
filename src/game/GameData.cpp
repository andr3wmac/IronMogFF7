#include "GameData.h"
#include "game/MemoryOffsets.h"

static FieldData gInvalidField = { "" };

std::unordered_map<uint8_t, std::string> GameData::accessoryNames;
std::unordered_map<uint8_t, std::string> GameData::armorNames;
std::unordered_map<uint8_t, std::string> GameData::itemNames;
std::unordered_map<uint8_t, std::string> GameData::weaponNames;
std::unordered_map<uint8_t, std::string> GameData::materiaNames;

std::unordered_map<uint16_t, FieldData> GameData::fieldData;

std::string GameData::getAccessoryName(uint8_t id)
{
    if (accessoryNames.count(id) == 0)
    {
        return "";
    }

    return accessoryNames[id];
}

std::string GameData::getArmorName(uint8_t id)
{
    if (armorNames.count(id) == 0)
    {
        return "";
    }

    return armorNames[id];
}

std::string GameData::getItemName(uint8_t id)
{
    if (itemNames.count(id) == 0)
    {
        return "";
    }

    return itemNames[id];
}

std::string GameData::getWeaponName(uint8_t id)
{
    if (weaponNames.count(id) == 0)
    {
        return "";
    }

    return weaponNames[id];
}

std::string GameData::getMateriaName(uint8_t id)
{
    if (materiaNames.count(id) == 0)
    {
        return "";
    }

    return materiaNames[id];
}

FieldData GameData::getField(uint16_t id)
{
    if (fieldData.count(id) == 0)
    {
        return gInvalidField;
    }

    return fieldData[id];
}

std::string GameData::getNameFromFieldScriptID(uint16_t fieldItemID)
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
        return getItemName((uint8_t)fieldItemID);
    }
    else if (fieldItemID < 256)
    {
        return getWeaponName((uint8_t)(fieldItemID - 128));
    }
    else if (fieldItemID < 288)
    {
        return getArmorName((uint8_t)(fieldItemID - 256));
    }
    else
    {
        return getAccessoryName((uint8_t)(fieldItemID - 288));
    }
}

std::string GameData::getNameFromBattleDropID(uint16_t battleDropID)
{
    std::pair<uint8_t, uint16_t> data = unpackDropID(battleDropID);
    if (data.first == DropType::Accessory)
    {
        return getAccessoryName((uint8_t)data.second);
    }
    if (data.first == DropType::Armor)
    {
        return getArmorName((uint8_t)data.second);
    }
    if (data.first == DropType::Item)
    {
        return getItemName((uint8_t)data.second);
    }
    if (data.first == DropType::Weapon)
    {
        return getWeaponName((uint8_t)data.second);
    }
    return "";
}