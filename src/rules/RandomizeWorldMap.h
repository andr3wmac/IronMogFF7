#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>
#include <set>

class RandomizeWorldMap : public Rule
{
public:
    void setup() override;

private:
    int lastClosestIndex = -1;
    uint16_t lastCmd0 = 0;
    uint16_t lastCmd1 = 0;

    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);
    uint16_t getRandomEntrance(uint16_t entranceIndex);

    std::vector<std::set<uint16_t>> entranceGroups;
    std::unordered_map<int, int> randomizedEntrances;
};