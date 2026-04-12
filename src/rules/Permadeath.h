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
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    std::vector<std::string> describe(RuleDescripionType descType) override;

    bool isCharacterDead(uint8_t characterID)
    {
        return deadCharacters.isBitSet(characterID);
    }

private:
    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);
    void onBattleExit();

    void killCharacter(uint8_t id);
    bool isExempt(uint16_t fieldID);
    std::vector<uint8_t> getLivingCharacters();
    int selectRandomLivingCharacter(uint16_t fieldID, uint8_t ignoreCharacter);
    void updateOverrideFights();
    
    bool deleteEquipped = true;

    std::vector<PermadeathExemption> exemptions;
    Flags<uint16_t> deadCharacters;
    std::set<uint8_t> justDiedCharacters;

    bool appliedRufusRandom = false;
    bool appliedDyneRandom = false;
    bool waitingOnBattleExit = false;
    uint16_t lastFieldTrigger = 0;
};