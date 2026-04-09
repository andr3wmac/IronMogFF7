#include "DifficultyProgression.h"
#include "core/game/MemoryOffsets.h"
#include "rules/Restrictions.h"
#include "core/utilities/Logging.h"

#include <imgui.h>
#include "core/gui/GUI.h"

REGISTER_RULE(DifficultyProgression, "Difficulty Progression", "Progressively scales into your randomizer settings.")

static const char* progressSource[] { "Game Progress", "Highest Level" };

static const char* progressEnds[] { "After 7th Heaven", "Exit Midgar", "End of Disc 1", "End of Disc 2", "Final Descent" };
static uint16_t progressGameMoments[]{ 105, 341, 677, 1620, 1997 };

void DifficultyProgression::setup()
{
    BIND_EVENT_ONE_ARG(game->onGameMomentChanged, DifficultyProgression::onGameMomentChanged);
    BIND_EVENT(game->onBattleExit, DifficultyProgression::onBattleExit);
}

bool DifficultyProgression::onSettingsGUI()
{
    bool changed = false;

    ImGui::Spacing();
    ImGui::Text("Source:");
    ImGui::SetItemTooltip("How progress is determined.\nGame Progress: as you advance the story difficulty increases.\nHighest Level: difficulty increases with max level of your party.");
    ImGui::SameLine(DPI(120.0f));
    ImGui::SetNextItemWidth(DPI(200.0f));

    int progressionSourceIndex = (int)progressionSource;
    if (ImGui::Combo("##DifficultyProfession_progressionSource", &progressionSourceIndex, progressSource, IM_ARRAYSIZE(progressSource)))
    {
        progressionSource = (ProgressionSource)progressionSourceIndex;
        changed = true;
    }

    // Game Progress
    if (progressionSource == ProgressionSource::GameProgress)
    {
        ImGui::Spacing();
        ImGui::Text("End Moment:");
        ImGui::SetItemTooltip("Sets the point in the game where difficulty has fully progressed.");
        ImGui::SameLine(DPI(120.0f));
        ImGui::SetNextItemWidth(DPI(200.0f));

        changed = ImGui::Combo("##DifficultyProfession_progressionEnd", &progressionEnd, progressEnds, IM_ARRAYSIZE(progressEnds));
    }

    // Highest Level
    if (progressionSource == ProgressionSource::HighestLevel)
    {
        ImGui::Spacing();
        ImGui::Text("End Level:");
        ImGui::SetItemTooltip("When a member of your party reaches\nthis level progression is complete.");
        ImGui::SameLine(DPI(120.0f));
        ImGui::PushItemWidth(DPI(50.0f));
        if (ImGui::InputInt("##DifficultyProfession_endLevel", &progressionEndLevel, 0, 0))
        {
            progressionEndLevel = std::max(0, progressionEndLevel);
            changed = true;
        }
        ImGui::PopItemWidth();
    }

    return changed;
}

void DifficultyProgression::loadSettings(const ConfigFile& cfg)
{
    progressionSource   = (ProgressionSource)cfg.get<int>("progressionSource", (int)progressionSource);
    progressionEnd      = cfg.get<int>("progressionEnd", progressionEnd);
    progressionEndLevel = cfg.get<int>("progressionEndLevel", progressionEndLevel);
}

void DifficultyProgression::saveSettings(ConfigFile& cfg)
{
    cfg.set<int>("progressionSource", (int)progressionSource);
    cfg.set<int>("progressionEnd", progressionEnd);
    cfg.set<int>("progressionEndLevel", progressionEndLevel);
}

std::vector<std::string> DifficultyProgression::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Unique)
    {
        std::string progressionString = "Difficulty Progression until ";

        if (progressionSource == ProgressionSource::GameProgress)
        {
            progressionString += progressEnds[progressionEnd];
        }
        
        if (progressionSource == ProgressionSource::HighestLevel)
        {
            progressionString += "level " + std::to_string(progressionEndLevel);
        }
        
        return { progressionString };
    }

    return {};
}

void DifficultyProgression::onGameMomentChanged(uint16_t gameMoment)
{
    if (progressionSource != ProgressionSource::GameProgress)
    {
        return;
    }

    uint16_t endMoment = progressGameMoments[progressionEnd];

    float difficultyScale = (float)gameMoment / endMoment;
    if (difficultyScale > 1.0f)
    {
        difficultyScale = 1.0f;
    }

    game->setDifficultyScale(difficultyScale);
    LOG("Game Moment changed to: %d, difficulty scale now: %f", gameMoment, difficultyScale);
}

void DifficultyProgression::onBattleExit()
{
    if (progressionSource != ProgressionSource::HighestLevel)
    {
        return;
    }

    uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);
    int maxLevel = 0;

    for (int i = 0; i < 9; ++i)
    {
        uint8_t characterID = CharacterDataOffsets::CharacterIDs[i];

        // Note: Cloud is always included
        if (i == 0 || Utilities::isBitSet(phsVisMask, i))
        {
            uint8_t level = game->read<uint8_t>(CharacterDataOffsets::Characters[i] + CharacterDataOffsets::Level);
            maxLevel = std::max(maxLevel, (int)level);
        }
    }

    if (maxLevel > 0 && maxLevel != lastMaxLevel)
    {
        float difficultyScale = (float)maxLevel / progressionEndLevel;
        game->setDifficultyScale(difficultyScale);
        LOG("Max level changed to: %d, difficulty scale now: %f", maxLevel, difficultyScale);
        lastMaxLevel = maxLevel;
    }
}