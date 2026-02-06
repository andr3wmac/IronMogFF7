#pragma once
#include "extras/Extra.h"
#include "core/utilities/ModelEditor.h"
#include "core/utilities/Utilities.h"
#include <cstdint>
#include <unordered_map>

class RandomizeColors : public Extra
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;

private:
    void onStart();
    void onFieldChanged(uint16_t fieldID);
    void onBattleEnter();
    void onWorldMapEnter();
    void onFrame(uint32_t frameNumber);
    void applyColors();

    ModelEditor modelEditor;
    std::unordered_map<std::string, std::vector<Utilities::Color>> randomModelColors;
    int rerollOffset = 0;

    bool waitingForBattle = false;
    bool appliedHackFix = false;

    // Debug variables
    char debugStartNum[20];
    char debugCount[20];
    std::vector<uintptr_t> debugAddresses;
};