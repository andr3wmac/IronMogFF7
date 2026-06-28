#pragma once

#include "LiveModFF7/game/GameManager.h"

class Restrictions
{
public:
    static void reset();

    static void banItem(uint16_t id);
    static bool isItemBanned(uint16_t itemID);
    static void banMateria(uint16_t materiaID);
    static bool isMateriaBanned(uint16_t materiaID);

    // Ban enforcement. These delete banned items/materia that a randomizer (or
    // nothing) left in place. They are intended to be bound to the matching
    // GameManager events *after* all rules are set up, so they run last and only
    // remove what wasn't already replaced.
    static void enforceBattleBans(GameManager* game);
    static void enforceFieldBans(GameManager* game, uint16_t fieldID);
    static void enforceShopBans(GameManager* game, uint8_t menuIndex);
};