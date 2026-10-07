#include "RandomizeMusic.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/GUI.h"
#include "core/utilities/ConfigFile.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
namespace fs = std::filesystem;

const uint16_t UnsetMusicID = 65535;

// The AKAO sound driver's master music volume is 16.16 fixed point and only 
// the integer part masked with 0x7F is used, so 0x80 is silent. 
const uint32_t MusicVolumeOne    = 0x00010000;
const uint32_t MusicVolumeFull   = 0x007F0000;
const uint32_t MusicVolumeSilent = 0x00800000;
const uint32_t MusicVolumeMask   = 0x007F0000;

// A fade that never ends, which also stops the driver from fading in new songs.
const int16_t MusicFadeTicksLocked = 0x7FFF;

// AKAO command handlers that change the master music volume, and a handler that does nothing.
const uint32_t AKAONoopHandler = 0x8002CF98;
const std::pair<uint8_t, uint32_t> AKAOMusicVolumeCommands[] = {
    { 0xC0, 0x8002BA5C },   // Set music volume
    { 0xC1, 0x8002BA98 },   // Fade music volume
    { 0xC2, 0x8002BB20 },   // Fade music volume from one level to another
};

const std::vector<std::string> MusicList = {
    "none", "nothing", "oa", "ob", "dun2", "guitar2", "fanfare", "makoro", "bat",
    "fiddle", "kurai", "chu", "ketc", "earis", "ta", "tb", "sato",
    "parade", "comical", "yume", "mati", "sido", "siera", "walz", "corneo",
    "horror", "canyon", "red", "seto", "ayasi", "sinra", "sinraslo", "dokubo",
    "bokujo", "tm", "tifa", "costa", "rocket", "earislo", "chase", "rukei",
    "cephiros", "barret", "corel", "boo", "elec", "rhythm", "fan2", "hiku",
    "cannon", "date", "cintro", "cinco", "chu2", "yufi", "aseri", "gold1",
    "mura1", "yado", "over2", "crwin", "crlost", "odds", "geki", "junon",
    "tender", "wind", "vincent", "bee", "jukai", "sadbar", "aseri2", "kita",
    "sid2", "sadsid", "iseki", "hen", "utai", "snow", "yufi2", "mekyu",
    "condor", "lb2", "gun", "weapon", "pj", "sea", "ld", "lb1",
    "sensui", "ro", "jyro", "nointro", "riku", "si", "mogu", "pre",
    "fin", "heart", "roll"
};

REGISTER_EXTRA(RandomizeMusic, "Randomize Music", "Music tracks are randomized and can include music from other games.")

RandomizeMusic::RandomizeMusic()
{
    scanMusicFolder();
}

void RandomizeMusic::setup()
{
    BIND_EVENT(game->onStart, RandomizeMusic::onStart);
    BIND_EVENT(game->onExit, RandomizeMusic::onExit);
    BIND_EVENT(game->onEmulatorPaused, RandomizeMusic::onEmulatorPaused);
    BIND_EVENT(game->onEmulatorResumed, RandomizeMusic::onEmulatorResumed);
    BIND_EVENT(game->onUpdate, RandomizeMusic::onUpdate);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeMusic::onFrame);

    previousMusicID = UnsetMusicID;
    previousBattlePaused = 0;
    gameMusicState = GameMusicState::Playing;
}

bool RandomizeMusic::onSettingsGUI()
{
    bool changed = false;

    if (disabled)
    {
        ImGui::Text("No music found, randomization disabled.");
    }

    // Curated Music
    ImGui::Checkbox("Use Curated Music", &useCuratedMusic);
    ImGui::SetItemTooltip("Limits music randomization to songs chosen\nto be appropriate replacements.\ne.g. Battle music randomizes to battle music.");

    // Volume
    constexpr float min = 0.0f;
    constexpr float max = 2.0f;
    ImGui::Text("Volume");
    ImGui::SameLine(DPI(75.0f));
    ImGui::SetNextItemWidth(DPI(250.0f));
    changed |= ImGui::SliderScalar("##musicVolume", ImGuiDataType_Float, &currentVolume, &min, &max, "%.2lf");

    if (currentVolume != previousVolume)
    {
        AudioManager::setMusicVolume(currentVolume);
        previousVolume = currentVolume;
    }

    // Rescan
    if (ImGui::Button("Rescan Music Folder", ImVec2(DPI(150.0f), 0.0f)))
    {
        scanMusicFolder();
    }

    ImGui::SameLine();
    std::string trackCountText = "Tracks: " + std::to_string(trackCount);
    ImGui::Text(trackCountText.c_str());

    // Reroll
    ImGui::BeginDisabled(game == nullptr);
    if (ImGui::Button("Reroll Music", ImVec2(DPI(150.0f), 0.0f)))
    {
        randomizeMusic(previousMusicID);
    }
    ImGui::EndDisabled();

    if (game != nullptr)
    {
        uint16_t musicID = game->read<uint16_t>(GameOffsets::MusicID);
        std::string currentSongText = "Game Music: " + MusicList[musicID] + " (" + std::to_string(musicID) + ")";
        ImGui::Text(currentSongText.c_str());
    }

    return changed;
}

