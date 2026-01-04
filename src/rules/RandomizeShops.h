#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>
#include <set>

class RandomizeShops : public Rule
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    void onSettingsGUI() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    void onFieldChanged(uint16_t fieldID);
    void onShopOpened();

    uint16_t randomizeShopItem(uint16_t itemID);
    uint16_t randomizeShopItem(uint16_t itemID, std::set<uint16_t> previouslyChosen);
    uint16_t randomizeShopMateria(uint16_t materiaID);
    uint16_t randomizeShopMateria(uint16_t materiaID, std::set<uint16_t> previouslyChosen);

    std::mt19937_64 rng;
    uint16_t lastFieldID = 0;
    bool disableShops = false;
    bool keepPrices = true;
};