#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>
#include <random>
#include <unordered_map>

class RandomizeCharacters : public Rule
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
    void onNameEntryOpened(std::string name);

    std::mt19937_64 rng;
    bool randomizeNames = true;
    std::unordered_map<std::string, std::string> nameMap;
};