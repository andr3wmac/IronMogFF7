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
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

    bool isCharacterDead(uint8_t characterID)
    {
        return deadCharacters.isBitSet(characterID);
    }

private:
    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);
    void onBattleExit();

    bool isExempt(uint16_t fieldID);
    std::vector<uint8_t> getLivingCharacters();
    int selectRandomLivingCharacter(uint16_t fieldID, uint8_t ignoreCharacter);
    void updateOverrideFights();
    
    std::vector<PermadeathExemption> exemptions;
    Flags<uint16_t> deadCharacters;
    std::set<uint8_t> justDiedCharacters;

    bool appliedRufusRandom = false;
    bool appliedDyneRandom = false;
    bool waitingOnBattleExit = false;
    uint16_t lastFieldTrigger = 0;
};