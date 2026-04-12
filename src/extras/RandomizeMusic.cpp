#include "RandomizeMusic.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/GUI.h"
#include "core/utilities/ConfigFile.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <filesystem>
namespace fs = std::filesystem;

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

REGISTER_EXTRA(RandomizeMusic, "Randomize Music", "Music tracks are randomized and can include music from other games.")

RandomizeMusic::RandomizeMusic()
{
    scanMusicFolder();
}

void RandomizeMusic::setup()
{
    BIND_EVENT(game->onStart, RandomizeMusic::onStart);
    BIND_EVENT(game->onEmulatorPaused, RandomizeMusic::onEmulatorPaused);
    BIND_EVENT(game->onEmulatorResumed, RandomizeMusic::onEmulatorResumed);
    BIND_EVENT(game->onUpdate, RandomizeMusic::onUpdate);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeMusic::onFrame);

    previousMusicID = UnsetMusicID;
    previousBattlePaused = 0;
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
    if (disabled)
    {
        return;
    }

    // Keep master music volume locked to 1 (lowest volume)
    if (overrideMusic)
    {
        game->write<uint16_t>(GameOffsets::MusicVolume, 1);
    }
}

void RandomizeMusic::onFrame(uint32_t frameNumber)
{
    if (disabled)
    {
        return;
    }

    // Fix for Midgar raid skip music
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

    if (overrideMusic)
    {
        // Keep master music volume locked to 1 (lowest volume)
        game->write<uint16_t>(GameOffsets::MusicVolume, 1);

        // Set all of the AKOA track volumes to 0
        for (int i = 0; i < 24; i++)
        {
            uint32_t akaoTrackAddr = AKAOOffsets::TrackStart + (i * AKAOOffsets::TrackStride) + AKAOOffsets::MasterVolume;
            game->write<uint32_t>(akaoTrackAddr, 0);
        }
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