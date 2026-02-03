#include "GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "rules/Restrictions.h"

static FieldData gInvalidField = { 0, "" };

std::unordered_map<uint8_t, Item> GameData::accessories;
std::unordered_map<uint8_t, Item> GameData::armors;
std::unordered_map<uint8_t, Item> GameData::items;
std::unordered_map<uint8_t, Item> GameData::weapons;
std::unordered_map<uint8_t, Item> GameData::materia;

std::vector<ESkill> GameData::eSkills;
std::unordered_map<uint16_t, FieldData> GameData::fieldData;
std::vector<WorldMapEntrance> GameData::worldMapEntrances;
std::vector<WorldMapEncounters> GameData::worldMapEncounters;
std::unordered_map<uint8_t, BattleScene> GameData::battleScenes;
std::vector<Boss> GameData::bosses;
std::unordered_map<uint8_t, Shop> GameData::shops;
std::vector<Model> GameData::models;
std::vector<BattleModel> GameData::battleModels;

Item* GameData::getAccessory(uint8_t id)
{
    if (accessories.count(id) == 0)
    {
        LOG("Invalid accessory ID: %d", id);
        return nullptr;
    }

    return &accessories[id];
}

Item* GameData::getArmor(uint8_t id)
{
    if (armors.count(id) == 0)
    {
        LOG("Invalid armor ID: %d", id);
        return nullptr;
    }

    return &armors[id];
}

Item* GameData::getItem(uint8_t id)
{
    if (items.count(id) == 0)
    {
        LOG("Invalid item ID: %d", id);
        return nullptr;
    }

    return &items[id];
}

Item* GameData::getWeapon(uint8_t id)
{
    if (weapons.count(id) == 0)
    {
        LOG("Invalid weapon ID: %d", id);
        return nullptr;
    }

    return &weapons[id];
}

Item* GameData::getMateria(uint8_t id)
{
    if (materia.count(id) == 0)
    {
        LOG("Invalid materia ID: %d", id);
        return nullptr;
    }

    return &materia[id];
}

std::string GameData::getItemName(uint16_t fieldScriptID)
{
    /*
      Item ID Conversion:
        0   + X = Items
        128 + X = Weapons
        256 + X = Armor
        288 + X = Accessories
    */

    Item* item = nullptr;

    if (fieldScriptID < 128)
    {
        item = getItem((uint8_t)fieldScriptID);
    }
    else if (fieldScriptID < 256)
    {
        item = getWeapon((uint8_t)(fieldScriptID - 128));
    }
    else if (fieldScriptID < 288)
    {
        item = getArmor((uint8_t)(fieldScriptID - 256));
    }
    else
    {
        item = getAccessory((uint8_t)(fieldScriptID - 288));
    }

    if (item == nullptr)
    {
        return "";
    }

    return item->name;
}

uint32_t GameData::getItemPrice(uint16_t fieldScriptID)
{
    Item* item = nullptr;

    if (fieldScriptID < 128)
    {
        item = getItem((uint8_t)fieldScriptID);
    }
    else if (fieldScriptID < 256)
    {
        item = getWeapon((uint8_t)(fieldScriptID - 128));
    }
    else if (fieldScriptID < 288)
    {
        item = getArmor((uint8_t)(fieldScriptID - 256));
    }
    else
    {
        item = getAccessory((uint8_t)(fieldScriptID - 288));
    }

    if (item == nullptr)
    {
        return 0;
    }

    return item->price;
}

std::string GameData::getMateriaName(uint8_t id)
{
    Item* materia = getMateria(id);
    if (materia == nullptr)
    {
        return "";
    }
    return materia->name;
}

uint32_t GameData::getMateriaPrice(uint8_t id)
{
    Item* materia = getMateria(id);
    if (materia == nullptr)
    {
        return 0;
    }
    return materia->price;
}

uint16_t GameData::getRandomAccessory(std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;
    candidates.reserve(GameData::accessories.size());

    for (const auto& [id, data] : GameData::accessories)
    {
        if ((excludeBanned && Restrictions::isAccessoryBanned(id)) ||
            (excludeRare && data.price == 2) ||
            (excludeSet.count(288 + id) > 0))
        {
            continue;
        }

        candidates.push_back(id);
    }

    if (candidates.empty())
    {
        LOG("No accessory selected from getRandomAccessory.");
        return 0;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

uint16_t GameData::getRandomArmor(std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;
    candidates.reserve(GameData::armors.size());

    for (const auto& [id, data] : GameData::armors)
    {
        if ((excludeBanned && Restrictions::isArmorBanned(id)) ||
            (excludeRare && data.price == 2) ||
            (excludeSet.count(256 + id) > 0))
        {
            continue;
        }

        candidates.push_back(id);
    }

    if (candidates.empty())
    {
        LOG("No armor selected from getRandomArmor.");
        return 0;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

uint16_t GameData::getRandomItem(std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;
    candidates.reserve(GameData::items.size());

    for (const auto& [id, data] : GameData::items)
    {
        if ((excludeBanned && Restrictions::isItemBanned(id)) ||
            (excludeRare && data.price == 2) ||
            (excludeSet.count(id) > 0))
        {
            continue;
        }

        candidates.push_back(id);
    }

    if (candidates.empty())
    {
        LOG("No item selected from getRandomItem.");
        return 0;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

uint16_t GameData::getRandomWeapon(std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;
    candidates.reserve(GameData::weapons.size());

    for (const auto& [id, data] : GameData::weapons)
    {
        if ((excludeBanned && Restrictions::isWeaponBanned(id)) ||
            (excludeRare && data.price == 2) ||
            (excludeSet.count(128 + id) > 0))
        {
            continue;
        }

        candidates.push_back(id);
    }

    if (candidates.empty())
    {
        LOG("No weapon selected from getRandomWeapon.");
        return 0;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

uint16_t GameData::getRandomItemFromID(uint16_t origItemID, std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
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
        return getRandomItem(rng, excludeBanned, excludeRare, excludeSet);
    }
    else if (origItemID < 256)
    {
        return getRandomWeapon(rng, excludeBanned, excludeRare, excludeSet) + 128;
    }
    else if (origItemID < 288)
    {
        return getRandomArmor(rng, excludeBanned, excludeRare, excludeSet) + 256;
    }
    else
    {
        return getRandomAccessory(rng, excludeBanned, excludeRare, excludeSet) + 288;
    }

    return origItemID;
}

uint16_t GameData::getRandomMateria(std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;
    candidates.reserve(GameData::materia.size());

    for (const auto& [id, data] : GameData::materia)
    {
        if ((excludeBanned && Restrictions::isMateriaBanned(id)) ||
            (excludeRare && data.price == 1) ||
            (excludeSet.count(id) > 0))
        {
            continue;
        }

        candidates.push_back(id);
    }

    if (candidates.empty())
    {
        LOG("No materia selected from getRandomMateria.");
        return 0;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

FieldData GameData::getField(uint16_t id)
{
    if (fieldData.count(id) == 0)
    {
        return gInvalidField;
    }

    return fieldData[id];
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
        for (int enemyID : scene->enemyIDs)
        {
            if (enemyID == boss.id)
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