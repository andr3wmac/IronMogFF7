#pragma once

#include "Emulator.h"

class BizHawk : public Emulator
{
public:
    uintptr_t getPS1MemoryOffset() override;

private:
    uintptr_t findPS1MemoryOffset(uintptr_t searchStartAddr, uintptr_t searchEndAddr);
};