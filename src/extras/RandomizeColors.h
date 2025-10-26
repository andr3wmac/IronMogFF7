#pragma once
#include "extras/Extra.h"
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

    std::vector<Utilities::Color> randomColors;

    bool waitingForField = false;
    int lastFieldID = -1;
    int lastFieldFade = 0;

    // Debug variables
    char debugStartNum[20];
    char debugCount[20];
    std::vector<uintptr_t> debugAddresses;
};