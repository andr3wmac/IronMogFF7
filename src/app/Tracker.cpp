#include "Tracker.h"
#include "livemod/game/MemoryOffsets.h"
#include "livemod/utilities/Utilities.h"
#include "app/RuleManager.h"
#include "extras/RandomizeMusic.h"
#include "rules/Permadeath.h"

Tracker::Tracker()
{
    reset();
}

void Tracker::setup(GameManager* game)
{ 
    reset();
    this->game = game;
    
    BIND_EVENT(game->onNewGame, Tracker::onNewGame);
    BIND_EVENT(game->onGameOver, Tracker::onGameOver);
}

void Tracker::reset()
{
    game = nullptr;

    for (int i = 0; i < 9; ++i)
    {
        characters[i].isActive = false;
        characters[i].isPermadead = false;
    }

    inGameTime = "Not connected.";
    currentSong = "";
    rulesSummary = "";
}

void Tracker::update()
{
    if (game == nullptr)
    {
        return;
    }

    // Permadeath Character Portraits
    {
        Permadeath* permadeathRule = (Permadeath*)RuleManager::getRule("Permadeath");
        uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);

        for (int i = 0; i < 9; ++i)
        {
            uint8_t characterID = CharacterDataOffsets::CharacterIDs[i];

            characters[i].isActive = Utilities::isBitSet(phsVisMask, i);
            characters[i].isPermadead = false;

            if (permadeathRule != nullptr)
            {
                if (permadeathRule->isCharacterDead(characterID))
                {
                    characters[i].isPermadead = true;
                }
            }
        }
    }

    // In-Game Time
    uint32_t igt = game->read<uint32_t>(GameOffsets::InGameTime);
    inGameTime = Utilities::formatTime(igt);

    // Current Song
    if (RuleManager::isExtraEnabled("Randomize Music"))
    {
        RandomizeMusic* musicRando = (RandomizeMusic*)RuleManager::getExtra("Randomize Music");
        if (musicRando->isPlaying())
        {
            currentSong = musicRando->getCurrentlyPlaying();
        }
        else
        {
            currentSong = "";
        }
    }

    // Rules summary
    rulesSummary = RuleManager::getSettingsSummary();
}

bool Tracker::showAttempts()
{
    if (attemptsDisplayMode == AttemptsDisplayMode::Automatic)
    {
        if (game == nullptr)
        {
            return true;
        }

        // If No Saving is on then we show attempts.
        return RuleManager::isRuleEnabled("No Saving");
    }
    else if (attemptsDisplayMode == AttemptsDisplayMode::Attempts)
    {
        return true;
    }

    return false;
}

bool Tracker::showGameOvers()
{
    if (attemptsDisplayMode == AttemptsDisplayMode::Automatic)
    {
        if (game == nullptr)
        {
            return false;
        }

        // If No Saving is on then we show attempts.
        return !RuleManager::isRuleEnabled("No Saving");
    }
    else if (attemptsDisplayMode == AttemptsDisplayMode::GameOvers)
    {
        return true;
    }

    return false;
}

void Tracker::onNewGame()
{
    attemptCounter++;
}

void Tracker::onGameOver()
{
    gameOverCounter++;
}