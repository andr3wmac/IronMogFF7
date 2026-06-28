#include "Randomizer.h"
#include "LiveModFF7/utilities/Logging.h"

namespace
{
    std::function<bool(uint16_t)> gItemBanFilter;
    std::function<bool(uint16_t)> gMateriaBanFilter;
}

void Randomizer::setItemBanFilter(const std::function<bool(uint16_t)>& filter)
{
    gItemBanFilter = filter;
}

void Randomizer::setMateriaBanFilter(const std::function<bool(uint16_t)>& filter)
{
    gMateriaBanFilter = filter;
}

std::vector<uint16_t> Randomizer::getItemsOfType(ItemType type, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> items;
    items.reserve(GameData::items.size());

    for (const auto& [id, data] : GameData::items)
    {
        if (type == ItemType::Normal && id >= 128)
        {
            continue;
        }
        if (type == ItemType::Weapon && (id < 128 || id >= 256))
        {
            continue;
        }
        if (type == ItemType::Armor && (id < 256 || id >= 288))
        {
            continue;
        }
        if (type == ItemType::Accessory && id < 288)
        {
            continue;
        }

        if ((excludeBanned && gItemBanFilter && gItemBanFilter(id)) ||
            (excludeRare && data.price == 2) ||
            (excludeSet.count(id) > 0))
        {
            continue;
        }

        items.push_back(id);
    }

    return items;
}

uint16_t Randomizer::getRandomItem(uint16_t origItemID, std::mt19937_64& rng, bool keepType, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;

    if (keepType)
    {
        /*
          Item ID Conversion:
            0   + X = Items
            128 + X = Weapons
            256 + X = Armor
            288 + X = Accessories
        */

        if (origItemID < 128)
        {
            candidates = getItemsOfType(ItemType::Normal, excludeBanned, excludeRare, excludeSet);
        }
        else if (origItemID < 256)
        {
            candidates = getItemsOfType(ItemType::Weapon, excludeBanned, excludeRare, excludeSet);
        }
        else if (origItemID < 288)
        {
            candidates = getItemsOfType(ItemType::Armor, excludeBanned, excludeRare, excludeSet);
        }
        else
        {
            candidates = getItemsOfType(ItemType::Accessory, excludeBanned, excludeRare, excludeSet);
        }
    }
    else
    {
        // Collect items from all the categories
        ItemType types[] = { ItemType::Normal, ItemType::Weapon, ItemType::Armor, ItemType::Accessory };
        for (ItemType type : types)
        {
            std::vector<uint16_t> result = getItemsOfType(type, excludeBanned, excludeRare, excludeSet);
            candidates.insert(candidates.end(), result.begin(), result.end());
        }
    }

    // If there are no candidate items then return the original
    if (candidates.empty())
    {
        return origItemID;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}

uint16_t Randomizer::getRandomMateria(std::mt19937_64& rng, bool excludeBanned, bool excludeRare, const std::set<uint16_t>& excludeSet)
{
    std::vector<uint16_t> candidates;
    candidates.reserve(GameData::materia.size());

    for (const auto& [id, data] : GameData::materia)
    {
        if ((excludeBanned && gMateriaBanFilter && gMateriaBanFilter(id)) ||
            (excludeRare && data.price == 1) ||
            (excludeSet.count(id) > 0))
        {
            continue;
        }

        candidates.push_back(id);
    }

    if (candidates.empty())
    {
        LOG("No materia selected from getRandomMateria.");
        return UINT16_MAX;
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}
