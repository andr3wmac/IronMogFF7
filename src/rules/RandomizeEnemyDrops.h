#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>

class RandomizeEnemyDrops : public Rule
{
public:
    void setup() override;

private:
    std::mt19937_64 rng;

    uint16_t randomizeDropID(uint16_t dropID);
    void onBattleEnter();
};