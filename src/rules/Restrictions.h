#pragma once

#include "core/game/GameManager.h"

class Restrictions
{
public:
    static void reset();

    static void banMateria(uint8_t materiaID);
    static bool isMateriaBanned(uint8_t materiaID);
};