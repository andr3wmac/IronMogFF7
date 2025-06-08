#pragma once
#include "Feature.h"
#include <cstdint>

class RandomizeEnemyDrops : public Feature
{
public:
    void onEnable() override;
    void onDisable() override;

private:
    void onFrame(uint32_t frameNumber);
    void onBattleEnter();
    void onBattleExit();

    int framesInBattle = 0;
};