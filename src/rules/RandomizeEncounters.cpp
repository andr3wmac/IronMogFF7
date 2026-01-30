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
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeEncounters::onFieldChanged);
    BIND_EVENT(game->onBattleEnter, RandomizeEncounters::onBattleEnter);

    // Chocobo fights
    excludedFormations.insert({ 56, 57, 60, 61, 78, 79, 80, 81, 98, 99, 104, 105, 152, 153, 156, 157, 162, 163, 166, 167, 202, 203, 206, 207, 214, 215, 218, 219 });

    // Yuffie
    excludedFormations.insert({ 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 296, 297, 298 });

    // Midgar Zolom
    excludedFormations.insert({ 469, 470 });

    // Turks
    excludedFormations.insert({ 841, 842, 843 });

    // Ruby Weapon
    excludedFormations.insert({ 982, 983 });

    // Emerald Weapon
    excludedFormations.insert({ 984, 985, 986, 987 });
}

bool RandomizeEncounters::onSettingsGUI()
{
    bool changed = false;

    ImGui::Text("Max Level Difference");
    ImGui::SetItemTooltip("How much higher or lower the max level of the random\nformation can be from the original formation.");
    ImGui::SameLine(180.0f);
    ImGui::SetNextItemWidth(80.0f);
    changed |= ImGui::InputInt("##maxLevelDifference", &maxLevelDifference);

    ImGui::Text("Stat Multiplier");
    ImGui::SetItemTooltip("Multiplies each enemy's HP, MP, Strength, Magic,\nEvade, Speed, Luck, Defense, and MDefense.");
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(50.0f);
    changed |= ImGui::InputFloat("##encounterStatMultiplier", &statMultiplier, 0.0f, 0.0f, "%.2f");

    return changed;
}

void RandomizeEncounters::loadSettings(const ConfigFile& cfg)
{
    maxLevelDifference = cfg.get<int>("maxLevelDifference", 5);
    statMultiplier = cfg.get<float>("statMultiplier", 1.0f);
}

void RandomizeEncounters::saveSettings(ConfigFile& cfg)
{
    cfg.set<int>("maxLevelDifference", maxLevelDifference);
    cfg.set<float>("statMultiplier", statMultiplier);
}

void RandomizeEncounters::onDebugGUI()
{
    if (game->getGameModule() != GameModule::Field)
    {
        ImGui::Text("Not currently in field.");
        return;
    }

    // Display field encounter table randomization
    {
        FieldData fieldData = GameData::getField(game->getFieldID());
        if (!fieldData.isValid())
        {
            ImGui::Text("Invalid field.");
            return;
        }

        for (int t = 0; t < 2; ++t)
        {
            uintptr_t tableOffset = FieldScriptOffsets::EncounterStart + fieldData.encounterOffset + (t * FieldScriptOffsets::EncounterTableStride);

            uint8_t tableEnabled = game->read<uint8_t>(tableOffset);
            if (tableEnabled == 1)
            {
                uint16_t dbgEncTable[10];
                game->read(tableOffset + 2, sizeof(uint16_t) * 10, (uint8_t*)dbgEncTable);

                std::string encTableText = "Encounter Table " + std::to_string(t);
                ImGui::Text(encTableText.c_str());

                for (int i = 0; i < 10; ++i)
                {
                    uint8_t  prob = dbgEncTable[i] >> 10;
                    uint16_t encounterID = dbgEncTable[i] & 0x03FF;

                    if (encounterID == 0 && prob == 0)
                    {
                        continue;
                    }

                    auto [origProb, origEncounterID] = fieldData.getEncounter(t, i);

                    std::string encText = std::to_string(i) + ") " + std::to_string(origEncounterID) + " to " + std::to_string(encounterID);
                    ImGui::Text(encText.c_str());
                }
            }
        }
    }
}

void RandomizeEncounters::onStart()
{
    rng.seed(game->getSeed());
    generateRandomEncounterMap();
}

void RandomizeEncounters::onFieldChanged(uint16_t fieldID)
{
    updateEncounterTable(fieldID);
}

void RandomizeEncounters::updateEncounterTable(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    // Two encounter tables per field
    for (int t = 0; t < 2; ++t)
    {
        uintptr_t tableOffset = FieldScriptOffsets::EncounterStart + fieldData.encounterOffset + (t * FieldScriptOffsets::EncounterTableStride);

        for (int i = 0; i < 10; ++i)
        {
            auto [origProb, origEncounterID] = fieldData.getEncounter(t, i);
            if (origProb == 0 && origEncounterID == 0)
            {
                continue;
            }

            std::vector<uint16_t> candidates = randomEncounterMap[origEncounterID];
            if (candidates.size() == 0)
            {
                LOG("No random encounter candidates for formation %d", origEncounterID);
                continue;
            }

            std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
            uint16_t randomEncounterID = candidates[dist(rng)];
            uint16_t newEncounter = (origProb << 10) | (randomEncounterID & 0x03FF);

            game->write<uint16_t>(tableOffset + 2 + (sizeof(uint16_t) * i), newEncounter);
            LOG("Randomized battle: %d to %d (Candidates: %d, Table: %d)", origEncounterID, randomEncounterID, candidates.size(), t);
        }
    }
}

void RandomizeEncounters::onBattleEnter()
{
    if (statMultiplier == 1.0f)
    {
        return;
    }

    std::pair<BattleScene*, BattleFormation*> battleData = game->getBattleFormation();
    BattleFormation* formation = battleData.second;
    if (formation == nullptr)
    {
        return;
    }

    if (randomEncounterMap.count(formation->id) == 0)
    {
        return;
    }

    for (int i = 0; i < 6; ++i)
    {
        if (formation->enemyIDs[i] == UINT16_MAX)
        {
            continue;
        }

        game->applyBattleStatMultiplier(BattleOffsets::Enemies[i], statMultiplier);
    }
}

void RandomizeEncounters::generateRandomEncounterMap()
{
    for (const auto& kv : GameData::battleScenes)
    {
        BattleScene scene = kv.second;

        // Determine max enemy level in the scene
        uint8_t maxLevel = 0;
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

            if (formation.isArenaBattle())
            {
                // For now skip randomizing these.
                continue;
            }

            // Don't randomize excluded formations
            if (excludedFormations.count(formation.id) > 0)
            {
                continue;
            }

            std::vector<uint16_t> candidateFormationIDs;
            for (const auto& candidateKv : GameData::battleScenes)
            {
                BattleScene candidateScene = candidateKv.second;

                // Determine max enemy level in the candidate scene
                uint8_t candidateMaxLevel = 0;
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

                    if (candidateFormation.isArenaBattle())
                    {
                        continue;
                    }

                    // Skip excluded formations
                    if (excludedFormations.count(candidateFormation.id) > 0)
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