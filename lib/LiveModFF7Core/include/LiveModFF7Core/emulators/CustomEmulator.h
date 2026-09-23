#pragma once

#include "Emulator.h"

class CustomEmulator : public Emulator
{
public:
    CustomEmulator(uintptr_t memoryAddress)
        : customMemoryAddress(memoryAddress)
    {

    }

    bool resolveMemory() override
    {
        ps1BaseAddress = customMemoryAddress;
        return true;
    }

protected:
    uintptr_t customMemoryAddress = 0;
};