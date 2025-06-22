#include "RandomizeEnemyDrops.h"
#include "game/GameData.h"
#include "game/MemoryOffsets.h"
#include "utilities/Logging.h"

#include <random>

REGISTER_RULE("Randomize Enemy Drops", RandomizeEnemyDrops)

void RandomizeEnemyDrops::onStart()
{
    BIND_EVENT(game->onBattleEnter, RandomizeEnemyDrops::onBattleEnter);
}

uint16_t RandomizeEnemyDrops::randomizeDropID(uint16_t dropID)
{
    std::pair<uint8_t, uint16_t> data = unpackDropID(dropID);

    uint16_t selectedID = data.second;

    // Loop in case we select an unused ID.
    while (true)
    {
        if (data.first == DropType::Accessory)
        {
            std::uniform_int_distribution<int> dist(0, (int)GameData::accessoryData.size() - 1);
            int randIdx = dist(rng);
            if (GameData::accessoryData.count(randIdx) > 0)
            {
                selectedID = (uint16_t)randIdx;
                break;
            }
        }
        else if (data.first == DropType::Armor)
        {
            std::uniform_int_distribution<int> dist(0, (int)GameData::armorData.size() - 1);
            int randIdx = dist(rng);
            if (GameData::armorData.count(randIdx) > 0)
            {
                selectedID = (uint16_t)randIdx;
                break;
            }
        }
        else if (data.first == DropType::Item)
        {
            std::uniform_int_distribution<int> dist(0, (int)GameData::itemData.size() - 1);
            int randIdx = dist(rng);
            if (GameData::itemData.count(randIdx) > 0)
            {
                selectedID = (uint16_t)randIdx;
                break;
            }
        }
        else if (data.first == DropType::Weapon)
        {
            std::uniform_int_distribution<int> dist(0, (int)GameData::weaponData.size() - 1);
            int randIdx = dist(rng);
            if (GameData::weaponData.count(randIdx) > 0)
            {
                selectedID = (uint16_t)randIdx;
                break;
            }
        }
        else 
        {
            break;
        }
    }
    
    return packDropID(data.first, selectedID);
}

uint64_t makeCombinedSeed(uint32_t battleID, uint32_t seed)
{
    uint64_t combined = (uint64_t(battleID) << 32) | seed;

    // Mix bits to spread entropy
    combined ^= combined >> 33;
    combined *= 0xff51afd7ed558ccd;
    combined ^= combined >> 33;
    combined *= 0xc4ceb9fe1a85ec53;
    combined ^= combined >> 33;

    return combined;
}

void RandomizeEnemyDrops::onBattleEnter()
{
    // Seed the RNG for this particular battle formation on this field.
    // This produces predictable drops based on the game seed.
    uint16_t fieldID = game->getFieldID();
    uint16_t formationID = game->read<uint16_t>(EnemyFormationOffsets::FormationID);
    uint32_t battleIDSeed = (uint32_t(fieldID) << 16) | formationID;
    uint64_t combinedSeed = makeCombinedSeed(battleIDSeed, game->getSeed());
    rng.seed(combinedSeed);

    // Max 3 unique enemies per fight
    for (int i = 0; i < 3; ++i)
    {
        // Maximum of 4 item slots per enemy
        for (int j = 0; j < 4; ++j)
        {
            uint16_t dropID = game->read<uint16_t>(EnemyFormationOffsets::Enemies[i] + EnemyFormationOffsets::DropIDs[j]);

            // Empty slot
            if (dropID == 65535)
            {
                continue;
            }

            uint16_t newDropID = randomizeDropID(dropID);
            game->write<uint16_t>(EnemyFormationOffsets::Enemies[i] + EnemyFormationOffsets::DropIDs[j], newDropID);

            ItemData oldItemData = GameData::getItemDataFromBattleDropID(dropID);
            ItemData newItemData = GameData::getItemDataFromBattleDropID(newDropID);
            LOG("Randomized enemy drop in formation %d: %s changed to %s", formationID, oldItemData.name.c_str(), newItemData.name.c_str());
        }
    }
}