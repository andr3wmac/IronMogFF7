#pragma once
#include "Mod.h"
#include <cstdint>
#include <deque>

class NoDuping : public Mod
{
public:
    std::string getDescription() const override;

    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    void onStart();
    void onBattleEnter();
    void onFrame(uint32_t frameNumber);

    void checkPartyMembers();
    void checkWItemDuping();
    void checkFieldItemDuping();

    std::array<bool, 3> wItemPartyMembers;
    std::deque<std::pair<uint16_t, uint8_t>> wItemCache;
    bool cancelWasPressed = false;
    uint8_t lastTargetTrigger = 0xFF;
    int lastActivePlayer = -1;
};