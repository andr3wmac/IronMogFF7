#include "Tracker.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Utilities.h"
#include "app/ModManager.h"
#include "mods/RandomizeMusic.h"
#include "mods/Permadeath.h"

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
    modsSummary = "";
}

void Tracker::update()
{
    if (game == nullptr)
    {
        return;
    }

    // Permadeath Character Portraits
    {
        Permadeath* permadeathMod = (Permadeath*)ModManager::getMod("Permadeath");
        uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);

        for (uint8_t i = 0; i < 9; ++i)
        {
            characters[i].isActive = Utilities::isBitSet(phsVisMask, i);
            characters[i].isPermadead = false;

            if (permadeathMod != nullptr)
            {
                if (permadeathMod->isCharacterDead(i))
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
    if (RandomizeMusic* musicRando = (RandomizeMusic*)ModManager::getMod("Randomize Music"))
    {
        if (musicRando->isPlaying())
        {
            currentSong = musicRando->getCurrentlyPlaying();
        }
        else
        {
            currentSong = "";
        }
    }

    // Mod summary
    modsSummary = ModManager::getSettingsSummary();
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
        return ModManager::isModEnabled("No Saving");
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
        return !ModManager::isModEnabled("No Saving");
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
