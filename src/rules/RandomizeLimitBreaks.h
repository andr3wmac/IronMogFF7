#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>

class RandomizeLimitBreaks : public Rule
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

    void generateRandomLimitMap();
    void updateLimitStringTable();

    bool keepFirstLimit = false;
    bool unrestricted = false;

    std::vector<int> limitMap;
    std::vector<uint16_t> limitStringOffsets;
};