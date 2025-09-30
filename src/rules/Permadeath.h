#pragma once
#include "Rule.h"
#include <cstdint>
#include <set>

struct PermadeathExemption
{
    uint16_t minGameMoment = 0;
    uint16_t maxGameMoment = UINT16_MAX;
    std::set<uint16_t> fieldIDs;
};

class Permadeath : public Rule
{
public:
    void setup() override;

    bool isCharacterDead(uint8_t characterID)
    {
        return deadCharacterIDs.count(characterID) > 0;
    }

private:
    std::vector<PermadeathExemption> exemptions;
    std::set<uint8_t> deadCharacterIDs;

    void onStart();
    void onFrame(uint32_t frameNumber);
    bool isExempt(uint16_t fieldID);
};