void RandomizeMusic::loadSettings(const ConfigFile& cfg)
{
    useCuratedMusic = cfg.get<bool>("useCuratedMusic", useCuratedMusic);
    currentVolume = cfg.get<float>("volume", currentVolume);
    AudioManager::setMusicVolume(currentVolume);
}

void RandomizeMusic::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("useCuratedMusic", useCuratedMusic);
    cfg.set<float>("volume", currentVolume);
}

void RandomizeMusic::onDebugGUI()
{
    uint16_t musicID = game->read<uint16_t>(GameOffsets::MusicID);
    std::string musicText = "Music: " + std::to_string(musicID);
    ImGui::Text(musicText.c_str());

    if (musicID < MusicList.size())
    {
        std::string internalName = "Internal Name: " + MusicList[musicID];
        ImGui::Text(internalName.c_str());
    }

    std::string validStackStr = "Stack: " + std::to_string(previousValidStack[0]) + " " + std::to_string(previousValidStack[1]);
    ImGui::Text(validStackStr.c_str());

    const char* stateNames[] = { "Playing", "Muting", "Muted" };
    uint32_t volume = game->read<uint32_t>(AKAOOffsets::MusicVolume);
    int16_t fadeTicks = game->read<int16_t>(AKAOOffsets::MusicFadeTicks);
    std::string gameMusicText = "Game Music: " + std::string(stateNames[(int)gameMusicState]) + " (Volume: " +
        Utilities::seedToHexString(volume) + ", Fade: " + std::to_string(fadeTicks) + ")";
    ImGui::Text(gameMusicText.c_str());

    std::string leakText = "Leaks: " + std::to_string(leakCount) + ", Mute Repairs: " + std::to_string(muteRepairCount);
    ImGui::Text(leakText.c_str());

    VolumeTestState testState = volumeTestState;
    bool testRunning = testState == VolumeTestState::Requested || testState == VolumeTestState::Running;
    ImGui::BeginDisabled(testRunning || gameMusicState != GameMusicState::Muted);
    if (ImGui::Button("Test Volume Commands"))
    {
        volumeTestState = VolumeTestState::Requested;
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Sends the game's volume commands to the sound driver\nand checks the game music stays muted.");

    ImGui::SameLine();
    const char* testNames[] = { "", "Running..", "Running..", "Passed", "Failed, see log" };
    ImGui::Text(testNames[(int)testState]);
}

std::vector<std::string> RandomizeMusic::describe(ExtraDescripionType descType)
{
    if (descType == ExtraDescripionType::Randomized)
    {
        return { "Music" };
    }

    return {};
}

bool RandomizeMusic::isPlaying()
{
    if (disabled)
    {
        return false;
    }

    return true;
}

std::string RandomizeMusic::getCurrentlyPlaying()
{
    if (disabled)
    {
        return "";
    }

    return currentSong;
}

void RandomizeMusic::onStart()
{
    scanMusicFolder();
    previousMusicID = UnsetMusicID;

    // Game music stays muted unless we find a song we don't have a replacement for. Muting up
    // front means a song can't sneak a few notes out before we notice its music ID changed.
    if (!disabled && enabled)
    {
        muteGameMusic();
    }
}

void RandomizeMusic::onExit()
{
    // Leave the game the way we found it.
    if (gameMusicState != GameMusicState::Playing)
    {
        unmuteGameMusic();
    }

    overrideMusic = false;
    currentSong = "";
    previousMusicID = UnsetMusicID;
}

void RandomizeMusic::onEmulatorPaused()
{
    if (disabled)
    {
        return;
    }

    AudioManager::pauseMusic();
}

void RandomizeMusic::onEmulatorResumed()
{
    if (disabled || !overrideMusic || currentSong == "")
    {
        return;
    }

    AudioManager::resumeMusic();
}

void RandomizeMusic::onUpdate()
{
    // Updates run more often than frames so we're more likely to see short lived leaks.
    if (!disabled && enabled && gameMusicState == GameMusicState::Muted && leakCheckDelay == 0)
    {
        detectGameMusicLeaks();
    }
}

void RandomizeMusic::onFrame(uint32_t frameNumber)
{
    // Extras can be turned off mid-game, in which case we hand the music back to the game.
    if (disabled || !enabled)
    {
        if (gameMusicState != GameMusicState::Playing)
        {
            unmuteGameMusic();
        }

        if (overrideMusic)
        {
            overrideMusic = false;
            currentSong = "";
            AudioManager::pauseMusic();
        }

        previousMusicID = UnsetMusicID;
        return;
    }

    // Fix for Midgar raid skip music
    if (game->getFieldID() == 741)
    {
        if (game->read<uint8_t>(GameOffsets::MusicLock) == 1)
        {
            if (game->getWindowText(0) == "Cloud �Hojo!  Stop right there!!�")
            {
                game->write<uint8_t>(GameOffsets::MusicLock, 0);
            }
        }
    }

    // Handle pausing in battles
    if (game->inBattle())
    {
        uint8_t battlePaused = game->read<uint8_t>(0x9A118);
        if (battlePaused != previousBattlePaused)
        {
            if (battlePaused == 0xFF)
            {
                LOG("Battle paused.");
                AudioManager::pauseMusic();
            }
            else
            {
                LOG("Battle resumed.");
                AudioManager::resumeMusic();
            }
            previousBattlePaused = battlePaused;
        }
    }

    updateGameMusicMute();
    updateVolumeTest();

    uint16_t musicID = game->read<uint16_t>(GameOffsets::MusicID);
    if (musicID != previousMusicID)
    {
        // We track our previous selections and don't reroll field music when exiting battles.
        bool usePreviousTrackSelection = false;
        uint8_t currentGameModule = game->getGameModule();
        if (previousGameModule != currentGameModule)
        {
            if (previousGameModule == GameModule::Battle && currentGameModule != GameModule::Battle)
            {
                usePreviousTrackSelection = true;
            }
        }

        previousMusicID = musicID;
        previousGameModule = currentGameModule;

        // 0 and 1 are nothing so if thats switched to we need to pause any running tracks.
        if (musicID == 0 || musicID == 1)
        {
            currentSong = "";
            AudioManager::pauseMusic();

            // Nothing should be playing anyway, but this keeps the next song muted from its first note.
            if (gameMusicState == GameMusicState::Playing)
            {
                muteGameMusic();
            }
            return;
        }

        // Reuse recent songs except in battle. The point of this is just for continuity when
        // songs change temporarily. For example: when you sleep at an inn.
        if (currentGameModule != GameModule::Battle && previousValidStack[1] == musicID)
        {
            usePreviousTrackSelection = true;
        }
        std::swap(previousValidStack[0], previousValidStack[1]);
        previousValidStack[0] = musicID;

        bool didRandomize = false;

        // Reuse previously selected random track.
        if (usePreviousTrackSelection)
        {
            if (useCuratedMusic)
            {
                std::vector<Track> tracks = musicMap[MusicList[musicID]];
                uint16_t selectedMusic = previousTrackSelection[musicID];

                if (selectedMusic < tracks.size())
                {
                    const Track& track = tracks[selectedMusic];
                    play(track);
                    didRandomize = true;
                }
            }
            else
            {
                uint16_t selectedMusic = previousTrackSelection[musicID];
                if (selectedMusic < uniqueTrackList.size())
                {
                    const Track& track = uniqueTrackList[selectedMusic];
                    play(track);
                    didRandomize = true;
                }
            }
        }
        else 
        {
            didRandomize = randomizeMusic(musicID);
        }

        if (didRandomize)
        {
            overrideMusic = true;
            if (gameMusicState == GameMusicState::Playing)
            {
                muteGameMusic();
            }
        }
        else
        {
            // No tracks available for this music ID, stop overriding and let the game take over.
            currentSong = "";
            overrideMusic = false;
            if (gameMusicState != GameMusicState::Playing)
            {
                unmuteGameMusic();
            }
            AudioManager::pauseMusic();
            LOG("No tracks available, resuming in-game music.");
        }
    }
}

void RandomizeMusic::muteGameMusic()
{
    // Stop the game from changing the master music volume. Only the command table is changed rather than
    // the driver code because emulator recompilers won't notice code being changed from outside the emulator.
    for (const auto& [command, handler] : AKAOMusicVolumeCommands)
    {
        game->write<uint32_t>(AKAOOffsets::CommandTable + (command * 4), AKAONoopHandler);
    }

    // The driver only recalculates voice volumes when the master volume changes on its own, so rather than
    // setting it to silent directly we have it fade from 1 to silent in a single step. Fade ticks are written
    // first and last so the driver can't apply a half written fade.
    game->write<int16_t>(AKAOOffsets::MusicFadeTicks, 0);
    game->write<int32_t>(AKAOOffsets::MusicFadeDelta, MusicVolumeSilent - MusicVolumeOne);
    game->write<uint32_t>(AKAOOffsets::MusicVolume, MusicVolumeOne);
    game->write<int16_t>(AKAOOffsets::MusicFadeTicks, 1);

    gameMusicState = GameMusicState::Muting;
}

void RandomizeMusic::unmuteGameMusic()
{
    for (const auto& [command, handler] : AKAOMusicVolumeCommands)
    {
        game->write<uint32_t>(AKAOOffsets::CommandTable + (command * 4), handler);
    }

    // Fade from silent to full in a single step so the driver recalculates the volume of playing voices.
    game->write<int16_t>(AKAOOffsets::MusicFadeTicks, 0);
    game->write<int32_t>(AKAOOffsets::MusicFadeDelta, MusicVolumeFull);
    game->write<uint32_t>(AKAOOffsets::MusicVolume, 0);
    game->write<int16_t>(AKAOOffsets::MusicFadeTicks, 1);

    gameMusicState = GameMusicState::Playing;
}

void RandomizeMusic::updateGameMusicMute()
{
    if (gameMusicState == GameMusicState::Playing)
    {
        return;
    }

    int16_t fadeTicks = game->read<int16_t>(AKAOOffsets::MusicFadeTicks);

    if (gameMusicState == GameMusicState::Muting)
    {
        // Still waiting for the driver to apply the single step fade to silent.
        if (fadeTicks == 1)
        {
            return;
        }

        // Give the driver a moment to send the new volumes to the voices before checking for leaks.
        gameMusicState = GameMusicState::Muted;
        leakCheckDelay = 3;
        leakingTracks[0] = 0;
        leakingTracks[1] = 0;

        // The fade was ours so it doesn't count as a repair below.
        game->write<int32_t>(AKAOOffsets::MusicFadeDelta, 0);
        game->write<int16_t>(AKAOOffsets::MusicFadeTicks, MusicFadeTicksLocked);
        fadeTicks = MusicFadeTicksLocked;
    }

    if (leakCheckDelay > 0)
    {
        leakCheckDelay--;
    }

    // Lock the volume with a fade that never moves or ends. While a fade is running the driver
    // skips fading in new songs, which would otherwise briefly raise the volume.
    int32_t fadeDelta = game->read<int32_t>(AKAOOffsets::MusicFadeDelta);
    if (fadeDelta != 0)
    {
        LOG("Game music fade changed to %d, repairing mute.", fadeDelta);
        game->write<int32_t>(AKAOOffsets::MusicFadeDelta, 0);
        muteRepairCount++;
    }

    if (fadeTicks < MusicFadeTicksLocked / 2)
    {
        game->write<int16_t>(AKAOOffsets::MusicFadeTicks, MusicFadeTicksLocked);
    }

    // Loading a save state from before we muted would bring back the original handlers.
    for (const auto& [command, handler] : AKAOMusicVolumeCommands)
    {
        uintptr_t entry = AKAOOffsets::CommandTable + (command * 4);
        uint32_t entryHandler = game->read<uint32_t>(entry);
        if (entryHandler != AKAONoopHandler)
        {
            LOG("Game music command %02X handler changed to %08X, repairing mute.", command, entryHandler);
            game->write<uint32_t>(entry, AKAONoopHandler);
            muteRepairCount++;
        }
    }

    // If anything managed to make the music audible then mute it again.
    uint32_t volume = game->read<uint32_t>(AKAOOffsets::MusicVolume);
    if ((volume & MusicVolumeMask) != 0)
    {
        LOG("Game music volume changed to %08X, muting again.", volume);
        muteGameMusic();
        muteRepairCount++;
    }
}

void RandomizeMusic::detectGameMusicLeaks()
{
    const uintptr_t trackTables[2] = { AKAOOffsets::MusicTracks, AKAOOffsets::Music2Tracks };
    const uintptr_t activeMasks[2] = { AKAOOffsets::MusicTracksActive, AKAOOffsets::Music2TracksActive };

    for (int player = 0; player < 2; ++player)
    {
        uint32_t activeTracks = game->read<uint32_t>(activeMasks[player]);
        uint32_t leaking = 0;

        for (int track = 0; track < 24; ++track)
        {
            uint32_t trackBit = 1 << track;
            if ((activeTracks & trackBit) == 0)
            {
                continue;
            }

            uintptr_t volumeOffset = trackTables[player] + (track * AKAOOffsets::TrackStride) + AKAOOffsets::TrackVoiceVolume;
            uint32_t volumes = game->read<uint32_t>(volumeOffset);
            int16_t left = (int16_t)(volumes & 0xFFFF);
            int16_t right = (int16_t)(volumes >> 16);

            // Silent voices are 0, or -1 when the driver inverts the phase of a channel.
            if (std::abs(left) <= 1 && std::abs(right) <= 1)
            {
                continue;
            }

            leaking |= trackBit;
            if ((leakingTracks[player] & trackBit) == 0)
            {
                leakCount++;
                LOG("Game music leak: field %d, music %d, player %d, track %d, volume %d/%d", game->getFieldID(),
                    game->read<uint16_t>(GameOffsets::MusicID), player, track, left, right);
            }
        }

        leakingTracks[player] = leaking;
    }
}

void RandomizeMusic::updateVolumeTest()
{
    VolumeTestState testState = volumeTestState;
    if (testState == VolumeTestState::Requested)
    {
        LOG("Volume command test started.");
        volumeTestState = VolumeTestState::Running;
        volumeTestStep = 0;
        volumeTestFrames = 0;
        volumeTestStartLeaks = leakCount;
        volumeTestStartRepairs = muteRepairCount;
    }
    else if (testState != VolumeTestState::Running)
    {
        return;
    }

    if (gameMusicState != GameMusicState::Muted)
    {
        failVolumeTest("game music stopped being muted during the test");
        return;
    }

    // Each command would make the music audible if it reached its real handler.
    const uint32_t testCommands[3][4] = {
        { 0xC0, 0x7F, 0,    0    },     // Set volume to full
        { 0xC1, 30,   0x7F, 0    },     // Fade to full over 30 ticks
        { 0xC2, 30,   0,    0x7F },     // Fade from silent to full over 30 ticks
    };

    volumeTestFrames++;
    uint32_t queuedCommands = game->read<uint32_t>(AKAOOffsets::CommandCount);

    if (volumeTestStep < 3)
    {
        // Only add a command when the queue is empty and the game isn't adding one of its own.
        if (queuedCommands == 0 && game->read<uint32_t>(AKAOOffsets::CommandBusy) == 0)
        {
            uint8_t entry[AKAOOffsets::CommandStride] = {};
            memcpy(entry, testCommands[volumeTestStep], sizeof(testCommands[volumeTestStep]));
            game->write(AKAOOffsets::CommandQueue, entry, sizeof(entry));
            game->write<uint32_t>(AKAOOffsets::CommandCount, 1);

            volumeTestStep++;
            volumeTestFrames = 0;
        }
        else if (volumeTestFrames > 60)
        {
            failVolumeTest("the command queue never became free");
        }
        return;
    }

    // The driver empties the queue every tick so the commands should be gone almost immediately.
    if (queuedCommands != 0)
    {
        if (volumeTestFrames > 60)
        {
            failVolumeTest("the sound driver never ran the commands");
        }
        return;
    }

    if (leakCount != volumeTestStartLeaks)
    {
        failVolumeTest("game music leaked");
        return;
    }

    if (muteRepairCount != volumeTestStartRepairs)
    {
        failVolumeTest("a command changed the game music volume");
        return;
    }

    // Wait longer than the fades would have taken to be sure nothing changes.
    if (volumeTestFrames > 60)
    {
        LOG("Volume command test passed.");
        volumeTestState = VolumeTestState::Passed;
    }
}

void RandomizeMusic::failVolumeTest(const char* reason)
{
    LOG("Volume command test failed: %s.", reason);
    volumeTestState = VolumeTestState::Failed;
}

void RandomizeMusic::scanMusicFolder()
{
    musicMap.clear();
    uniqueTrackList.clear();
    trackCount = 0;

    // Scan music folder
    const std::string basePath = "music";

    if (!fs::exists(basePath) || !fs::is_directory(basePath))
    {
        LOG("Randomize Music Error: music directory does not exist.");
        disabled = true;
        return;
    }

    for (const auto& musicEntry : fs::directory_iterator(basePath)) 
    {
        if (!musicEntry.is_directory())
        {
            continue;
        }

        const std::string& name = musicEntry.path().filename().string();

        // This is to prevent someone from overriding silence.
        if (name == "none" || name == "nothing")
        {
            continue;
        }

        fs::path subdir = fs::path(basePath) / name;

        if (!fs::exists(subdir) || !fs::is_directory(subdir))
        {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(subdir))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::string ext = entry.path().extension().string();
            for (char& c : ext) c = std::tolower(c);

            if (ext == ".mp3" || ext == ".wav")
            {
                Track track = loadTrack(entry.path().string());
                musicMap[name].push_back(track);
                addUniqueTrack(track);
                trackCount++;
            }
        }
    }

    if (trackCount > 0)
    {
        disabled = false;
    }
    else
    {
        LOG("Randomize Music Error: no music was found.");
        disabled = true;
    }
}

