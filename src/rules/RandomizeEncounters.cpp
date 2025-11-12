#include "RandomizeEncounters.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "rules/Restrictions.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <random>

REGISTER_RULE("Randomize Encounters", RandomizeEncounters)

void RandomizeEncounters::setup()
{
    BIND_EVENT(game->onStart, RandomizeEncounters::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeEncounters::onFrame);
    BIND_EVENT(game->onBattleExit, RandomizeEncounters::onBattleExit);
}

void RandomizeEncounters::onSettingsGUI()
{
    ImGui::PushItemWidth(100);
    ImGui::InputInt("Max Level Difference", &maxLevelDifference);
    ImGui::PopItemWidth();
}

void RandomizeEncounters::onStart()
{
    rng.seed(game->getSeed());
    generateRandomEncounterMap();
}

void RandomizeEncounters::onFrame(uint32_t frameNumber)
{
    uint16_t formationID = game->read<uint16_t>(GameOffsets::NextFormationID);
    if (formationID == lastFormation)
    {
        return;
    }
    lastFormation = formationID;

    if (randomEncounterMap.count(formationID) > 0)
    {
        std::vector<uint16_t> candidates = randomEncounterMap[formationID];

        // Originally tried choosing the battle once in generateRandomEncounterMap but it didn't feel
        // random enough so choosing from the candidates on each encounter is done instead.
        std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
        uint16_t randomFormation = candidates[dist(rng)];
        game->write<uint16_t>(GameOffsets::NextFormationID, randomFormation);
        lastFormation = randomFormation;

        LOG("Randomized battle: %d to %d", formationID, randomFormation);
    }
}

void RandomizeEncounters::onBattleExit()
{
    lastFormation = 0;
}

void RandomizeEncounters::generateRandomEncounterMap()
{
    for (const auto& kv : GameData::battleScenes)
    {
        BattleScene scene = kv.second;

        // Determine max enemy level in the scene
        int maxLevel = 0;
        for (int i = 0; i < 3; ++i)
        {
            if (scene.enemyLevels[i] == 255)
            {
                continue;
            }
            maxLevel = std::max(maxLevel, scene.enemyLevels[i]);
        }

        for (int i = 0; i < 4; ++i)
        {
            BattleFormation formation = scene.formations[i];

            // We don't randomize battles with no escape flag, they're probably important.
            if (formation.noEscape)
            {
                continue;
            }

            // Don't randomize midgar zolom
            if (formation.id == 469 || formation.id == 470)
            {
                continue;
            }

            std::vector<uint16_t> candidateFormationIDs;
            for (const auto& candidateKv : GameData::battleScenes)
            {
                BattleScene candidateScene = candidateKv.second;

                // Determine max enemy level in the candidate scene
                int candidateMaxLevel = 0;
                for (int j = 0; j < 3; ++j)
                {
                    if (candidateScene.enemyLevels[j] == 255)
                    {
                        continue;
                    }
                    candidateMaxLevel = std::max(candidateMaxLevel, candidateScene.enemyLevels[j]);
                }

                // Skip the entire scene if max level exceeds max difference
                if (std::abs(maxLevel - candidateMaxLevel) > maxLevelDifference)
                {
                    continue;
                }

                // Check each formation in this scene, avoiding no escapes.
                for (int j = 0; j < 4; ++j)
                {
                    BattleFormation candidateFormation = candidateScene.formations[j];
                    if (candidateFormation.noEscape)
                    {
                        continue;
                    }

                    candidateFormationIDs.push_back(candidateFormation.id);
                }
            }

            randomEncounterMap[formation.id] = candidateFormationIDs;
        }
    }
}