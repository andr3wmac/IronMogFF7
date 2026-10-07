#pragma once
#include "extras/Extra.h"
#include <atomic>
#include <cstdint>
#include <unordered_map>

// Represents a song found in the 'music' folder
struct Track
{
    std::string path;

    // These are all in PCM samples
    uint64_t start = 0;
    uint64_t loopStart = 0;
    uint64_t loopEnd = UINT64_MAX;

    bool playOnce = false;
    bool noFade = false;
};

// Whether the game's own music is audible or muted by us.
enum class GameMusicState : uint8_t
{
    Playing,    // Game music is untouched
    Muting,     // Waiting on the sound driver to apply a silent volume
    Muted       // Game music volume is locked to silent
};

// Debug test that sends the game's own volume commands to check they can't unmute the game music.
enum class VolumeTestState : uint8_t
{
    Idle,
    Requested,
    Running,
    Passed,
    Failed
};

class RandomizeMusic : public Extra
{
public:
    RandomizeMusic();

    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    std::vector<std::string> describe(ExtraDescripionType descType) override;

    bool isPlaying();
    std::string getCurrentlyPlaying();

private:
    void onStart();
    void onExit();
    void onEmulatorPaused();
    void onEmulatorResumed();
    void onUpdate();
    void onFrame(uint32_t frameNumber);

    void muteGameMusic();
    void unmuteGameMusic();
    void updateGameMusicMute();
    void detectGameMusicLeaks();
    void updateVolumeTest();
    void failVolumeTest(const char* reason);

    void scanMusicFolder();
    Track loadTrack(std::string path);
    void addUniqueTrack(const Track& newTrack);
    bool randomizeMusic(uint16_t musicID);
    void play(const Track& track);

    bool useCuratedMusic = true;
    bool disabled = false;
    bool overrideMusic = false;
    int trackCount = 0;
    std::string currentSong = "";
    float currentVolume = 1.0f;
    float previousVolume = 1.0f;
    uint16_t previousMusicID = 0;
    uint8_t previousGameModule = 0;
    uint16_t previousValidStack[2] = { 0, 0 };
    uint8_t previousBattlePaused = 0;
    GameMusicState gameMusicState = GameMusicState::Playing;

    // Leak detection, a leak is an active game music voice with a volume above silent while muted.
    int leakCheckDelay = 0;                 // Frames to wait after muting for the driver to update its voices
    uint32_t leakingTracks[2] = { 0, 0 };   // Per music player, so each leak is only logged once
    std::atomic<int> leakCount = 0;
    std::atomic<int> muteRepairCount = 0;   // Times something undid part of the mute and we had to fix it

    std::atomic<VolumeTestState> volumeTestState = VolumeTestState::Idle;
    int volumeTestStep = 0;
    int volumeTestFrames = 0;
    int volumeTestStartLeaks = 0;
    int volumeTestStartRepairs = 0;

    std::unordered_map<uint16_t, uint16_t> previousTrackSelection;
    std::unordered_map<std::string, std::vector<Track>> musicMap;
    std::vector<Track> uniqueTrackList;
};