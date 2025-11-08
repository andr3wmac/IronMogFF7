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
    generateRandomEncounterMap();
}

void RandomizeEncounters::onFrame(uint32_t frameNumber)
{
    uint16_t formationID = game->read<uint16_t>(0x707BC);
    if (formationID == lastFormation)
    {
        return;
    }
    lastFormation = formationID;

    if (randomEncounterMap.count(formationID) > 0)
    {
        uint16_t randomFormation = randomEncounterMap[formationID];
        game->write<uint16_t>(0x707BC, randomFormation);
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
    std::mt19937 rng(game->getSeed());

    for (const auto& kv : GameData::battleScenes)
    {
        BattleScene scene = kv.second;

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

            // Select a random formation from the candidates
            std::uniform_int_distribution<std::size_t> dist(0, candidateFormationIDs.size() - 1);
            randomEncounterMap[formation.id] = candidateFormationIDs[dist(rng)];
        }
    }
}