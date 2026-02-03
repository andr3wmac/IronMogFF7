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

REGISTER_RULE(RandomizeEncounters, "Randomize Encounters", "Field, world map, and/or scripted encounters are randomized to any enemy formation within set specifications.")

void RandomizeEncounters::setup()
{
    BIND_EVENT(game->onStart, RandomizeEncounters::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeEncounters::onFieldChanged);
    BIND_EVENT(game->onWorldMapEnter, RandomizeEncounters::onWorldMapEnter);
    BIND_EVENT(game->onBattleEnter, RandomizeEncounters::onBattleEnter);

    // Chocobo fights
    excludedFormations.insert({ 56, 57, 60, 61, 78, 79, 80, 81, 98, 99, 104, 105, 152, 153, 156, 157, 162, 163, 166, 167, 202, 203, 206, 207, 214, 215, 218, 219 });

    // Yuffie
    excludedFormations.insert({ 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 296, 297, 298 });

    // Midgar Zolom
    excludedFormations.insert({ 469, 470 });

    // Add all boss formations to excluded formations
    {
        std::set<uint16_t> bossIDs;
        for (const Boss& boss : GameData::bosses)
        {
            bossIDs.insert(boss.id);
        }

        for (auto& [id, scene] : GameData::battleScenes)
        {
            for (BattleFormation& formation : scene.formations)
            {
                for (int i = 0; i < 6; ++i)
                {
                    if (bossIDs.count(formation.enemyIDs[i]) > 0)
                    {
                        excludedFormations.insert(formation.id);
                        break;
                    }
                }
            }
        }
    }
}

bool RandomizeEncounters::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Random Encounters", &randomEncounters);
    changed |= ImGui::Checkbox("Scripted Encounters", &scriptedEncounters);
    ImGui::SetItemTooltip("Randomize fights triggered from scripts excluding boss fights.");
    changed |= ImGui::Checkbox("World Map Encounters", &worldMapEncounters);

    ImGui::Text("Max Level Difference");
    ImGui::SetItemTooltip("How much higher or lower the max level of the random\nformation can be from the original formation.");
    ImGui::SameLine(180.0f);
    ImGui::SetNextItemWidth(80.0f);
    changed |= ImGui::InputInt("##maxLevelDifference", &maxLevelDifference);

    ImGui::Text("Stat Multiplier");
    ImGui::SetItemTooltip("Multiplies each enemy's HP, MP, Strength, Magic, Evade,\nSpeed, Luck, Defense, and MDefense.\nMultiplier is randomly chosen for each stat for each enemy.");
    ImGui::SameLine();

    ImGui::PushItemWidth(60);
    changed |= ImGui::InputFloat("##encMinStatMultiplier", &minStatMultiplier, 0, 0, "%.2f");
    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();
    changed |= ImGui::InputFloat("##encMaxStatMultiplier", &maxStatMultiplier, 0, 0, "%.2f");
    ImGui::PopItemWidth();

    return changed;
}

void RandomizeEncounters::loadSettings(const ConfigFile& cfg)
{
    randomEncounters   = cfg.get<bool>("randomEncounters", true);
    scriptedEncounters = cfg.get<bool>("scriptedEncounters", true);
    worldMapEncounters = cfg.get<bool>("worldMapEncounters", true);
    maxLevelDifference = cfg.get<int>("maxLevelDifference", 5);
    minStatMultiplier  = cfg.get<float>("minStatMultiplier", 1.0f);
    maxStatMultiplier  = cfg.get<float>("maxStatMultiplier", 1.0f);
}

void RandomizeEncounters::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("randomEncounters",   randomEncounters);
    cfg.set<bool>("scriptedEncounters", scriptedEncounters);
    cfg.set<bool>("worldMapEncounters", worldMapEncounters);
    cfg.set<int>("maxLevelDifference",  maxLevelDifference);
    cfg.set<float>("minStatMultiplier", minStatMultiplier);
    cfg.set<float>("maxStatMultiplier", maxStatMultiplier);
}

void RandomizeEncounters::onDebugGUI()
{
    if (game->getGameModule() == GameModule::Field)
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
                Encounter dbgEncTable[10];
                game->read(tableOffset + 2, sizeof(uint16_t) * 10, (uint8_t*)dbgEncTable);

                std::string encTableText = "Encounter Table " + std::to_string(t);
                ImGui::Text(encTableText.c_str());

                for (int i = 0; i < 10; ++i)
                {
                    Encounter& origEnc = fieldData.getEncounter(t, i);
                    Encounter& enc = dbgEncTable[i];

                    std::string encText = std::to_string(i) + ") " + std::to_string(origEnc.id) + " to " + std::to_string(enc.id);
                    ImGui::Text(encText.c_str());
                }
            }
        }
    }

    if (game->getGameModule() == GameModule::World)
    {
        for (int r = 0; r < 16; ++r)
        {
            WorldMapEncounters& origEncounters = GameData::worldMapEncounters[r];

            std::string regionText = "World Region " + std::to_string(r);
            if (ImGui::CollapsingHeader(regionText.c_str()))
            {
                for (int s = 0; s < 4; ++s)
                {
                    std::vector<Encounter>& origEncSet = origEncounters.sets[s];

                    std::string setText = "Set " + std::to_string(s);
                    ImGui::Text(setText.c_str());
                    uintptr_t tableOffset = WorldOffsets::EncounterStart + (r * 128) + (s * 32);

                    uint8_t setEnabled = game->read<uint8_t>(tableOffset);
                    if (setEnabled == 1)
                    {
                        for (int i = 0; i < 14; ++i)
                        {
                            Encounter& origEnc = origEncSet[i];
                            Encounter enc = game->read<Encounter>(tableOffset + 2 + (i * 2));

                            std::string encText = " " + std::to_string(i) + ") " + std::to_string(origEnc.id) + " to " + std::to_string(enc.id);
                            ImGui::Text(encText.c_str());
                        }
                    }
                }
            }
        }
    }
}

