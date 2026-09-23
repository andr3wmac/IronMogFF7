#pragma once

#include "Emulator.h"

class DuckStation : public Emulator
{
    bool resolveMemory() override;
};