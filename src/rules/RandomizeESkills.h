#pragma once
#include "Rule.h"
#include <cstdint>

class RandomizeESkills : public Rule
{
public:
    void setup() override;

private:
    struct TrackedMateria
    {
        uintptr_t offset;
        uint32_t materiaID;
    };

    void onStart();
    void onBattleEnter();
    void onBattleExit();

    std::vector<int> eSkillMapping;
    std::vector<TrackedMateria> trackedMateria;
};