#pragma once
#include "Rule.h"
#include <cstdint>

class RandomizeEnemyDrops : public Rule
{
public:
    void onStart() override;

private:
    void onFrame(uint32_t frameNumber);
    void onBattleEnter();
    void onBattleExit();

    int framesInBattle = 0;
};