Track RandomizeMusic::loadTrack(std::string path)
{
    Track track;
    track.path = path;

    std::string cfgFilename = Utilities::replaceExtension(path, ".mp3", ".cfg");
    cfgFilename = Utilities::replaceExtension(cfgFilename, ".wav", ".cfg");

    ConfigFile cfg;
    if (!cfg.load(cfgFilename))
    {
        return track;
    }

    track.start     = cfg.get<uint64_t>("Start", 0);
    track.loopStart = cfg.get<uint64_t>("LoopStart", 0);
    track.loopEnd   = cfg.get<uint64_t>("LoopEnd", UINT64_MAX);
    track.playOnce  = cfg.get<bool>("PlayOnce", false);
    track.noFade    = cfg.get<bool>("NoFade", false);

    return track;
}

void RandomizeMusic::addUniqueTrack(const Track& newTrack)
{
    // Extract the filename once before starting the loop for efficiency
    std::string newFileName = fs::path(newTrack.path).filename().string();
    bool isDuplicate = false;

    for (const Track& track : uniqueTrackList) 
    {
        // Extract the filename of the track currently being inspected
        std::string existingFileName = fs::path(track.path).filename().string();

        // Check if filename and parameters all match
        if (existingFileName == newFileName &&
            track.start == newTrack.start &&
            track.loopStart == newTrack.loopStart &&
            track.loopEnd == newTrack.loopEnd &&
            track.playOnce == newTrack.playOnce &&
            track.noFade == newTrack.noFade)
        {
            isDuplicate = true;
            break;
        }
    }

    if (!isDuplicate) 
    {
        uniqueTrackList.push_back(newTrack);
    }
}

