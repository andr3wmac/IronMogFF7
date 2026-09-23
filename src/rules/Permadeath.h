#pragma once
#include "Rule.h"
#include <atomic>
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
    enum class CloudDeathMode : uint8_t
    {
        Permanent             = 0,
        ReviveAfterLifestream = 1,
        SacrificeYourFriends  = 2,
        Item                  = 3
    };

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

    // Thread-safe copy of the dead character mask for display on the GUI thread.
    uint16_t getDeadCharacterMask() const
    {
        return publishedDeadCharacters.load();
    }

private:
    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);
    void onBattleExit();
    void onCustomItemUsed(CustomItemUse use);

    void killCharacter(uint8_t id);
    void clearDeadCharacters();
    void reviveCharacter(uint8_t id);
    void loadPermadeathState();
    void savePermadeathState();
    void reviveCloudAfterLifestream(uint16_t fieldID);
    void sacrificeFriendForCloud();
    bool isExempt(uint16_t fieldID);
    std::vector<uint8_t> getLivingCharacters();
    int selectRandomLivingCharacter(uint16_t fieldID, uint8_t ignoreCharacter);
    void updateOverrideFights();
    
    bool deleteEquipped = true;
    CloudDeathMode cloudDeathMode = CloudDeathMode::Permanent;
    int cloudReviveItemCount = 3;
    uint16_t cloudReviveItemId = 0xFFFF;

    std::vector<PermadeathExemption> exemptions;
    Flags<uint16_t> deadCharacters;
    std::atomic<uint16_t> publishedDeadCharacters = 0;
    uint8_t cloudDeathCount = 0;
    std::set<uint8_t> justDiedCharacters;

    bool appliedRufusRandom = false;
    bool appliedDyneRandom = false;
    bool waitingOnBattleExit = false;
    uint16_t lastFieldTrigger = 0;
};