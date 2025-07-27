#include "Restrictions.h"
#include "core/game/MemoryOffsets.h"
#include <set>

std::set<uint8_t> bannedMateria;

void Restrictions::reset()
{
    bannedMateria.clear();
}

void Restrictions::banMateria(uint8_t materiaID)
{
    bannedMateria.insert(materiaID);
}

bool Restrictions::isMateriaBanned(uint8_t materiaID)
{
    return (bannedMateria.count(materiaID) > 0);
}