#pragma once

#include "core/game/GameManager.h"

class Restrictions
{
public:
    static void reset();

    static void banItem(uint16_t id);
    static bool isItemBanned(uint16_t itemID);
    static void banMateria(uint16_t materiaID);
    static bool isMateriaBanned(uint16_t materiaID);
};