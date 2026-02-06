#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>
#include <set>

class RandomizeEncounters : public Rule
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
    void onFieldChanged(uint16_t fieldID);
    void onWorldMapEnter();
    void onBattleEnter();

    void generateRandomEncounterMap();
    void generateEnemyStatMultipliers();

    bool randomEncounters = true;
    bool scriptedEncounters = true;
    bool worldMapEncounters = true;
    int maxLevelDifference = 5;
    float minStatMultiplier = 1.0f;
    float maxStatMultiplier = 1.0f;

    std::set<uint16_t> excludedFormations;
    std::unordered_map<uint16_t, std::vector<uint16_t>> randomEncounterMap;
    std::unordered_map<uint16_t, StatMultiplierSet> enemyStatMultipliers;
    std::mt19937 rng;
};