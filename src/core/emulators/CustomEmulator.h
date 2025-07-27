#pragma once

#include "Emulator.h"

class CustomEmulator : public Emulator
{
public:
    CustomEmulator(uintptr_t memoryAddress)
        : customMemoryAddress(memoryAddress)
    {

    }

    uintptr_t getPS1MemoryOffset() override
    {
        return customMemoryAddress;
    }

protected:
    uintptr_t customMemoryAddress = 0;
};