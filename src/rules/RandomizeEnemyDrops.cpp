#include "RandomizeEnemyDrops.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>
#include <set>

REGISTER_RULE(RandomizeEnemyDrops, "Randomize Enemy Drops", "Enemy drops and steals are randomized.")

void RandomizeEnemyDrops::setup()
{
    BIND_EVENT(game->onStart, RandomizeEnemyDrops::onStart);
    BIND_EVENT(game->onBattleEnter, RandomizeEnemyDrops::onBattleEnter);
}

bool RandomizeEnemyDrops::onSettingsGUI()
{
    bool changed = false;

    ImGui::Text("Gil Multiplier");
    ImGui::SetItemTooltip("Multiplies the gil dropped by each enemy.");
    ImGui::SameLine();

    ImGui::PushItemWidth(60);
    changed |= ImGui::InputFloat("##minGilMultiplier", &minGilMultiplier, 0, 0, "%.2f");
    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();
    changed |= ImGui::InputFloat("##maxGilMultiplier", &maxGilMultiplier, 0, 0, "%.2f");
    ImGui::PopItemWidth();

    ImGui::Text("Exp Multiplier");
    ImGui::SetItemTooltip("Multiplies the exp obtained from each enemy.");
    ImGui::SameLine();

    ImGui::PushItemWidth(60);
    changed |= ImGui::InputFloat("##minExpMultiplier", &minExpMultiplier, 0, 0, "%.2f");
    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();
    changed |= ImGui::InputFloat("##maxExpMultiplier", &maxExpMultiplier, 0, 0, "%.2f");
    ImGui::PopItemWidth();

    return changed;
}

void RandomizeEnemyDrops::loadSettings(const ConfigFile& cfg)
{
    minGilMultiplier = cfg.get<float>("minGilMultiplier", 1.0f);
    maxGilMultiplier = cfg.get<float>("maxGilMultiplier", 1.0f);
    minExpMultiplier = cfg.get<float>("minExpMultiplier", 1.0f);
    maxExpMultiplier = cfg.get<float>("maxExpMultiplier", 1.0f);
}

void RandomizeEnemyDrops::saveSettings(ConfigFile& cfg)
{
    cfg.set<float>("minGilMultiplier", minGilMultiplier);
    cfg.set<float>("maxGilMultiplier", maxGilMultiplier);
    cfg.set<float>("minExpMultiplier", minExpMultiplier);
    cfg.set<float>("maxExpMultiplier", maxExpMultiplier);
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

            std::string dropText = "  Drop " + std::to_string(j) + ": " + GameData::getItemName(dropID) + "(" + std::to_string(dropID) + ")";
            ImGui::Text(dropText.c_str());
        }
    }
}

void RandomizeEnemyDrops::onStart()
{
    rng.seed(game->getSeed());
}

void RandomizeEnemyDrops::onBattleEnter()
{
    uint16_t fieldID = game->getFieldID();
    uint16_t formationID = game->read<uint16_t>(BattleOffsets::FormationID);

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

        std::uniform_real_distribution<float> gilDist(minGilMultiplier, maxGilMultiplier);
        float gilMultiplier = gilDist(rng);

        std::uniform_real_distribution<float> expDist(minExpMultiplier, maxExpMultiplier);
        float expMultiplier = expDist(rng);

        // Gil and EXP Multipliers
        uint32_t gil = game->read<uint32_t>(BattleOffsets::Enemies[i] + BattleOffsets::Gil);
        uint32_t exp = game->read<uint32_t>(BattleOffsets::Enemies[i] + BattleOffsets::Exp);
        uint32_t newGil = Utilities::clampTo<uint32_t>(gil * gilMultiplier);
        uint32_t newExp = Utilities::clampTo<uint32_t>(exp * expMultiplier);
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

            uint16_t newDropID = GameData::getRandomItemSameType(dropID, rng, true);
            game->write<uint16_t>(BattleSceneOffsets::Enemies[id] + BattleSceneOffsets::DropIDs[i], newDropID);

            std::string oldItemName = GameData::getItemName(dropID);
            std::string newItemName = GameData::getItemName(newDropID);
            LOG("Randomized enemy drop in formation %d: %s changed to %s", formationID, oldItemName.c_str(), newItemName.c_str());
        }
    }
}