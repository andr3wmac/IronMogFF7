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

    // Per-eligible-pickup chance that a field item is replaced by this custom item.
    float spawnChance = 0.0f;

    // If true, only one can ever be obtained across the run (tracked via a saved "found" bit). 
    // If false, there is no hard cap and spawnChance is the only thing limiting how many appear.
    bool isUnique = true;
};

struct CustomItemUse
{
    uint16_t itemId = 0xFFFF;
    uint8_t targetCharId = 0xFF;
};
