#include "DifficultyProgression.h"
#include "app/gui/GUI.h"
#include "AppFrame/AppFrame.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"
#include "rules/Restrictions.h"

REGISTER_RULE(DifficultyProgression, "Difficulty Progression", "Progressively scales into your randomizer settings.")

static const char* progressSource[] { "Game Progress", "Highest Level", "In-Game Time"};

static const char* progressEnds[] { "Cloud Named", "After 7th Heaven", "Exit Midgar", "End of Disc 1", "End of Disc 2", "Final Descent" };
static uint16_t progressGameMoments[]{ 7, 105, 341, 677, 1620, 1997 };

void DifficultyProgression::setup()
{
    BIND_EVENT(game->onStart, DifficultyProgression::onStart);
    BIND_EVENT_ONE_ARG(game->onGameMomentChanged, DifficultyProgression::onGameMomentChanged);
    BIND_EVENT(game->onBattleExit, DifficultyProgression::onBattleExit);
    BIND_EVENT_ONE_ARG(game->onFrame, DifficultyProgression::onFrame);
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
    if (ImGui::Combo("##DifficultyProgression_progressionSource", &progressionSourceIndex, progressSource, IM_ARRAYSIZE(progressSource)))
    {
        progressionSource = (ProgressionSource)progressionSourceIndex;
        changed = true;
    }

    ImGui::Spacing();
    ImGui::Text("Start At:");
    ImGui::SetItemTooltip("The amount of progression you start the game at.\nThis prevents always starting at lowest difficulty.");
    ImGui::SameLine(DPI(120.0f));
    ImGui::PushItemWidth(DPI(50.0f));
    if (ImGui::InputFloat("##DifficultyProgression_progressionStart", &progressionStart, 0, 0, "%.2f"))
    {
        progressionStart = Utilities::clamp(progressionStart, 0.0f, 100.0f);
        changed = true;
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Text("%%");

    // Game Progress
    if (progressionSource == ProgressionSource::GameProgress)
    {
        ImGui::Spacing();
        ImGui::Text("End Moment:");
        ImGui::SetItemTooltip("The point in the game where difficulty has fully progressed.");
        ImGui::SameLine(DPI(120.0f));
        ImGui::SetNextItemWidth(DPI(200.0f));

        changed = ImGui::Combo("##DifficultyProgression_progressionEnd", &progressionEnd, progressEnds, IM_ARRAYSIZE(progressEnds));
    }

    // Highest Level
    if (progressionSource == ProgressionSource::HighestLevel)
    {
        ImGui::Spacing();
        ImGui::Text("End Level:");
        ImGui::SetItemTooltip("When a member of your party reaches\nthis level progression is complete.");
        ImGui::SameLine(DPI(120.0f));
        ImGui::PushItemWidth(DPI(50.0f));
        if (ImGui::InputInt("##DifficultyProgression_endLevel", &progressionEndLevel, 0, 0))
        {
            progressionEndLevel = std::max(6, progressionEndLevel);
            changed = true;
        }
        ImGui::PopItemWidth();
    }

    // In-Game Time 
    if (progressionSource == ProgressionSource::InGameTime)
    {
        ImGui::Spacing();
        ImGui::Text("End Time:");
        ImGui::SetItemTooltip("The in-game time where difficulty has fully progressed.");
        ImGui::SameLine(DPI(120.0f));
        ImGui::PushItemWidth(DPI(25.0f));

        bool timeChanged = false;
        int endH, endM, endS;
        Utilities::splitTime(progressionEndTime, endH, endM, endS);

        timeChanged |= ImGui::InputInt("##DifficultyProgression_endTimeH", &endH, 0, 0);
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        timeChanged |= ImGui::InputInt("##DifficultyProgression_endTimeM", &endM, 0, 0);
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        timeChanged |= ImGui::InputInt("##DifficultyProgression_endTimeS", &endS, 0, 0);
        ImGui::PopItemWidth();

        if (timeChanged)
        {
            progressionEndTime = Utilities::toTotalSeconds(endH, endM, endS);
            changed = true;
        }
    }

    return changed;
}

void DifficultyProgression::loadSettings(const ConfigFile& cfg)
{
    progressionSource   = (ProgressionSource)cfg.get<int>("progressionSource", (int)progressionSource);
    progressionStart    = cfg.get<float>("progressionStart", progressionStart);
    progressionEnd      = cfg.get<int>("progressionEnd", progressionEnd);
    progressionEndLevel = cfg.get<int>("progressionEndLevel", progressionEndLevel);
    progressionEndTime  = cfg.get<uint32_t>("progressionEndTime", progressionEndTime);
}

void DifficultyProgression::saveSettings(ConfigFile& cfg)
{
    cfg.set<int>("progressionSource", (int)progressionSource);
    cfg.set<float>("progressionStart", progressionStart);
    cfg.set<int>("progressionEnd", progressionEnd);
    cfg.set<int>("progressionEndLevel", progressionEndLevel);
    cfg.set<uint32_t>("progressionEndTime", progressionEndTime);
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

        if (progressionSource == ProgressionSource::InGameTime)
        {
            
            progressionString += "at " + Utilities::formatTime(progressionEndTime);
        }
        
        return { progressionString };
    }

    return {};
}

