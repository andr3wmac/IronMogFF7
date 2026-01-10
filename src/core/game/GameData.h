#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <random>

#define SHOP_ITEM_MAX 10
#define ESKILL_EMPTY 562949953421567

struct ESkill
{
    std::string name = "";

    uint8_t targetFlags = 0;
    uint32_t mpCost = 0;
    uint8_t index = 0;

    constexpr uint64_t uint64() const
    {
        return (static_cast<uint64_t>(index) & 0xFF) | (static_cast<uint64_t>(mpCost) & 0xFFFFFFFF) << 8 | (static_cast<uint64_t>(targetFlags) & 0xFF) << 40;
    }
};

struct FieldScriptItem
{
    uint8_t group = 0;
    uint8_t script = 0;
    uint32_t offset = 0;
    uint16_t id = 0;
    uint8_t quantity = 0;
};

struct FieldScriptMessage
{
    uint8_t group = 0;
    uint8_t script = 0;
    uint32_t offset = 0;
    uint32_t strOffset = 0;
    uint32_t strLength = 0;
};

struct FieldScriptShop
{
    uint8_t group = 0;
    uint8_t script = 0;
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
    uint16_t id = 0;
    std::string name = "";
    std::vector<FieldScriptItem> items;
    std::vector<FieldScriptItem> materia;
    std::vector<FieldScriptMessage> messages;
    std::vector<FieldScriptShop> shops;
    std::vector<FieldWorldExit> worldExits;
    std::vector<uint8_t> modelIDs;

    bool isValid() { return name != ""; }
};

struct WorldMapEntrance
{
    uint32_t offset = 0;
    uint16_t fieldID = 0;
    std::string fieldName = "";
    uint32_t centerX = 0;
    uint32_t centerZ = 0;
};

struct BattleFormation
{
    uint16_t id = 0;
    bool noEscape = false;
    int enemyIDs[6];
};

struct BattleScene
{
    uint8_t id = 0;
    int enemyIDs[3];
    int enemyLevels[3];
    std::vector<BattleFormation> formations;
};

struct Boss
{
    std::string name = "";
    uint16_t id = 0;
    std::vector<int> sceneIDs;
    uint64_t elementTypes = 0;
    uint64_t elementRates = 0;
};

struct ModelPart
{
    int quadColorTex = 0;
    int triColorTex  = 0;
    int quadMonoTex  = 0;
    int triMonoTex   = 0;
    int triMono      = 0;
    int quadMono     = 0;
    int triColor     = 0;
    int quadColor    = 0;
};

struct Model
{
    std::string name = "";
    int polyCount = 0;
    std::vector<ModelPart> parts;
};

struct BattleModelPart
{
    int sizeInBytes = 0;
    int vertexCount = 0;
    int triMonoTex  = 0;
    int quadMonoTex = 0;
    int triColor    = 0;
    int quadColor   = 0;
};

struct BattleModel
{
    std::string name = "";
    int headerSize = 0;
    std::vector<BattleModelPart> parts;
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

