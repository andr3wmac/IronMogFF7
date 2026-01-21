#include "GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
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
std::vector<Boss> GameData::bosses;
std::vector<Model> GameData::models;
std::vector<BattleModel> GameData::battleModels;

std::string GameData::getAccessoryName(uint8_t id)
{
    if (accessoryNames.count(id) == 0)
    {
        LOG("Invalid accessory ID: %d", id);
        return "";
    }

    return accessoryNames[id];
}

std::string GameData::getArmorName(uint8_t id)
{
    if (armorNames.count(id) == 0)
    {
        LOG("Invalid armor ID: %d", id);
        return "";
    }

    return armorNames[id];
}

std::string GameData::getItemName(uint8_t id)
{
    if (itemNames.count(id) == 0)
    {
        LOG("Invalid item ID: %d", id);
        return "";
    }

    return itemNames[id];
}

std::string GameData::getWeaponName(uint8_t id)
{
    if (weaponNames.count(id) == 0)
    {
        LOG("Invalid weapon ID: %d", id);
        return "";
    }

    return weaponNames[id];
}

std::string GameData::getMateriaName(uint8_t id)
{
    if (materiaNames.count(id) == 0)
    {
        LOG("Invalid materia ID: %d", id);
        return "";
    }

    return materiaNames[id];
}

uint16_t GameData::getRandomAccessory(std::mt19937_64& rng, bool excludeBanned)
{
    if (excludeBanned)
    {
        static std::vector<uint16_t> keysWithoutBanned;
        if (keysWithoutBanned.empty())
        {
            for (const auto& pair : GameData::accessoryNames)
            {
                if (Restrictions::isAccessoryBanned(pair.first))
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
        for (const auto& pair : GameData::accessoryNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomArmor(std::mt19937_64& rng, bool excludeBanned)
{
    if (excludeBanned)
    {
        static std::vector<uint16_t> keysWithoutBanned;
        if (keysWithoutBanned.empty())
        {
            for (const auto& pair : GameData::armorNames)
            {
                if (Restrictions::isArmorBanned(pair.first))
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
        for (const auto& pair : GameData::armorNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomItem(std::mt19937_64& rng, bool excludeBanned)
{
    if (excludeBanned)
    {
        static std::vector<uint16_t> keysWithoutBanned;
        if (keysWithoutBanned.empty())
        {
            for (const auto& pair : GameData::itemNames)
            {
                if (Restrictions::isItemBanned(pair.first))
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
        for (const auto& pair : GameData::itemNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomWeapon(std::mt19937_64& rng, bool excludeBanned)
{
    if (excludeBanned)
    {
        static std::vector<uint16_t> keysWithoutBanned;
        if (keysWithoutBanned.empty())
        {
            for (const auto& pair : GameData::weaponNames)
            {
                if (Restrictions::isWeaponBanned(pair.first))
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
        for (const auto& pair : GameData::weaponNames)
        {
            keys.push_back(pair.first);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, keys.size() - 1);
    return keys[dist(rng)];
}

uint16_t GameData::getRandomItemFromID(uint16_t origItemID, std::mt19937_64& rng, bool excludeBanned)
{
    /*
      Item ID Conversion:
        0   + X = Items
        128 + X = Weapons
        256 + X = Armor
        288 + X = Accessories
    */

    if (origItemID < 128)
    {
        return getRandomItem(rng);
    }
    else if (origItemID < 256)
    {
        return getRandomWeapon(rng) + 128;
    }
    else if (origItemID < 288)
    {
        return getRandomArmor(rng) + 256;
    }
    else
    {
        return getRandomAccessory(rng) + 288;
    }

    return origItemID;
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

std::string GameData::getItemNameFromID(uint16_t fieldItemID)
{
    /*
      Item ID Conversion:
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

BattleModel* GameData::getBattleModel(std::string modelName)
{
    for (BattleModel& model : battleModels)
    {
        if (model.name == modelName)
        {
            return &model;
        }
    }

    return nullptr;
}

std::vector<const Boss*> GameData::getBossesInScene(const BattleScene* scene)
{
    std::vector<const Boss*> result;
    for (const Boss& boss : GameData::bosses)
    {
        bool bossInScene = false;
        for (int bossSceneID : boss.sceneIDs)
        {
            if (bossSceneID == scene->id)
            {
                result.push_back(&boss);
                break;
            }
        }
    }
    return result;
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
    std::string result = "";
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