void DifficultyProgression::onStart()
{
    lastMaxLevel = 0;
    lastUpdateIGT = 0;
    lastLogIGT = 0;

    updateDifficulty();
}

void DifficultyProgression::onGameMomentChanged(uint16_t gameMoment)
{
    updateDifficulty();
}

void DifficultyProgression::onBattleExit()
{
    updateDifficulty();
}

void DifficultyProgression::onFrame(int frameNumber)
{
    uint32_t currentTime = game->read<uint32_t>(GameOffsets::InGameTime);

    // Only update every 60 seconds.
    if (currentTime - lastUpdateIGT >= 60)
    {
        updateDifficulty();
        lastUpdateIGT = currentTime;
    }
}


void DifficultyProgression::updateDifficulty()
{
    if (progressionSource == ProgressionSource::GameProgress)
    {
        uint16_t gameMoment = game->getGameMoment();
        uint16_t endMoment = progressGameMoments[progressionEnd];

        float progress = Utilities::clamp((float)gameMoment / endMoment, 0.0f, 1.0f);

        float difficultyScale = Utilities::lerp(progressionStart / 100.0f, 1.0f, progress);
        difficultyScale = Utilities::clamp(difficultyScale, 0.0f, 1.0f);

        game->setDifficultyScale(difficultyScale);
        LOG("Game Moment changed to: %d, difficulty scale now: %f", gameMoment, difficultyScale);
    }

    if (progressionSource == ProgressionSource::HighestLevel)
    {
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

        const int minLevel = 6;

        if (maxLevel > lastMaxLevel)
        {
            // Calculate how far we are above the floor
            int relativeProgress = maxLevel - minLevel;
            int totalRange = progressionEndLevel - minLevel;

            // Ensure we don't divide by zero and clamp at 0 if maxLevel < 6
            float progress = 0.0f;
            if (totalRange > 0)
            {
                progress = Utilities::clamp((float)relativeProgress / totalRange, 0.0f, 1.0f);
            }

            float difficultyScale = Utilities::lerp(progressionStart / 100.0f, 1.0f, progress);
            difficultyScale = Utilities::clamp(difficultyScale, 0.0f, 1.0f);

            game->setDifficultyScale(difficultyScale);
            LOG("Max level changed to: %d, difficulty scale now: %f", maxLevel, difficultyScale);

            lastMaxLevel = maxLevel;
        }
    }

    if (progressionSource == ProgressionSource::InGameTime)
    {
        uint32_t currentTime = game->read<uint32_t>(GameOffsets::InGameTime);
        float progress = Utilities::clamp((float)currentTime / progressionEndTime, 0.0f, 1.0f);

        float difficultyScale = Utilities::lerp(progressionStart / 100.0f, 1.0f, progress);
        difficultyScale = Utilities::clamp(difficultyScale, 0.0f, 1.0f);

        game->setDifficultyScale(difficultyScale);

        // To cut down on log spam we only log every 15 minutes.
        if (lastLogIGT == 0 || currentTime - lastLogIGT > (15 * 60))
        {
            LOG("In-Game Time changed to: %u, difficulty scale now: %f", currentTime, difficultyScale);
            lastLogIGT = currentTime;
        }
    }
}
