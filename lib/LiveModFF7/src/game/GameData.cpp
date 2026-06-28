#include "LiveModFF7/game/GameData.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"

static FieldData gInvalidField = { 0, "" };

std::unordered_map<uint16_t, Item> GameData::items;
std::unordered_map<uint16_t, Item> GameData::materia;
std::vector<ESkill> GameData::eSkills;
std::unordered_map<uint16_t, FieldData> GameData::fieldData;
std::vector<WorldMapEntrance> GameData::worldMapEntrances;
std::vector<WorldMapEncounters> GameData::worldMapEncounters;
std::unordered_map<uint8_t, BattleScene> GameData::battleScenes;
std::vector<Boss> GameData::bosses;
std::unordered_map<uint8_t, Shop> GameData::shops;
std::vector<Model> GameData::models;
std::vector<BattleModel> GameData::battleModels;

std::string StatMultiplierSet::toString()
{
    return  std::to_string(currentHP) + ", " +
            std::to_string(maxHP) + ", " +
            std::to_string(currentMP) + ", " +
            std::to_string(maxMP) + ", " +
            std::to_string(strength) + ", " +
            std::to_string(magic) + ", " +
            std::to_string(evade) + ", " +
            std::to_string(speed) + ", " +
            std::to_string(luck) + ", " +
            std::to_string(defense) + ", " +
            std::to_string(mDefense);
}

void GameData::clearGameData()
{
    items.clear();
    materia.clear();
    eSkills.clear();
    fieldData.clear();
    worldMapEntrances.clear();
    worldMapEncounters.clear();
    battleScenes.clear();
    bosses.clear();
    shops.clear();
    models.clear();
    battleModels.clear();
}

Item* GameData::getItem(uint16_t id)
{
    if (items.count(id) == 0)
    {
        LOG("Invalid item ID: %d", id);
        return nullptr;
    }

    return &items[id];
}

Item* GameData::getMateria(uint16_t id)
{
    if (materia.count(id) == 0)
    {
        LOG("Invalid materia ID: %d", id);
        return nullptr;
    }

    return &materia[id];
}

std::string GameData::getItemName(uint16_t itemID)
{
    Item* item = getItem(itemID);
    if (item == nullptr)
    {
        return "";
    }
    return item->name;
}

uint32_t GameData::getItemPrice(uint16_t itemID)
{
    Item* item = getItem(itemID);
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
    "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�",
    "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�",
    " ", "�", "�", "�", " ", " ", " ", " ", " ", " ", " ", "�", "�", " ", " ", "�",
    " ", "�", " ", " ", "�", "�", " ", " ", " ", " ", " ", "�", "�", " ", " ", "�",
    "�", "�", "�", " ", "�", " ", " ", " ", "�", "�", "�", "�", "�", "�", "�", "�",
    "�", "�", "�", "�", "�", "�", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ",
    "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", "�", " ", "�",
    "�", "�", "�", " ", "�", "�", "�", " ", " ", " ", " ", " ", " ", " ", " ", " ",
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