#pragma once

#include <cstdint>
#include "livemod/game/GameManager.h"

class ScriptUtilities
{
public:
    static void decompileWorldScript(GameManager* game, uintptr_t startAddress, size_t sizeInBytes);
    static void findWorldScripts(GameManager* game, uintptr_t startAddress, uintptr_t endAddress);
};
