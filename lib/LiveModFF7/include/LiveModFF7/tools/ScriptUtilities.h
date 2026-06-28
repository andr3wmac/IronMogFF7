#pragma once

#include "LiveModFF7/game/GameManager.h"
#include <cstdint>

class ScriptUtilities
{
public:
    static void decompileWorldScript(GameManager* game, uintptr_t startAddress, size_t sizeInBytes);
    static void findWorldScripts(GameManager* game, uintptr_t startAddress, uintptr_t endAddress);
};
