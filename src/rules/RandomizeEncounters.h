#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>

class RandomizeEncounters : public Rule
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    void onSettingsGUI() override;

private:
    void onStart();
    void onFrame(uint32_t frameNumber);
    void onBattleExit();
    void generateRandomEncounterMap();

    int maxLevelDifference = 5;
    uint16_t lastFormation = 0;
    std::unordered_map<uint16_t, std::vector<uint16_t>> randomEncounterMap;
    std::mt19937 rng;
};