#include "RandomizeEnemyDrops.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>

REGISTER_RULE("Randomize Enemy Drops", RandomizeEnemyDrops)

void RandomizeEnemyDrops::setup()
{
    BIND_EVENT(game->onBattleEnter, RandomizeEnemyDrops::onBattleEnter);
}

void RandomizeEnemyDrops::onDebugGUI()
{
    std::pair<BattleScene*, BattleFormation*> battleData = game->getBattleFormation();
    BattleScene* scene = battleData.first;
    BattleFormation* formation = battleData.second;

    if (scene == nullptr || formation == nullptr)
    {
        ImGui::Text("Not currently in a battle.");
        return;
    }

    // Max 3 unique enemies per fight
    for (int i = 0; i < 3; ++i)
    {
        std::string enemyName = game->readString(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::Name, 32);
        std::string enemyText = std::to_string(i) + ") " + enemyName;
        ImGui::Text(enemyText.c_str());

        // Maximum of 4 item slots per enemy
        for (int j = 0; j < 4; ++j)
        {
            uint16_t dropID = game->read<uint16_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::DropIDs[j]);

            // Empty slot
            if (dropID == 65535)
            {
                std::string dropText = "  Drop " + std::to_string(j) + ": Empty.";
                ImGui::Text(dropText.c_str());
                continue;
            }

            std::string dropText = "  Drop " + std::to_string(j) + ": " + GameData::getItemNameFromID(dropID) + "(" + std::to_string(dropID) + ")";
            ImGui::Text(dropText.c_str());
        }
    }
}

void RandomizeEnemyDrops::onBattleEnter()
{
    // Seed the RNG for this particular battle formation on this field.
    // This produces predictable drops based on the game seed.
    uint16_t fieldID = game->getFieldID();
    uint16_t formationID = game->read<uint16_t>(BattleOffsets::FormationID);
    uint32_t battleIDSeed = (uint32_t(fieldID) << 16) | formationID;
    uint64_t combinedSeed = Utilities::makeSeed64(game->getSeed(), battleIDSeed);
    rng.seed(combinedSeed);

    std::pair<BattleScene*, BattleFormation*> battleData = game->getBattleFormation();
    BattleScene* scene = battleData.first;
    BattleFormation* formation = battleData.second;

    std::vector<int> activeEnemyIDs;
    for (int i = 0; i < 6; ++i)
    {
        if (formation->enemyIDs[i] == UINT16_MAX)
        {
            continue;
        }

        for (int j = 0; j < 3; ++j)
        {
            if (formation->enemyIDs[i] == scene->enemyIDs[j])
            {
                activeEnemyIDs.push_back(j);
            }
        }
    }

    for (int id : activeEnemyIDs)
    {
        // Maximum of 4 item slots per enemy
        for (int i = 0; i < 4; ++i)
        {
            uint16_t dropID = game->read<uint16_t>(BattleSceneOffsets::Enemies[id] + BattleSceneOffsets::DropIDs[i]);
            if (dropID == UINT16_MAX)
            {
                continue;
            }

            uint16_t newDropID = GameData::getRandomItemFromID(dropID, rng, true);
            game->write<uint16_t>(BattleSceneOffsets::Enemies[id] + BattleSceneOffsets::DropIDs[i], newDropID);

            std::string oldItemName = GameData::getItemNameFromID(dropID);
            std::string newItemName = GameData::getItemNameFromID(newDropID);
            LOG("Randomized enemy drop in formation %d: %s changed to %s", formationID, oldItemName.c_str(), newItemName.c_str());
        }
    }
}