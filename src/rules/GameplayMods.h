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

    enum class AerithMode : uint8_t
    {
        Never  = 0,  // Aerith dies as in vanilla.
        Always = 1,  // Aerith survives from the start.
        Item   = 2   // Aerith dies, but a findable custom item can revive her.
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
    void onCustomItemUsed(CustomItemUse use);
    void applyMasamuneMode();
    void applyAerithSurvives(uint16_t fieldID);

    // Aerith's survival is active in Always mode, or in Item mode once the revive item has been used.
    bool aerithActive() { return aerithMode == AerithMode::Always || (aerithMode == AerithMode::Item && aerithRevived); }
    void addAerithToPHS();

    std::mt19937_64 rng;
    MasamuneMode masamuneMode = MasamuneMode::Disabled;

    AerithMode aerithMode = AerithMode::Never;
    int aerithItemCount = 3;
    uint16_t aerithItemId = 0xFFFF;
    bool aerithRevived = false;
};