bool RandomizeMusic::randomizeMusic(uint16_t musicID)
{
    if (musicID >= MusicList.size())
    {
        return false;
    }

    static std::mt19937 rng(std::random_device{}());

    if (useCuratedMusic)
    {
        std::vector<Track> tracks;
        uint8_t gameModule = game->getGameModule();

        // Special cases
        if (musicMap.count("snowboarding") > 0 && (gameModule == GameModule::Snowboarding1 || gameModule == GameModule::Snowboarding2))
        {
            tracks = musicMap["snowboarding"];
        }
        else if (musicMap.count(MusicList[musicID]) > 0)
        {
            tracks = musicMap[MusicList[musicID]];
        }

        // Exit if we haven't found any candidates to play.
        if (tracks.size() == 0)
        {
            return false;
        }

        // Randomly select a track from the choices for this music ID
        std::uniform_int_distribution<size_t> dist(0, tracks.size() - 1);
        uint16_t selectedMusic = (uint16_t)dist(rng);

        // Play track
        Track& track = tracks[selectedMusic];
        play(track);
        previousTrackSelection[musicID] = selectedMusic;
    }
    else 
    {
        // Randomly select a track from the unique song list
        std::uniform_int_distribution<size_t> dist(0, uniqueTrackList.size() - 1);
        uint16_t selectedMusic = (uint16_t)dist(rng);

        // Play track
        Track& track = uniqueTrackList[selectedMusic];
        play(track);
        previousTrackSelection[musicID] = selectedMusic;
    }

    return true;
}

void RandomizeMusic::play(const Track& track)
{
    std::filesystem::path p(track.path);
    currentSong = p.stem().string();

    overrideMusic = true;
    AudioManager::playMusic(track.path, track.start, track.loopStart, track.loopEnd, track.playOnce, track.noFade);
    LOG("Playing: %s", track.path.c_str());
}