    static uint16_t getRandomAccessory(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomArmor(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomItem(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomWeapon(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomMateria(std::mt19937_64& rng, bool excludeBanned = true);

    // Returns a random item ID thats the same type as origItemID
    static uint16_t getRandomItemFromID(uint16_t origItemID, std::mt19937_64& rng, bool excludeBanned = true);

    static FieldData getField(uint16_t id);
    static std::string getItemNameFromID(uint16_t fieldScriptID);

    static BattleModel* getBattleModel(std::string modelName);
    static std::vector<const Boss*> getBossesInScene(const BattleScene* scene);

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
    static std::unordered_map<uint8_t, BattleScene> battleScenes;
    static std::vector<Boss> bosses;
    static std::vector<Model> models;
    static std::vector<BattleModel> battleModels;
};

#define ADD_ACCESSORY(ID, NAME) accessoryNames[ID] = NAME;
#define ADD_ARMOR(ID, NAME) armorNames[ID] = NAME;
#define ADD_ITEM(ID, NAME) itemNames[ID] = NAME;
#define ADD_WEAPON(ID, NAME) weaponNames[ID] = NAME;
#define ADD_MATERIA(ID, NAME) materiaNames[ID] = NAME;
#define ADD_ESKILL(NAME, TARGET_FLAGS, MP_COST, IDX) GameData::eSkills.push_back({NAME, TARGET_FLAGS, MP_COST, IDX});

#define ADD_FIELD(FIELD_ID, NAME) fieldData[FIELD_ID] = {FIELD_ID, NAME};

#define FIELD_SCRIPT_ITEM(FIELD_ID, GROUP_IDX, SCRIPT_IDX, OFFSET, ITEM_ID, QUANTITY) \
    { GameData::fieldData[FIELD_ID].items.push_back({GROUP_IDX, SCRIPT_IDX, OFFSET, ITEM_ID, QUANTITY}); }

#define FIELD_SCRIPT_MATERIA(FIELD_ID, GROUP_IDX, SCRIPT_IDX, OFFSET, MAT_ID) \
    { GameData::fieldData[FIELD_ID].materia.push_back({GROUP_IDX, SCRIPT_IDX, OFFSET, MAT_ID, 1}); }

#define FIELD_SCRIPT_MESSAGE(FIELD_ID, GROUP_IDX, SCRIPT_IDX, OFFSET, STR_OFFSET, STR_LEN) \
    { GameData::fieldData[FIELD_ID].messages.push_back({GROUP_IDX, SCRIPT_IDX, OFFSET, STR_OFFSET, STR_LEN}); }

#define FIELD_SCRIPT_SHOP(FIELD_ID, GROUP_IDX, SCRIPT_IDX, OFFSET, SHOP_ID) \
    { GameData::fieldData[FIELD_ID].shops.push_back({GROUP_IDX, SCRIPT_IDX, OFFSET, SHOP_ID}); }

#define ADD_FIELD_WORLD_EXIT(FIELD_ID, OFFSET, INDEX, TARGET_FIELD_ID) \
    { GameData::fieldData[FIELD_ID].worldExits.push_back({OFFSET, INDEX, TARGET_FIELD_ID}); }

#define ADD_FIELD_MODELS(FIELD_ID, ...) \
    { GameData::fieldData[FIELD_ID].modelIDs = {__VA_ARGS__}; }

#define ADD_WORLDMAP_ENTRANCE(OFFSET, FIELD_ID, FIELD_NAME, CENTER_X, CENTER_Z) GameData::worldMapEntrances.push_back({OFFSET, FIELD_ID, FIELD_NAME, CENTER_X, CENTER_Z});

#define ADD_BATTLE_SCENE(SCENE_ID, ENEMY_ID0, ENEMY_ID1, ENEMY_ID2, ENEMY_LEVEL0, ENEMY_LEVEL1, ENEMY_LEVEL2) GameData::battleScenes[SCENE_ID] = {SCENE_ID, {ENEMY_ID0, ENEMY_ID1, ENEMY_ID2}, {ENEMY_LEVEL0, ENEMY_LEVEL1, ENEMY_LEVEL2}};

#define ADD_BATTLE_FORMATION(FORMATION_ID, SCENE_ID, NO_ESCAPE, ...) GameData::battleScenes[SCENE_ID].formations.push_back({FORMATION_ID, NO_ESCAPE, {__VA_ARGS__}});
#define ADD_BOSS(NAME, ENEMY_ID, SCENE_IDS, ELEM_TYPES, ELEM_RATES) GameData::bosses.push_back({ NAME, ENEMY_ID, SCENE_IDS, ELEM_TYPES, ELEM_RATES });

#define ADD_MODEL(MODEL_NAME, POLY_COUNT, ...) { GameData::models.push_back({ MODEL_NAME, POLY_COUNT, __VA_ARGS__ }); }
#define ADD_BATTLE_MODEL(MODEL_NAME, ...) { GameData::battleModels.push_back({ MODEL_NAME, __VA_ARGS__ }); }