#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>

class RandomizeEnemyDrops : public Rule
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    std::mt19937_64 rng;

    void onBattleEnter();
};