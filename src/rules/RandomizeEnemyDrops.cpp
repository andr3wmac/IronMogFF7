#include "RandomizeEnemyDrops.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <random>

REGISTER_RULE("Randomize Enemy Drops", RandomizeEnemyDrops)

void RandomizeEnemyDrops::setup()
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
            selectedID = GameData::getRandomAccessory(rng);
            break;
        }
        else if (data.first == DropType::Armor)
        {
            selectedID = GameData::getRandomArmor(rng);
            break;
        }
        else if (data.first == DropType::Item)
        {
            selectedID = GameData::getRandomItem(rng);
            break;
        }
        else if (data.first == DropType::Weapon)
        {
            selectedID = GameData::getRandomWeapon(rng);
            break;
        }
        else 
        {
            break;
        }
    }
    
    return packDropID(data.first, selectedID);
}

void RandomizeEnemyDrops::onBattleEnter()
{
    // Seed the RNG for this particular battle formation on this field.
    // This produces predictable drops based on the game seed.
    uint16_t fieldID = game->getFieldID();
    uint16_t formationID = game->read<uint16_t>(EnemyFormationOffsets::FormationID);
    uint32_t battleIDSeed = (uint32_t(fieldID) << 16) | formationID;
    uint64_t combinedSeed = Utilities::makeCombinedSeed(battleIDSeed, game->getSeed());
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

            std::string oldItemName = GameData::getNameFromBattleDropID(dropID);
            std::string newItemName = GameData::getNameFromBattleDropID(newDropID);
            LOG("Randomized enemy drop in formation %d: %s changed to %s", formationID, oldItemName.c_str(), newItemName.c_str());
        }
    }
}