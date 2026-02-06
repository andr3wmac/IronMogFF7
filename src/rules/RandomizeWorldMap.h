#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>
#include <set>

class RandomizeWorldMap : public Rule
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    void onStart();
    void onFrame(uint32_t frameNumber);
    void onWorldMapEnter();
    void onFieldChanged(uint16_t fieldID);
    uint16_t getRandomEntrance(uint16_t entranceIndex);

    int lastClosestIndex = -1;
    uint16_t lastCmd0 = 0;
    uint16_t lastCmd1 = 0;
    uint16_t lastGameMoment = 0;
    uint32_t lastLoggedSeed = 0;

    std::vector<std::set<uint16_t>> entranceGroups;
    std::unordered_map<int, int> randomizedEntrances;
};