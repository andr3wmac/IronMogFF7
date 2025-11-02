#include "GameData.h"
#include "core/game/MemoryOffsets.h"
#include "rules/Restrictions.h"

static FieldData gInvalidField = { 0, "" };

std::unordered_map<uint8_t, std::string> GameData::accessoryNames;
std::unordered_map<uint8_t, std::string> GameData::armorNames;
std::unordered_map<uint8_t, std::string> GameData::itemNames;
std::unordered_map<uint8_t, std::string> GameData::weaponNames;
std::unordered_map<uint8_t, std::string> GameData::materiaNames;

std::vector<ESkill> GameData::eSkills;
std::unordered_map<uint16_t, FieldData> GameData::fieldData;
std::vector<WorldMapEntrance> GameData::worldMapEntrances;
std::unordered_map<uint8_t, BattleScene> GameData::battleScenes;
std::unordered_map<std::string, Model> GameData::models;
std::unordered_map<std::string, BattleModel> GameData::battleModels;

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

uint16_t GameData::getRandomAccessory(std::mt19937_64& rng)
{
    static std::vector<uint16_t> keys;
    if (keys.empty())
    {
        for (const auto& pair : GameData::accessoryNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomArmor(std::mt19937_64& rng)
{
    static std::vector<uint16_t> keys;
    if (keys.empty())
    {
        for (const auto& pair : GameData::armorNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomItem(std::mt19937_64& rng)
{
    static std::vector<uint16_t> keys;
    if (keys.empty())
    {
        for (const auto& pair : GameData::itemNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomWeapon(std::mt19937_64& rng)
{
    static std::vector<uint16_t> keys;
    if (keys.empty())
    {
        for (const auto& pair : GameData::weaponNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomMateria(std::mt19937_64& rng, bool excludeBanned)
{
    if (excludeBanned)
    {
        static std::vector<uint16_t> keysWithoutBanned;
        if (keysWithoutBanned.empty())
        {
            for (const auto& pair : GameData::materiaNames)
            {
                if (Restrictions::isMateriaBanned(pair.first))
                {
                    continue;
                }
                keysWithoutBanned.push_back(pair.first);
            }
        }

        std::uniform_int_distribution<size_t> dist(0, keysWithoutBanned.size() - 1);
        return keysWithoutBanned[dist(rng)];
    }

    static std::vector<uint16_t> keys;
    if (keys.empty()) 
    {
        for (const auto& pair : GameData::materiaNames) 
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
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

const char* normalChars[256] = {
    " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
    "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
    "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~",     
    "Ä", "Å", "Ç", "É", "Ñ", "Ö", "Ü", "á", "à", "â", "ä", "ã", "å", "ç", "é", "è",
    "ê", "ë", "í", "ì", "î", "ï", "ñ", "ó", "ò", "ô", "ö", "õ", "ú", "ù", "û", "ü",
    " ", "°", "¢", "£", " ", " ", " ", " ", " ", " ", " ", "´", "¨", " ", " ", "Ø",
    " ", "±", " ", " ", "¥", "µ", " ", " ", " ", " ", " ", "ª", "º", " ", " ", "ø",
    "¿", "¡", "¬", " ", "ƒ", " ", " ", " ", "»", "…", "À", "Ã", "Õ", "Œ", "œ", "–",
    "—", "“", "”", "‘", "’", "÷", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",
    "‚", "„", "‰", "Â", "Ê", "Á", "Ë", "È", "Í", "Î", "Ï", "Ì", "Ó", "Ô", " ", "Ò",
    "Ú", "Û", "Ù", " ", "ˆ", "˜", "¯", " ", " ", " ", " ", " ", " ", " ", " ", " ",
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",
    " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " "
};

std::string GameData::decodeString(const std::vector<uint8_t>& data)
{
    std::string result;
    for (uint8_t byte : data) 
    {
        if (byte == 0xFF) break;
        result += normalChars[byte];
    }
    return result;
}

std::vector<uint8_t> GameData::encodeString(const std::string& input)
{
    std::vector<uint8_t> encoded;

    for (size_t i = 0; i < input.size(); ++i) 
    {
        uint8_t value = 0;
        for (int j = 0; j < 256; ++j)
        {
            if (*normalChars[j] == input.at(i))
            {
                value = j;
                break;
            }
        }
        encoded.push_back(value);
    }

    return encoded;
}