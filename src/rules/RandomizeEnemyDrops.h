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

private:
    std::mt19937_64 rng;
    float gilMultiplier = 1.0f;
    float expMultiplier = 1.0f;

    void onBattleEnter();
};