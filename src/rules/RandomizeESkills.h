#pragma once
#include "Rule.h"
#include "core/utilities/Flags.h"
#include <cstdint>

class RandomizeESkills : public Rule
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:

    struct TrackedMateria
    {
        uintptr_t offset;
        uint32_t materiaID;
    };

    struct TrackedPlayer
    {
        uint8_t index = 0;
        std::vector<TrackedMateria> trackedMateria;
        Flags<uint32_t> previousESkills;
    };

    void onStart();
    void onBattleEnter();
    void onBattleExit();
    void onFrame(uint32_t frameNumber);

    void setESkillBattleMenu(TrackedPlayer& player, int eSkillIndex, bool enabled);

    bool battleEntered = false;
    std::vector<int> eSkillMapping;
    std::vector<TrackedPlayer> trackedPlayers;
};