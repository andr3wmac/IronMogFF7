#pragma once
#include "extras/Extra.h"
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
};

class RandomizeMusic : public Extra
{
public:
    void setup() override;
    bool hasSettings() override { return true; }
    void onSettingsGUI() override;

    bool isPlaying();
    std::string getCurrentlyPlaying();

private:
    void onStart();
    void onEmulatorPaused();
    void onEmulatorResumed();
    void onFrame(uint32_t frameNumber);

    void scanMusicFolder();
    Track loadTrack(std::string path);

    bool disabled = false;
    bool overrideMusic = false;
    std::string currentSong = "";
    float currentVolume = 1.0f;
    float previousVolume = 1.0f;
    uint16_t previousMusicID = 0;
    uint8_t previousGameModule = 0;
    std::unordered_map<uint16_t, uint16_t> previousTrackSelection;
    std::unordered_map<std::string, std::vector<Track>> musicMap;
};