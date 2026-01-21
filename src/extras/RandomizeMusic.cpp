#include "RandomizeMusic.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/ConfigFile.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <filesystem>
namespace fs = std::filesystem;

REGISTER_EXTRA("Randomize Music", RandomizeMusic)

const uint16_t UnsetMusicID = 65535;
const uint16_t FullVolume = 0x7F;

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

void RandomizeMusic::setup()
{
    BIND_EVENT(game->onStart, RandomizeMusic::onStart);
    BIND_EVENT(game->onEmulatorPaused, RandomizeMusic::onEmulatorPaused);
    BIND_EVENT(game->onEmulatorResumed, RandomizeMusic::onEmulatorResumed);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeMusic::onFrame);

    previousMusicID = UnsetMusicID;
}

bool RandomizeMusic::onSettingsGUI()
{
    bool changed = false;

    if (disabled)
    {
        ImGui::Text("No music found, randomization disabled.");
    }

    constexpr float min = 0.0f;
    constexpr float max = 2.0f;

    changed |= ImGui::SliderScalar("Volume", ImGuiDataType_Float, &currentVolume, &min, &max, "%.3lf");

    if (currentVolume != previousVolume)
    {
        AudioManager::setMusicVolume(currentVolume);
        previousVolume = currentVolume;
    }

    if (ImGui::Button("Rescan Music Folder", ImVec2(150, 0)))
    {
        scanMusicFolder();
    }

    ImGui::SameLine();
    std::string trackCountText = "Tracks: " + std::to_string(trackCount);
    ImGui::Text(trackCountText.c_str());

    if (ImGui::Button("Reroll Music", ImVec2(150, 0)))
    {
        randomizeMusic(previousMusicID);
    }

    return changed;
}

void RandomizeMusic::loadSettings(const ConfigFile& cfg)
{
    currentVolume = cfg.get<float>("volume", 5);
}

void RandomizeMusic::saveSettings(ConfigFile& cfg)
{
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
    if (disabled)
    {
        return;
    }

    AudioManager::resumeMusic();
}

void RandomizeMusic::onFrame(uint32_t frameNumber)
{
    if (disabled)
    {
        return;
    }

    // Fix for midgar raid skip music
    if (game->getFieldID() == 741)
    {
        if (game->read<uint8_t>(GameOffsets::MusicLock) == 1)
        {
            if (game->getWindowText(0) == "Cloud ‘Hojo!  Stop right there!!’")
            {
                game->write<uint8_t>(GameOffsets::MusicLock, 0);
            }
        }
    }

    // Keep in game music volume locked to 0
    if (overrideMusic)
    {
        game->write<uint16_t>(GameOffsets::MusicVolume, 1);
    }

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
            didRandomize = randomizeMusic(musicID);
        }

        if (didRandomize)
        {
            overrideMusic = true;
            game->write<uint16_t>(GameOffsets::MusicVolume, 1);
        }
        else
        {
            // No tracks available for this music ID, stop overriding and let the game take over.
            currentSong = "";
            overrideMusic = false;
            game->write<uint16_t>(GameOffsets::MusicVolume, FullVolume);
            AudioManager::pauseMusic();
            LOG("No tracks available, resuming in-game music.");
        }
    }
}

void RandomizeMusic::scanMusicFolder()
{
    musicMap.clear();
    trackCount = 0;

    // Scan music folder
    const std::string basePath = "music";

    if (!fs::exists(basePath) || !fs::is_directory(basePath))
    {
        LOG("Randomize Music Error: music directory does not exist.");
        disabled = true;
        return;
    }

    for (const std::string& name : MusicList)
    {
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
                musicMap[name].push_back(loadTrack(entry.path().string()));
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

    return track;
}

bool RandomizeMusic::randomizeMusic(uint16_t musicID)
{
    if (musicID >= MusicList.size() || musicMap.count(MusicList[musicID]) == 0)
    {
        return false;
    }

    // Get available tracks for this music ID
    std::vector<Track> tracks = musicMap[MusicList[musicID]];
    if (tracks.size() == 0)
    {
        return false;
    }

    // Randomly select a track from the choices for this music ID
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, tracks.size() - 1);
    uint16_t selectedMusic = (uint16_t)dist(rng);

    // Play track
    Track& track = tracks[selectedMusic];
    play(track);
    previousTrackSelection[musicID] = selectedMusic;

    return true;
}

void RandomizeMusic::play(const Track& track)
{
    std::filesystem::path p(track.path);
    currentSong = p.stem().string();

    AudioManager::playMusic(track.path, track.start, track.loopStart, track.loopEnd, track.playOnce);
    LOG("Playing: %s", track.path.c_str());
}