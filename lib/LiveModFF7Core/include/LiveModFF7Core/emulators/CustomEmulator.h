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
        // Make sure the user supplied address actually points at FF7's PS1 memory.
        if (!verifyPS1MemoryOffset(customMemoryAddress))
        {
            return false;
        }

        ps1BaseAddress = customMemoryAddress;
        return true;
    }

protected:
    uintptr_t customMemoryAddress = 0;
};