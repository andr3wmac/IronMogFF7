#include "RandomizeEnemyDrops.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>
#include <set>

REGISTER_RULE("Randomize Enemy Drops", RandomizeEnemyDrops)

void RandomizeEnemyDrops::setup()
{
    BIND_EVENT(game->onBattleEnter, RandomizeEnemyDrops::onBattleEnter);
}

bool RandomizeEnemyDrops::onSettingsGUI()
{
    bool changed = false;

    ImGui::Text("Gil Multiplier");
    ImGui::SetItemTooltip("Multiplies the gil dropped by each enemy.");
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(50.0f);
    changed |= ImGui::InputFloat("##gilMultiplier", &gilMultiplier, 0.0f, 0.0f, "%.2f");

    ImGui::Text("Exp Multiplier");
    ImGui::SetItemTooltip("Multiplies the exp obtained from each enemy.");
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(50.0f);
    changed |= ImGui::InputFloat("##expMultiplier", &expMultiplier, 0.0f, 0.0f, "%.2f");

    return changed;
}

void RandomizeEnemyDrops::loadSettings(const ConfigFile& cfg)
{
    gilMultiplier = cfg.get<float>("gilMultiplier", 1.0f);
    expMultiplier = cfg.get<float>("expMultiplier", 1.0f);
}

void RandomizeEnemyDrops::saveSettings(ConfigFile& cfg)
{
    cfg.set<float>("gilMultiplier", gilMultiplier);
    cfg.set<float>("expMultiplier", expMultiplier);
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

    std::set<int> activeEnemyIDs;
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
                activeEnemyIDs.insert(j);
            }
        }

        // Gil and EXP Multipliers
        uint32_t gil = game->read<uint32_t>(BattleOffsets::Enemies[i] + BattleOffsets::Gil);
        uint32_t exp = game->read<uint32_t>(BattleOffsets::Enemies[i] + BattleOffsets::Exp);
        uint32_t newGil = (uint32_t)std::clamp(gil * gilMultiplier, 0.0f, FLT_MAX);
        uint32_t newExp = (uint32_t)std::clamp(exp * expMultiplier, 0.0f, FLT_MAX);
        game->write<uint32_t>(BattleOffsets::Enemies[i] + BattleOffsets::Gil, newGil);
        game->write<uint32_t>(BattleOffsets::Enemies[i] + BattleOffsets::Exp, newExp);
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