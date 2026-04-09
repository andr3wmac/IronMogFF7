#pragma once
#include "Rule.h"
#include <cstdint>

class GameplayMods : public Rule
{
public:
    enum class MasamuneMode : uint8_t
    {
        Disabled        = 0,
        Cloud           = 1,
        Everyone        = 2,
        RandomCharacter = 3
    };

    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;

private:
    void onStart();
    void applyMasamuneMode();

    std::mt19937_64 rng;
    MasamuneMode masamuneMode = MasamuneMode::Disabled;
};