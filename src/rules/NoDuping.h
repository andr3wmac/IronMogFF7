#pragma once
#include "Rule.h"
#include <cstdint>
#include <deque>

class NoDuping : public Rule
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    void onStart();
    void onBattleEnter();
    void onFrame(uint32_t frameNumber);

    void checkPartyMembers();

    std::array<bool, 3> wItemPartyMembers;
    std::deque<std::pair<uint16_t, uint8_t>> wItemCache;
    bool cancelWasPressed = false;
    uint8_t lastTargetTrigger = 0xFF;
    int lastActivePlayer = -1;
};