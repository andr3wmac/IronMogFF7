#pragma once
#include "Rule.h"
#include "LiveModFF7/game/GameData.h"
#include <cstdint>

class RandomizeFieldItems : public Rule
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