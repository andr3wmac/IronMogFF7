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

private:
    void onStart();
    void onFieldChanged(uint16_t fieldID);
    void onFrame(uint32_t frameNumber);

    ModelEditor modelEditor;
    std::unordered_map<std::string, std::vector<Utilities::Color>> randomModelColors;

    bool waitingForField = false;
    int lastFieldID = -1;
    int lastFieldFade = 0;

    // Debug variables
    char debugStartNum[20];
    char debugCount[20];
    std::vector<uintptr_t> debugAddresses;
};