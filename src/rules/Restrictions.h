#pragma once

#include "core/game/GameManager.h"

class Restrictions
{
public:
    static void reset();

    static void banAccessory(uint8_t id);
    static bool isAccessoryBanned(uint8_t id);
    static void banArmor(uint8_t id);
    static bool isArmorBanned(uint8_t id);
    static void banItem(uint8_t id);
    static bool isItemBanned(uint8_t id);
    static void banWeapon(uint8_t id);
    static bool isWeaponBanned(uint8_t id);

    static bool isFieldItemBanned(uint16_t fieldItemID);

    static void banMateria(uint8_t materiaID);
    static bool isMateriaBanned(uint8_t materiaID);
};