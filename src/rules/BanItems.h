#pragma once
#include "Rule.h"
#include <cstdint>

class BanItems : public Rule
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    std::vector<std::string> describe(RuleDescripionType descType) override;

private:
    bool noConsumables = false;
    bool noWeapons = false;
    bool noArmor = false;
    bool noAccessories = false;
};