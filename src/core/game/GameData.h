#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>

#define SHOP_ITEM_MAX 10
#define ESKILL_EMPTY 562949953421567

struct Item
{
    std::string name = "";
    uint32_t price = 0;
};

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
    uint8_t window = 0;
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

struct ShopItem
{
    uint8_t index;
    uint16_t id;
};

struct Shop
{
    std::vector<ShopItem> items;
    std::vector<ShopItem> materia;
};

struct BattleFormation
{
    uint16_t id = 0;
    bool noEscape = false;
    std::array<uint16_t, 6> enemyIDs;
    std::array<uint16_t, 4> arenaIDs;

    inline bool isArenaBattle()
    {
        return arenaIDs[0] != 999 || arenaIDs[1] != 999 || arenaIDs[2] != 999 || arenaIDs[3] != 999;
    }
};

struct BattleScene
{
    uint8_t id = 0;
    std::array<uint16_t, 3> enemyIDs;
    std::array<uint8_t, 3> enemyLevels;
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
    static std::unordered_map<uint8_t, Item> accessories;
    static std::unordered_map<uint8_t, Item> armors;
    static std::unordered_map<uint8_t, Item> items;
    static std::unordered_map<uint8_t, Item> weapons;
    static std::unordered_map<uint8_t, Item> materia;

    static std::vector<ESkill> eSkills;
    static std::unordered_map<uint16_t, FieldData> fieldData;
    static std::vector<WorldMapEntrance> worldMapEntrances;
    static std::unordered_map<uint8_t, Shop> shops;
    static std::unordered_map<uint8_t, BattleScene> battleScenes;
    static std::vector<Boss> bosses;
    static std::vector<Model> models;
    static std::vector<BattleModel> battleModels;

    static void loadGameData();

    static void addAccessory(uint8_t id, std::string name, uint32_t shopPrice)      { accessories[id] = { name, shopPrice }; }
    static void addArmor(uint8_t id, const std::string& name, uint32_t shopPrice)   { armors[id] = { name, shopPrice }; }
    static void addItem(uint8_t id, const std::string& name, uint32_t shopPrice)    { items[id] = { name, shopPrice }; }
    static void addWeapon(uint8_t id, const std::string& name, uint32_t shopPrice)  { weapons[id] = { name, shopPrice }; }
    static void addMateria(uint8_t id, const std::string& name, uint32_t shopPrice) { materia[id] = { name, shopPrice }; }

    static void addESkill(const std::string& name, uint8_t targetFlags, uint32_t mpCost, uint8_t idx) {
        GameData::eSkills.push_back({ name, targetFlags, mpCost, idx });
    }

    static void addField(uint16_t fieldID, const std::string& name) {
        fieldData[fieldID] = { fieldID, name };
    }

    static void addFieldScriptItem(uint16_t fieldID, uint8_t groupIdx, uint8_t scriptIdx, uint32_t offset, uint16_t itemID, uint8_t quantity) {
        GameData::fieldData[fieldID].items.push_back({ groupIdx, scriptIdx, offset, itemID, quantity });
    }

    static void addFieldScriptMateria(uint16_t fieldID, uint8_t groupIdx, uint8_t scriptIdx, uint32_t offset, uint16_t matID) {
        GameData::fieldData[fieldID].materia.push_back({ groupIdx, scriptIdx, offset, matID, 1 });
    }

    static void addFieldScriptMessage(uint16_t fieldID, uint8_t groupIdx, uint8_t scriptIdx, uint8_t windowIdx, uint32_t offset, uint32_t strOffset, uint32_t strLen) {
        GameData::fieldData[fieldID].messages.push_back({ groupIdx, scriptIdx, windowIdx, offset, strOffset, strLen });
    }

    static void addFieldScriptShop(uint16_t fieldID, uint8_t groupIdx, uint8_t scriptIdx, uint32_t offset, uint8_t shopID) {
        GameData::fieldData[fieldID].shops.push_back({ groupIdx, scriptIdx, offset, shopID });
    }

    static void addFieldWorldExit(uint16_t fieldID, uint32_t offset, uint8_t index, uint16_t targetFieldID) {
        GameData::fieldData[fieldID].worldExits.push_back({ offset, index, targetFieldID });
    }

    static void addFieldModels(uint16_t fieldID, std::vector<uint8_t> modelIDs) {
        GameData::fieldData[fieldID].modelIDs = modelIDs;
    }

    static void addWorldMapEntrance(uint32_t offset, uint16_t fieldID, const std::string& fieldName, uint32_t centerX, uint32_t centerZ) {
        GameData::worldMapEntrances.push_back({ offset, fieldID, fieldName, centerX, centerZ });
    }

    static void addShop(uint8_t shopID, std::vector<uint16_t> itemData) 
    {
        Shop& shop = GameData::shops[shopID];
        for (uint8_t i = 0; i < itemData.size(); ++i)
        {
            if (itemData[i] >= 512)
            {
                shop.materia.push_back({ i, itemData[i] - 512u });
            }
            else 
            {
                shop.items.push_back({ i, itemData[i] });
            }
        }
    }

    static void addBattleScene(uint8_t sceneID, uint16_t id0, uint16_t id1, uint16_t id2, uint8_t lvl0, uint8_t lvl1, uint8_t lvl2) {
        GameData::battleScenes[sceneID] = { sceneID, {id0, id1, id2}, {lvl0, lvl1, lvl2} };
    }

    static void addBattleFormation(uint8_t sceneID, uint16_t formationID, bool noEscape, std::array<uint16_t, 6> enemyIDs, std::array<uint16_t, 4> arenaIDs) {
        GameData::battleScenes[sceneID].formations.push_back({ formationID, noEscape, enemyIDs, arenaIDs });
    }

    static void addBoss(const std::string& name, uint16_t enemyID, std::vector<int> sceneIDs, uint64_t elemTypes, uint64_t elemRates) {
        GameData::bosses.push_back({ name, enemyID, sceneIDs, elemTypes, elemRates });
    }

    static void addModel(const std::string& modelName, int polyCount, std::vector<ModelPart> parts) {
        GameData::models.push_back({ modelName, polyCount, parts });
    }

    static void addBattleModel(const std::string& modelName, int headerSize, std::vector<BattleModelPart> parts) {
        GameData::battleModels.push_back({ modelName, headerSize, parts });
    }

    static Item* getAccessory(uint8_t id);
    static Item* getArmor(uint8_t id);
    static Item* getItem(uint8_t id);
    static Item* getWeapon(uint8_t id);
    static Item* getMateria(uint8_t id);

    static uint16_t getRandomAccessory(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomArmor(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomItem(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomWeapon(std::mt19937_64& rng, bool excludeBanned = true);
    static uint16_t getRandomMateria(std::mt19937_64& rng, bool excludeBanned = true);

    // Returns a random item ID thats the same type as origItemID
    static uint16_t getRandomItemFromID(uint16_t origItemID, std::mt19937_64& rng, bool excludeBanned = true);

    static FieldData getField(uint16_t id);
    static std::string getItemName(uint16_t fieldScriptID);
    static uint32_t getItemPrice(uint16_t fieldScriptID);
    static std::string getMateriaName(uint8_t id);
    static uint32_t getMateriaPrice(uint8_t id);

    static BattleModel* getBattleModel(std::string modelName);
    static std::vector<const Boss*> getBossesInScene(const BattleScene* scene);

    static std::string decodeString(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> encodeString(const std::string& input);
};