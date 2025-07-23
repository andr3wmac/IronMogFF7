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

private:
    void onStart();
    void onEmulatorPaused();
    void onEmulatorResumed();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);

    Track loadTrack(std::string path);

    uint16_t previousMusicID = 0;
    std::unordered_map<std::string, std::vector<Track>> musicMap;
};