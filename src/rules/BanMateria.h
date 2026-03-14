#pragma once
#include "Rule.h"
#include <cstdint>

class BanMateria : public Rule
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    std::vector<std::string> describe(RuleDescripionType descType) override;

private:
    bool noSummons = true;
    bool noMagic = false;
    bool noCommand = false;
    bool noSupport = false;
    bool noIndependent = false;

    bool noESkill = false;
};