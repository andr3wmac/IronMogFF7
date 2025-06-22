#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>

class RandomizeEnemyDrops : public Rule
{
public:
    void onStart() override;

private:
    void onFrame(uint32_t frameNumber);
    void onBattleEnter();
    void onBattleExit();

    uint16_t randomizeDropID(uint16_t dropID);

    std::mt19937_64 rng;
    int waitingFrames = 0;
};