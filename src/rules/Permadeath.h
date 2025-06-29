#pragma once
#include "Rule.h"
#include <cstdint>
#include <set>

class Permadeath : public Rule
{
public:
    void setup() override;

private:
    void onFrame(uint32_t frameNumber);

    std::set<uint8_t> deadCharacterIDs;
};