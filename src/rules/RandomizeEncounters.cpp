#include "RandomizeEncounters.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/GUI.h"
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

    // Debug Room fights
    addExclusions({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 22, 23, 24, 25, 26, 952, 953, 954, 955, 957, 958, 959, 989, 990, 991, 996, 997, 998, 999 });

    // Chocobo fights
    addExclusions({ 56, 57, 60, 61, 78, 79, 80, 81, 98, 99, 104, 105, 152, 153, 156, 157, 162, 163, 166, 167, 202, 203, 206, 207, 214, 215, 218, 219 });

    // Yuffie
    addExclusions({ 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 296, 297, 298 });

    // Midgar Zolom
    addExclusions({ 469, 470 });

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
                        excludedFormations.set(formation.id);
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

    changed |= ImGui::Checkbox("Match Battle Types", &matchBattleTypes);
    ImGui::SetItemTooltip("Will only randomize Back Attacks to other Back Attacks, etc");

    ImGui::Text("Levels Below");
    ImGui::SetItemTooltip("How many levels below the original encounter's maximum\nlevel the randomized encounter can be.");
    ImGui::SameLine(DPI(125.0f));
    ImGui::PushItemWidth(DPI(50.0f));
    if (ImGui::InputInt("##encLevelsBelow", &levelsBelow, 0, 0))
    {
        levelsBelow = std::max(0, levelsBelow);
        changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::Text("Levels Above");
    ImGui::SetItemTooltip("How many levels above the original encounter's maximum\nlevel the randomized encounter can be.");
    ImGui::SameLine(DPI(125.0f));
    ImGui::PushItemWidth(DPI(50.0f));
    if (ImGui::InputInt("##encLevelsAbove", &levelsAbove, 0, 0))
    {
        levelsAbove = std::max(0, levelsAbove);
        changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::Text("Stat Multiplier");
    ImGui::SetItemTooltip("Multiplies each enemy's HP, MP, Strength, Magic, Evade,\nSpeed, Luck, Defense, and MDefense.\nMultiplier is randomly chosen for each stat for each enemy.");
    ImGui::SameLine();

    ImGui::PushItemWidth(DPI(60.0f));
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
    randomEncounters   = cfg.get<bool>("randomEncounters", randomEncounters);
    scriptedEncounters = cfg.get<bool>("scriptedEncounters", scriptedEncounters);
    worldMapEncounters = cfg.get<bool>("worldMapEncounters", worldMapEncounters);
    matchBattleTypes   = cfg.get<bool>("matchBattleTypes", matchBattleTypes);
    levelsBelow        = cfg.get<int>("levelsBelow", levelsBelow);
    levelsAbove        = cfg.get<int>("levelsAbove", levelsAbove);
    minStatMultiplier  = cfg.get<float>("minStatMultiplier", minStatMultiplier);
    maxStatMultiplier  = cfg.get<float>("maxStatMultiplier", maxStatMultiplier);
}

void RandomizeEncounters::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("randomEncounters",   randomEncounters);
    cfg.set<bool>("scriptedEncounters", scriptedEncounters);
    cfg.set<bool>("worldMapEncounters", worldMapEncounters);
    cfg.set<bool>("matchBattleTypes",   matchBattleTypes);
    cfg.set<int>("levelsBelow",         levelsBelow);
    cfg.set<int>("levelsAbove",         levelsAbove);
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

std::vector<std::string> RandomizeEncounters::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Randomized)
    {
        return { "Encounters" };
    }

    if (descType == RuleDescripionType::Multiplier)
    {
        if (minStatMultiplier != 1.0f || maxStatMultiplier != 1.0f)
        {
            return { Utilities::formatFloat(minStatMultiplier) + "-" + Utilities::formatFloat(maxStatMultiplier) + "x Enemy Stats" };
        }
    }

    return {};
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

    const auto& [scene, formation] = game->getBattleFormation();
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

uint8_t getMaxLevelInFormation(const BattleScene& scene, const BattleFormation& formation)
{
    uint8_t maxLevel = 0;

    for (int i = 0; i < 6; ++i)
    {
        uint16_t enemyID = formation.enemyIDs[i];
        if (enemyID == 0xFFFF)
        {
            continue;
        }

        for (int j = 0; j < 3; ++j)
        {
            if (scene.enemyIDs[j] == enemyID)
            {
                if (scene.enemyLevels[j] == 0xFF)
                {
                    continue;
                }

                maxLevel = std::max(maxLevel, scene.enemyLevels[j]);
            }
        }
    }

    return maxLevel;
}

void RandomizeEncounters::addExclusions(std::initializer_list<uint16_t> ids)
{
    for (uint16_t id : ids) 
    {
        excludedFormations.set(id);
    }
}

std::vector<uint16_t> RandomizeEncounters::findCandidates(int maxLevel, int battleType)
{
    std::vector<uint16_t> candidates;

    for (const auto& [candidateSceneID, candidateScene] : GameData::battleScenes)
    {
        // Check each formation in this scene.
        for (int j = 0; j < 4; ++j)
        {
            BattleFormation candidateFormation = candidateScene.formations[j];

            // Skip excluded formations
            if (excludedFormations.test(candidateFormation.id))
            {
                continue;
            }

            // We don't want a formation that triggers multiple fights sequentially
            if (candidateFormation.hasNextFormation())
            {
                continue;
            }

            // Skip formation if it exceeds the allowable range
            int candidateMaxLevel = getMaxLevelInFormation(candidateScene, candidateFormation);
            if (candidateMaxLevel < maxLevel - levelsBelow || candidateMaxLevel > (maxLevel + levelsAbove))
            {
                continue;
            }

            // Skip non-matching battle types if enabled
            if (matchBattleTypes && candidateFormation.battleType != battleType)
            {
                continue;
            }

            candidates.push_back(candidateFormation.id);
        }
    }

    return candidates;
}

void RandomizeEncounters::generateRandomEncounterMap()
{
    randomEncounterMap.clear();

    for (const auto& kv : GameData::battleScenes)
    {
        BattleScene scene = kv.second;

        for (int i = 0; i < 4; ++i)
        {
            BattleFormation formation = scene.formations[i];

            // Don't randomize excluded formations
            if (excludedFormations.test(formation.id))
            {
                continue;
            }

            uint8_t maxLevel = getMaxLevelInFormation(scene, formation);
            randomEncounterMap[formation.id] = findCandidates(maxLevel, formation.battleType);
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
        enemySet.maxHP      = enemySet.currentHP;
        enemySet.currentMP  = dist(rng);
        enemySet.maxMP      = enemySet.currentMP;
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