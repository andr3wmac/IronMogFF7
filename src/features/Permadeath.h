#pragma once
#include "Feature.h"
#include <cstdint>
#include <set>

class Permadeath : public Feature
{
public:
    void onEnable() override;
    void onDisable() override;

private:
    void onFrame(uint32_t frameNumber);

    std::set<uint8_t> deadCharacterIDs;
};