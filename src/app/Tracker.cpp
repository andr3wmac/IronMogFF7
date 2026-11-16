#include "Tracker.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Utilities.h"
#include "app/ModManager.h"
#include "mods/RandomizeMusic.h"
#include "mods/Permadeath.h"

#include <algorithm>

static constexpr uint16_t NeverByMoment = 0xFFFF;

// The game moment by which each character has been on screen in the story. Yuffie and Vincent are
// optional, so they're revealed by meeting them (see updateSightings) or joining.
static constexpr uint16_t RevealMoments[9] = {
    1,             // Cloud: the first battle (revealed earlier by the opening's jumps off the train).
    1,             // Barret: likewise.
    69,            // Tifa: named at 7th Heaven (also revealed on entering the bar).
    36,            // Aerith: selling flowers on arrival in Sector 8.
    259,           // Red XIII: Hojo's specimen on floor 67.
    NeverByMoment, // Yuffie
    442,           // Cait Sith: joins at the Gold Saucer.
    NeverByMoment, // Vincent
    538,           // Cid: met by the Tiny Bronco.
};

static constexpr uint16_t SeventhHeavenField = 154; // mds7pb_1
static constexpr uint16_t CoffinRoomField = 303;    // sininb2, Vincent's coffin in the Shinra Mansion basement
static constexpr uint16_t MysteryNinjaFormations[2] = {268, 279}; // Yuffie's forest encounters, all levels

// The opening (field 116, md1stin) is all game moment 0. Its director script (group 0) requests Barret's
// jump off the train, then Cloud's. CSR's disc 1 drops the FMV from this script, which moves them, so they
// are found in the loaded script rather than by address.
static constexpr uint16_t OpeningField = 116;
static constexpr uint16_t DirectorScriptStart = 0x388; // Both versions; the script ends before 0x4D0.
static constexpr uint16_t DirectorScriptSize = 0x148;

struct OpeningCue
{
    uint8_t character;
    uint8_t code[6];
};

static constexpr OpeningCue OpeningCues[] = {
    {CharacterID::Barret, {0x01, 0x02, 0x43, 0x24, 0x50, 0x00}}, // req ba 3; wait 80
    {CharacterID::Cloud, {0x01, 0x01, 0x43, 0x24, 0x02, 0x00}},  // req cl 3; wait 2
};

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
    BIND_EVENT_ONE_ARG(game->onFieldChanged, Tracker::onFieldChanged);
}

void Tracker::reset()
{
    game = nullptr;

    for (int i = 0; i < 9; ++i)
    {
        characters[i].isActive = false;
        characters[i].isPermadead = false;
        characters[i].isRevealed = true;
        characters[i].portrait = i;
    }

    sightedMask = 0;
    lastGameMoment = 0;
    loadedFieldID = 0;

    inGameTime = "Not connected.";
    currentSong = "";
    modsSummary = "";
}

void Tracker::update()
{
    if (game == nullptr || !game->isConnected())
    {
        return;
    }

    // Permadeath Character Portraits
    {
        Permadeath* permadeathMod = (Permadeath*)ModManager::getMod("Permadeath");
        uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);
        uint16_t deadMask = (permadeathMod != nullptr) ? permadeathMod->getDeadCharacterMask() : 0;

        // With Randomize Characters, each shows who they look like, as a ? until they've been seen.
        bool hideUnseen = ModManager::isModEnabled("Randomize Characters");
        uint16_t gameMoment = game->getGameMoment();
        updateSightings(gameMoment);

        for (uint8_t i = 0; i < 9; ++i)
        {
            characters[i].isActive = Utilities::isBitSet(phsVisMask, i);
            characters[i].isPermadead = Utilities::isBitSet(deadMask, i);
            characters[i].portrait = game->menu.getPortrait(i);

            // Joining reveals a character, except Cloud, who is on the PHS before the opening shows him.
            bool joined = characters[i].isActive && i != CharacterID::Cloud;
            characters[i].isRevealed = !hideUnseen || joined || gameMoment >= RevealMoments[i] || Utilities::isBitSet(sightedMask, i);
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

// Sightings last until the game moment goes back (a new game or an earlier save).
void Tracker::updateSightings(uint16_t gameMoment)
{
    if (gameMoment < lastGameMoment)
    {
        sightedMask = 0;
    }
    lastGameMoment = gameMoment;

    uint16_t fieldID = loadedFieldID;
    if (fieldID == SeventhHeavenField)
    {
        sightedMask |= 1 << CharacterID::Tifa;
    }

    if (fieldID == CoffinRoomField)
    {
        sightedMask |= 1 << CharacterID::Vincent;
    }

    if (gameMoment == 0 && fieldID == OpeningField && game->getGameModule() == GameModule::Field)
    {
        uint8_t script[DirectorScriptSize];
        uint16_t director = game->getScriptExecutionPointer(0);
        if (game->read(FieldScriptOffsets::ScriptStart + DirectorScriptStart, sizeof(script), script))
        {
            for (const OpeningCue& cue : OpeningCues)
            {
                const uint8_t* request = std::search(script, script + sizeof(script), cue.code, cue.code + sizeof(cue.code));
                if (request != script + sizeof(script) && director > DirectorScriptStart + (request - script))
                {
                    sightedMask |= 1 << cue.character;
                }
            }
        }
    }

    if (game->getGameModule() == GameModule::Battle)
    {
        uint16_t formation = game->read<uint16_t>(BattleOffsets::ActiveFormationID);
        if (formation >= MysteryNinjaFormations[0] && formation <= MysteryNinjaFormations[1])
        {
            sightedMask |= 1 << CharacterID::Yuffie;
        }
    }
}

// Fires once a field's data (and scripts) have loaded, unlike the field ID, which changes as the load begins.
void Tracker::onFieldChanged(uint16_t fieldID)
{
    loadedFieldID = fieldID;
}

void Tracker::onGameOver()
{
    gameOverCounter++;
}
