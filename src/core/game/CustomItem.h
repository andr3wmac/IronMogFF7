#pragma once

#include <cstdint>
#include <string>

// A custom item fabricated in one of FF7's unused item slots (ids 105-127). 
// Consumers register one via GameManager::registerCustomItem and react to its use by binding to GameManager::onCustomItemUsed.
struct CustomItem
{
    uint16_t id = 0xFFFF;
    std::string name;
    bool targetsCharacter = false;

    // How many field pickups are replaced by this custom item across the run.
    int spawnCount = 0;
};

struct CustomItemUse
{
    uint16_t itemId = 0xFFFF;
    uint8_t targetCharId = 0xFF;
};
