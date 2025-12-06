#pragma once
#include "Rule.h"
#include <cstdint>
#include <set>
#include "core/utilities/Flags.h"

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
        return deadCharacters.isBitSet(characterID);
    }

private:
    std::vector<PermadeathExemption> exemptions;
    Flags<uint16_t> deadCharacters;
    std::set<uint8_t> justDiedCharacters;

    void onStart();
    void onFrame(uint32_t frameNumber);
    bool isExempt(uint16_t fieldID);
};