void RandomizeEncounters::onStart()
{
    rng.seed(game->getSeed());
    generateRandomEncounterMap();
    generateEnemyStatMultipliers();
}

void RandomizeEncounters::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    if (randomEncounters)
    {
        // Two encounter tables per field
        for (int t = 0; t < 2; ++t)
        {
            uintptr_t tableOffset = FieldScriptOffsets::EncounterStart + fieldData.encounterOffset + (t * FieldScriptOffsets::EncounterTableStride);

            for (int i = 0; i < 10; ++i)
            {
                Encounter& origEncounter = fieldData.getEncounter(t, i);
                if (origEncounter.prob == 0 && origEncounter.id == 0)
                {
                    continue;
                }

                std::vector<uint16_t> candidates = randomEncounterMap[origEncounter.id];
                if (candidates.size() == 0)
                {
                    LOG("No random encounter candidates for formation %d", origEncounter.id);
                    continue;
                }

                std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
                uint16_t randomEncounterID = candidates[dist(rng)];
                uint16_t newEncounter = (origEncounter.prob << 10) | (randomEncounterID & 0x03FF);

                game->write<uint16_t>(tableOffset + 2 + (sizeof(uint16_t) * i), newEncounter);
                LOG("Randomized battle: %d to %d (Candidates: %d, Table: %d)", origEncounter.id, randomEncounterID, candidates.size(), t);
            }
        }
    }

    if (scriptedEncounters)
    {
        for (FieldScriptBattle& battle : fieldData.battles)
        {
            std::vector<uint16_t> candidates = randomEncounterMap[battle.formationID];
            if (candidates.size() == 0)
            {
                LOG("No random encounter candidates for formation %d", battle.formationID);
                continue;
            }

            std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
            uint16_t randomFormationID = candidates[dist(rng)];

            uintptr_t battleIDOffset = FieldScriptOffsets::ScriptStart + battle.offset + 2;
            game->write<uint16_t>(battleIDOffset, randomFormationID);
        }
    }
}

void RandomizeEncounters::onWorldMapEnter()
{
    if (!worldMapEncounters)
    {
        return;
    }

    for (int r = 0; r < 16; ++r)
    {
        WorldMapEncounters& encounters = GameData::worldMapEncounters[r];

        for (int s = 0; s < 4; ++s)
        {
            std::vector<Encounter>& encSet = encounters.sets[s];
            if (encSet.size() == 0)
            {
                continue;
            }

            uintptr_t tableOffset = WorldOffsets::EncounterStart + (r * 128) + (s * 32) + 2;

            // There are 14 in a set but the last 4 are chocobos and we don't randomize those fights.
            for (int i = 0; i < 10; ++i)
            {
                Encounter& encData = encSet[i];
                if (encData.raw == 0)
                {
                    continue;
                }

                std::vector<uint16_t> candidates = randomEncounterMap[encData.id];
                if (candidates.size() == 0)
                {
                    LOG("No random encounter candidates for formation %d", encData.id);
                    continue;
                }

                std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
                uint16_t randomEncounterID = candidates[dist(rng)];

                Encounter randEnc;
                randEnc.prob = encData.prob;
                randEnc.id = randomEncounterID;

                game->write<Encounter>(tableOffset + (i * 2), randEnc);
            }
        }
    }
}

void RandomizeEncounters::onBattleEnter()
{
    if (minStatMultiplier == 1.0f && maxStatMultiplier == 1.0f)
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

        if (enemyStatMultipliers.count(formation->enemyIDs[i]) == 0)
        {
            continue;
        }

        game->applyBattleStatMultiplier(BattleOffsets::Enemies[i], enemyStatMultipliers[formation->enemyIDs[i]]);
    }
}

void RandomizeEncounters::generateRandomEncounterMap()
{
    randomEncounterMap.clear();

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

void RandomizeEncounters::generateEnemyStatMultipliers()
{
    enemyStatMultipliers.clear();

    // Get list of all enemies
    std::set<uint16_t> enemyIDs;
    for (auto& [id, scene] : GameData::battleScenes)
    {
        for (BattleFormation& formation : scene.formations)
        {
            for (uint16_t enemyID : formation.enemyIDs)
            {
                if (enemyID != 0xFFFF)
                {
                    enemyIDs.insert(enemyID);
                }
            }
        }
    }

    std::uniform_real_distribution<float> dist(minStatMultiplier, maxStatMultiplier);

    for (uint16_t enemyID : enemyIDs)
    {
        StatMultiplierSet enemySet;

        enemySet.currentHP  = dist(rng);
        enemySet.maxHP      = dist(rng);
        enemySet.currentMP  = dist(rng);
        enemySet.maxMP      = dist(rng);
        enemySet.strength   = dist(rng);
        enemySet.magic      = dist(rng);
        enemySet.evade      = dist(rng);
        enemySet.speed      = dist(rng);
        enemySet.luck       = dist(rng);
        enemySet.defense    = dist(rng);
        enemySet.mDefense   = dist(rng);

        enemyStatMultipliers[enemyID] = enemySet;
    }
}