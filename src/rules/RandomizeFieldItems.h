#pragma once
#include "Rule.h"
#include <cstdint>

class RandomizeFieldItems : public Rule
{
public:
    void onStart() override;

private:
    void onFieldChanged(uint16_t fieldID);
    void onFrame(uint32_t frameNumber);
    void onBattleExit();

    void randomizeFieldItems(uint16_t fieldID);

    int framesUntilApply = 0;
};