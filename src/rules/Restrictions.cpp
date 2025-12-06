#include "Restrictions.h"
#include "core/game/MemoryOffsets.h"
#include <set>

std::set<uint8_t> bannedAccessories;
std::set<uint8_t> bannedArmor;
std::set<uint8_t> bannedItems;
std::set<uint8_t> bannedWeapons;
std::set<uint8_t> bannedMateria;

void Restrictions::reset()
{
    bannedAccessories.clear();
    bannedArmor.clear();
    bannedItems.clear(); 
    bannedWeapons.clear();
    bannedMateria.clear();
}

void Restrictions::banAccessory(uint8_t id)
{
    bannedAccessories.insert(id);
}
bool Restrictions::isAccessoryBanned(uint8_t id)
{
    return (bannedAccessories.count(id) > 0);
}
void Restrictions::banArmor(uint8_t id)
{
    bannedArmor.insert(id);
}
bool Restrictions::isArmorBanned(uint8_t id)
{
    return (bannedArmor.count(id) > 0);
}
void Restrictions::banItem(uint8_t id)
{
    bannedItems.insert(id);
}
bool Restrictions::isItemBanned(uint8_t id)
{
    return (bannedItems.count(id) > 0);
}
void Restrictions::banWeapon(uint8_t id)
{
    bannedWeapons.insert(id);
}
bool Restrictions::isWeaponBanned(uint8_t id)
{
    return (bannedWeapons.count(id) > 0);
}

bool Restrictions::isFieldItemBanned(uint16_t fieldItemID)
{
    if (fieldItemID < 128)
    {
        return isItemBanned((uint8_t)fieldItemID);
    }
    else if (fieldItemID < 256)
    {
        return isWeaponBanned((uint8_t)(fieldItemID - 128));
    }
    else if (fieldItemID < 288)
    {
        return isArmorBanned((uint8_t)(fieldItemID - 256));
    }
    else
    {
        return isAccessoryBanned((uint8_t)(fieldItemID - 288));
    }

    return false;
}

void Restrictions::banMateria(uint8_t materiaID)
{
    bannedMateria.insert(materiaID);
}

bool Restrictions::isMateriaBanned(uint8_t materiaID)
{
    return (bannedMateria.count(materiaID) > 0);
}