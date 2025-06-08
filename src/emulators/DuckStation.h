#pragma once

#include "Emulator.h"

class DuckStation : public Emulator
{
    uintptr_t getPS1MemoryOffset() override;
};