#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <random>

#define SHOP_ITEM_MAX 10

struct ESkill
{
    std::string name = "";
    uint64_t battleData = 0;
};

struct FieldItemData
{
    uint32_t offset = 0;
    uint16_t id = 0;
    uint8_t quantity = 0;
};

struct FieldMessage
{
    uint32_t offset = 0;
    uint32_t strOffset = 0;
    uint32_t strLength = 0;
};

struct FieldShop
{
    uint32_t offset = 0;
    uint8_t shopID = 0;
};

struct FieldWorldExit
{
    uint32_t offset = 0;
    uint8_t index = 0;
    uint16_t fieldID = 0;
};

struct FieldData
{
    std::string name = "";
    std::vector<FieldItemData> items;
    std::vector<FieldItemData> materia;
    std::vector<FieldMessage> messages;
    std::vector<FieldShop> shops;
    std::vector<FieldWorldExit> worldExits;

    bool isValid() { return name != ""; }
};

struct WorldMapEntrance
{
    uint32_t offset = 0;
    uint16_t fieldID = 0;
    uint32_t centerX = 0;
    uint32_t centerZ = 0;
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

    static uint16_t getRandomAccessory(std::mt19937_64& rng);
    static uint16_t getRandomArmor(std::mt19937_64& rng);
    static uint16_t getRandomItem(std::mt19937_64& rng);
    static uint16_t getRandomWeapon(std::mt19937_64& rng);
    static uint16_t getRandomMateria(std::mt19937_64& rng);

    static FieldData getField(uint16_t id);
    static std::string getNameFromFieldScriptID(uint16_t fieldScriptID);
    static std::string getNameFromBattleDropID(uint16_t battleDropID);

    static std::string decodeString(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> encodeString(const std::string& input);

    static std::unordered_map<uint8_t, std::string> accessoryNames;
    static std::unordered_map<uint8_t, std::string> armorNames;
    static std::unordered_map<uint8_t, std::string> itemNames;
    static std::unordered_map<uint8_t, std::string> weaponNames;
    static std::unordered_map<uint8_t, std::string> materiaNames;
    static std::vector<ESkill> eSkills;

    static std::unordered_map<uint16_t, FieldData> fieldData;

    static std::vector<WorldMapEntrance> worldMapEntrances;
};

#define ADD_ACCESSORY(ID, NAME) accessoryNames[ID] = NAME;
#define ADD_ARMOR(ID, NAME) armorNames[ID] = NAME;
#define ADD_ITEM(ID, NAME) itemNames[ID] = NAME;
#define ADD_WEAPON(ID, NAME) weaponNames[ID] = NAME;
#define ADD_MATERIA(ID, NAME) materiaNames[ID] = NAME;
#define ADD_ESKILL(NAME, BATTLE_DATA) GameData::eSkills.push_back({NAME, BATTLE_DATA});

#define ADD_FIELD(FIELD_ID, NAME) fieldData[FIELD_ID] = {NAME};

#define ADD_FIELD_ITEM(FIELD_ID, OFFSET, ITEM_ID, QUANTITY) \
    { GameData::fieldData[FIELD_ID].items.push_back({OFFSET, ITEM_ID, QUANTITY}); }

#define ADD_FIELD_MATERIA(FIELD_ID, OFFSET, MAT_ID) \
    { GameData::fieldData[FIELD_ID].materia.push_back({OFFSET, MAT_ID, 1}); }

#define ADD_FIELD_MESSAGE(FIELD_ID, OFFSET, STR_OFFSET, STR_LEN) \
    { GameData::fieldData[FIELD_ID].messages.push_back({OFFSET, STR_OFFSET, STR_LEN}); }

#define ADD_FIELD_SHOP(FIELD_ID, OFFSET, SHOP_ID) \
    { GameData::fieldData[FIELD_ID].shops.push_back({OFFSET, SHOP_ID}); }

#define ADD_FIELD_WORLD_EXIT(FIELD_ID, OFFSET, INDEX, TARGET_FIELD_ID) \
    { GameData::fieldData[FIELD_ID].worldExits.push_back({OFFSET, INDEX, TARGET_FIELD_ID}); }

#define ADD_WORLDMAP_ENTRANCE(OFFSET, FIELD_ID, CENTER_X, CENTER_Z) GameData::worldMapEntrances.push_back({OFFSET, FIELD_ID, CENTER_X, CENTER_Z});