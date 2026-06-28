#pragma once

#include "LiveModFF7/game/GameData.h"

#include <cstdint>
#include <functional>
#include <random>
#include <set>
#include <vector>

// Random item/materia selection helpers shared by randomizer rules and extras. This lives in
// utilities (rather than rules) so both rules and extras can use it. It draws from the engine's
// GameData tables, but the engine itself has no concept of randomization or bans.
namespace Randomizer
{
    // Host-supplied ban predicates. When set, the excludeBanned argument below consults these to
    // drop banned ids from the candidate pool. They are injected (rather than calling a concrete
    // ban registry directly) so this utility stays free of any rules/ dependency. Return true to
    // exclude the given id.
    void setItemBanFilter(const std::function<bool(uint16_t)>& filter);
    void setMateriaBanFilter(const std::function<bool(uint16_t)>& filter);

    std::vector<uint16_t> getItemsOfType(ItemType type, bool excludeBanned = true, bool excludeRare = false, const std::set<uint16_t>& excludeSet = {});
    uint16_t getRandomItem(uint16_t origItemID, std::mt19937_64& rng, bool keepType, bool excludeBanned = true, bool excludeRare = false, const std::set<uint16_t>& excludeSet = {});
    uint16_t getRandomMateria(std::mt19937_64& rng, bool excludeBanned = true, bool excludeRare = false, const std::set<uint16_t>& excludeSet = {});
}
