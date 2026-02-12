#include "Restrictions.h"
#include "core/game/MemoryOffsets.h"
#include <set>

std::set<uint16_t> bannedItems;
std::set<uint16_t> bannedMateria;

void Restrictions::reset()
{
    bannedItems.clear(); 
    bannedMateria.clear();
}

void Restrictions::banItem(uint16_t id)
{
    bannedItems.insert(id);
}

bool Restrictions::isItemBanned(uint16_t itemID)
{
    return (bannedItems.count(itemID) > 0);
}

void Restrictions::banMateria(uint16_t materiaID)
{
    bannedMateria.insert(materiaID);
}

bool Restrictions::isMateriaBanned(uint16_t materiaID)
{
    return (bannedMateria.count(materiaID) > 0);
}