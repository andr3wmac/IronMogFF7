#pragma once
#include "Mod.h"
#include <cstdint>

class GameplayMods : public Mod
{
public:
    std::string getDescription() const override;

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
    void onFieldChanged(uint16_t fieldID);
    void onFrame(uint32_t frameNumber);
    void applyMasamuneMode();
    void applyAerithSurvives(uint16_t fieldID);

    std::mt19937_64 rng;
    MasamuneMode masamuneMode = MasamuneMode::Disabled;
    bool aerithSurvives = false;
};