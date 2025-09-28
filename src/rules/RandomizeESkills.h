#pragma once
#include "Rule.h"
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
        bool menuLoaded = false;
        uint64_t previousESkillValues[24];
    };

    void onStart();
    void onBattleEnter();
    void onBattleExit();
    void onFrame(uint32_t frameNumber);

    bool isESkillMenuLoaded(TrackedPlayer& player);
    void setESkillBattleMenu(TrackedPlayer& player, int eSkillIndex, bool enabled);

    bool battleEntered = false;
    std::vector<int> eSkillMapping;
    std::vector<TrackedPlayer> trackedPlayers;
};