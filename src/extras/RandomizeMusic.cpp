#include "RandomizeMusic.h"
#include "audio/AudioManager.h"
#include "game/GameData.h"
#include "game/MemoryOffsets.h"
#include "utilities/Logging.h"
#include "utilities/Utilities.h"

#include <filesystem>
namespace fs = std::filesystem;

REGISTER_EXTRA("Randomize Music", RandomizeMusic)

const uint32_t AKAOHeader = 0x4F414B41; // The letters 'AKAO'
const uint16_t UnsetMusicID = 65535;

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
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeMusic::onFieldChanged);

    previousMusicID = UnsetMusicID;
}

void RandomizeMusic::onStart()
{
    // Scan music folder
    const std::string basePath = "music";

    if (!fs::exists(basePath) || !fs::is_directory(basePath)) 
    {
        LOG("Music directory does not exist.");
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
            }
        }
    }
}

void RandomizeMusic::onEmulatorPaused()
{
    AudioManager::pauseMusic();
}

void RandomizeMusic::onEmulatorResumed()
{
    AudioManager::resumeMusic();
}

void RandomizeMusic::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() == GameModule::Battle)
    {
        uint32_t battleMusicHeader = game->read<uint32_t>(0x1D0000);
        if (battleMusicHeader == AKAOHeader)
        {
            game->write<uint32_t>(0x1D0000 + 16, 0);
            LOG("KILLED FANFARE MUSIC!");
        }
    }

    uint16_t musicID = game->read<uint16_t>(GameOffsets::MusicID);
    if (musicID != previousMusicID)
    {
        LOG("MUSIC CHANGED FROM %d TO %d", previousMusicID, musicID);

        if (previousMusicID == UnsetMusicID)
        {
            // If the music is currently UnsetMusicID then this is the first time we're reading
            // the music variable. If this is a new game, the first music will be 0 which is nothing.
            // If this isn't a new game then we can't gaurantee the music was killed in the game itself
            // so we avoid playing anything to dodge playing music over music.
            previousMusicID = musicID;
            return;
        }

        previousMusicID = musicID;

        // 0 and 1 are nothing so if thats switched to we need to pause any running tracks.
        if (musicID == 0 || musicID == 1)
        {
            AudioManager::pauseMusic();
            return;
        }

        if (musicID >= MusicList.size())
        {
            AudioManager::pauseMusic();
            return;
        }

        if (musicMap.count(MusicList[musicID]) == 0)
        {
            AudioManager::pauseMusic();
            return;
        }

        std::vector<Track> tracks = musicMap[MusicList[musicID]];

        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, tracks.size() - 1);
        int selectedMusic = dist(rng);

        Track& track = tracks[selectedMusic];
        LOG("Playing: %s", track.path.c_str());
        AudioManager::playMusic(track.path, track.start, track.loopStart, track.loopEnd);
    }
}

void RandomizeMusic::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    for (int i = 0; i < fieldData.music.size(); ++i)
    {
        FieldMusicData& music = fieldData.music[i];
        uintptr_t musicOffset = FieldScriptOffsets::ScriptStart + music.offset;
        uintptr_t musicIDOffset = musicOffset + 4;

        uint32_t musicHeader = game->read<uint32_t>(musicOffset);
        uint8_t musicID = game->read<uint8_t>(musicIDOffset);

        if (music.id == 0 || music.id == 1)
        {
            continue;
        }

        if (musicHeader == AKAOHeader && music.id == musicID)
        {
            game->write<uint32_t>(musicIDOffset + 12, 0);
            LOG("KILLED MUSIC! %d %d", fieldID, musicID);
        }
    }
}

Track RandomizeMusic::loadTrack(std::string path)
{
    Track track;

    track.path = path;

    std::string cfgFilename = Utilities::replaceExtension(path, ".mp3", ".cfg");
    cfgFilename = Utilities::replaceExtension(cfgFilename, ".wav", ".cfg");

    std::unordered_map<std::string, std::string> cfg = Utilities::loadConfig(cfgFilename);

    if (cfg.count("Start") > 0)
    {
        track.start = std::stoull(cfg["Start"]);
    }

    if (cfg.count("LoopStart") > 0)
    {
        track.loopStart = std::stoull(cfg["LoopStart"]);
    }

    if (cfg.count("LoopEnd") > 0)
    {
        track.loopEnd = std::stoull(cfg["LoopEnd"]);
    }

    return track;
}