#pragma once
#include "Mod.h"
#include "LiveModFF7Core/game/GameData.h"
#include <cstdint>

class RandomizeFieldItems : public Mod
{
public:
    std::string getDescription() const override;

    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    enum class RandomMode : int
    {
        Shuffle = 0,
        Random = 1
    };

    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);

    // Shuffles items and materia between maps based on the game seed.
    void generateRandomizedItems();

    // Applies randomization to current field.
    void apply();

    RandomMode randomMode;
    bool keepItemType = true;

    // Generated randomization mapping
    std::unordered_map<uint32_t, FieldScriptItem> randomizedItems;
    std::unordered_map<uint32_t, FieldScriptItem> randomizedMateria;
};