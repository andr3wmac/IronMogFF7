#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>

class RandomizeEnemyDrops : public Rule
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

private:
    void onStart();
    void onBattleEnter();

    std::mt19937_64 rng;
    bool randomizeEveryFight = false;
    bool randomizeMorphs = true;
    bool keepItemType = true;

    float minAPMultiplier = 1.0f;
    float maxAPMultiplier = 1.0f;
    float minExpMultiplier = 1.0f;
    float maxExpMultiplier = 1.0f;
    float minGilMultiplier = 1.0f;
    float maxGilMultiplier = 1.0f;
};