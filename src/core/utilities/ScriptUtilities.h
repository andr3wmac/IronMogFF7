#pragma once

#include <cstdint>
#include "core/game/GameManager.h"

class ScriptUtilities
{
public:
    static void decompileWorldScript(uint8_t* data, size_t sizeInBytes);
    static void findWorldScripts(GameManager* game, uintptr_t startAddress, uintptr_t endAddress);
};
