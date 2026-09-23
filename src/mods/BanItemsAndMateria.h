#pragma once
#include "Mod.h"
#include <cstdint>

class BanItemsAndMateria : public Mod
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    bool noItems = false;
    bool noWeapons = false;
    bool noArmor = false;
    bool noAccessories = false;

    bool noSummon = true;
    bool noMagic = false;
    bool noCommand = false;
    bool noSupport = false;
    bool noIndependent = false;
    bool noEnemySkill = false;
    bool noMaster = false;
};