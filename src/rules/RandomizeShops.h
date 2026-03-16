#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>
#include <set>

struct RandomizedShopItem
{
    uintptr_t offset; // offset to where the entry is in the shop data
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
    std::vector<std::string> describe(RuleDescripionType descType) override;

    bool areShopsDisabled() { return disableShops; }

private:
    void onStart();
    void generateRandomizedShops();
    void onFieldChanged(uint16_t fieldID);
    void onShopOpened();
    void onShopMenuChanged(uint8_t menuIdx);

    uint16_t randomizeShopItem(uint16_t itemID, const std::set<uint16_t>& previouslyChosen);
    uint16_t randomizeShopMateria(uint16_t materiaID, const std::set<uint16_t>& previouslyChosen);

    bool disableShops = false;
    bool keepShopPrices = true;
    bool keepItemType = true;
    bool excludeRareItems = true;
    bool excludeSources = true;

    float minPriceMultiplier = 1.0f;
    float maxPriceMultiplier = 1.0f;

    std::mt19937_64 rng;
    std::unordered_map<uint8_t, RandomizedShop> randomizedShops;
    uint16_t lastFieldID = 0;
    std::set<uint8_t> fieldShopIDs;

    std::array<uint32_t, 320> itemBuyPrices;
    std::array<uint32_t, 320> itemSellPrices;
    std::array<uint32_t, 91> materiaBuyPrices;
    std::array<uint32_t, 91> materiaSellPrices;
};