#include "RandomizeEnemyDrops.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/GUI.h"
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

    changed |= ImGui::Checkbox("Randomize Every Fight", &randomizeEveryFight);
    ImGui::SetItemTooltip("Whether drops should be randomized every fight\nor randomized once for each enemy.");
    changed |= ImGui::Checkbox("Randomize Morphs", &randomizeMorphs);

    ImGui::Text("Gil Multiplier");
    ImGui::SetItemTooltip("Multiplies the gil dropped by each enemy.");
    ImGui::SameLine();

    ImGui::PushItemWidth(DPI(60.0f));
    changed |= ImGui::InputFloat("##minGilMultiplier", &minGilMultiplier, 0, 0, "%.2f");
    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();
    changed |= ImGui::InputFloat("##maxGilMultiplier", &maxGilMultiplier, 0, 0, "%.2f");
    ImGui::PopItemWidth();

    ImGui::Text("Exp Multiplier");
    ImGui::SetItemTooltip("Multiplies the exp obtained from each enemy.");
    ImGui::SameLine();

    ImGui::PushItemWidth(DPI(60.0f));
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
    randomizeEveryFight = cfg.get<bool>("randomizeEveryFight", randomizeEveryFight);
    randomizeMorphs     = cfg.get<bool>("randomizeMorphs", randomizeMorphs);
    minGilMultiplier    = cfg.get<float>("minGilMultiplier", minGilMultiplier);
    maxGilMultiplier    = cfg.get<float>("maxGilMultiplier", maxGilMultiplier);
    minExpMultiplier    = cfg.get<float>("minExpMultiplier", minExpMultiplier);
    maxExpMultiplier    = cfg.get<float>("maxExpMultiplier", maxExpMultiplier);
}

void RandomizeEnemyDrops::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("randomizeEveryFight", randomizeEveryFight);
    cfg.set<bool>("randomizeMorphs", randomizeMorphs);
    cfg.set<float>("minGilMultiplier", minGilMultiplier);
    cfg.set<float>("maxGilMultiplier", maxGilMultiplier);
    cfg.set<float>("minExpMultiplier", minExpMultiplier);
    cfg.set<float>("maxExpMultiplier", maxExpMultiplier);
}

void RandomizeEnemyDrops::onDebugGUI()
{
    const auto& [scene, formation] = game->getBattleFormation();

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

        uint16_t morphID = game->read<uint16_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::MorphItemID);
        if (morphID != 0xFFFF)
        {
            std::string itemName = GameData::getItemName(morphID);
            std::string morphText = "  Morph: " + std::to_string(morphID) + " " + itemName;
            ImGui::Text(morphText.c_str());
        }
        else 
        {
            std::string morphText = "  Morph: None";
            ImGui::Text(morphText.c_str());
        }
    }
}

std::vector<std::string> RandomizeEnemyDrops::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Randomized)
    {
        return { "Enemy Drops" };
    }

    if (descType == RuleDescripionType::Multiplier)
    {
        std::vector<std::string> results;

        if (minGilMultiplier != 1.0f || maxGilMultiplier != 1.0f)
        {
            results.push_back(Utilities::formatFloat(minGilMultiplier) + "-" + Utilities::formatFloat(maxGilMultiplier) + "x Gil");
        }
        if (minExpMultiplier != 1.0f || maxExpMultiplier != 1.0f)
        {
            results.push_back(Utilities::formatFloat(minExpMultiplier) + "-" + Utilities::formatFloat(maxExpMultiplier) + "x Exp");
        }

        return results;
    }

    return {};
}

void RandomizeEnemyDrops::onStart()
{
    rng.seed(game->getSeed());
}

void RandomizeEnemyDrops::onBattleEnter()
{
    uint16_t fieldID = game->getFieldID();
    uint16_t formationID = game->read<uint16_t>(BattleOffsets::FormationID);

    const auto& [scene, formation] = game->getBattleFormation();

    std::set<int> activeEnemyIndexes;
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
                activeEnemyIndexes.insert(j);
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

    for (int idx : activeEnemyIndexes)
    {
        if (!randomizeEveryFight)
        {
            // IF randomize every fight is disabled then we seed the RNG with the
            // enemy ID so the same game seed + enemy ID will produce the same drop
            // and morph randomization.
            uint16_t enemyID = scene->enemyIDs[idx];
            rng.seed(Utilities::makeSeed64(game->getSeed(), enemyID));
        }

        // Maximum of 4 item slots per enemy
        for (int i = 0; i < 4; ++i)
        {
            uint16_t dropID = game->read<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::DropIDs[i]);
            if (dropID == UINT16_MAX)
            {
                continue;
            }

            uint16_t newDropID = GameData::getRandomItemSameType(dropID, rng, true);
            game->write<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::DropIDs[i], newDropID);

            std::string oldItemName = GameData::getItemName(dropID);
            std::string newItemName = GameData::getItemName(newDropID);
            LOG("Randomized enemy drop in formation %d: %s changed to %s", formationID, oldItemName.c_str(), newItemName.c_str());
        }

        if (randomizeMorphs)
        {
            uint16_t morphID = game->read<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::MorphItemID);
            if (morphID != 0xFFFF)
            {
                uint16_t newMorphID = GameData::getRandomItemSameType(morphID, rng, true);
                std::string oldItemName = GameData::getItemName(morphID);
                std::string newItemName = GameData::getItemName(newMorphID);
                LOG("Randomized enemy morph in formation %d: %s changed to %s", formationID, oldItemName.c_str(), newItemName.c_str());
            }
        }
    }
}