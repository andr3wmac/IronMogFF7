#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>
#include <set>

struct RandomizedShopItem
{
    uintptr_t offset;   // offset to where the entry is in the shop data
    uint16_t id;
    uint32_t price;
};

struct RandomizedShop
{
    std::vector<RandomizedShopItem> items;
    std::vector<RandomizedShopItem> materia;
    std::vector<RandomizedShopItem> newItems;
    std::vector<RandomizedShopItem> newMateria;
};

class RandomizeShops : public Rule
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    void onStart();
    void generateRandomizedShops();
    void onFieldChanged(uint16_t fieldID);
    void onShopOpened();
    void onFrame(uint32_t frameNumber);

    uint16_t randomizeShopItem(uint16_t itemID);
    uint16_t randomizeShopItem(uint16_t itemID, std::set<uint16_t> previouslyChosen);
    uint16_t randomizeShopMateria(uint16_t materiaID);
    uint16_t randomizeShopMateria(uint16_t materiaID, std::set<uint16_t> previouslyChosen);

    bool disableShops = false;
    bool keepPrices = true;

    std::mt19937_64 rng;
    std::unordered_map<uint8_t, RandomizedShop> randomizedShops;
    uint16_t lastFieldID = 0;
    bool shopOpen = false;
    std::set<uint8_t> fieldShopIDs;
    int shopMenuIndex = -1;

    std::array<uint32_t, 320> itemSellPrices;
    std::array<uint32_t, 91> materiaSellPrices;
};