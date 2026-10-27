#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

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

// Tracks in the music folder, keyed by the internal name of the game song they replace.
using TrackMap = std::map<std::string, std::vector<Track>>;

namespace MusicLibrary
{
    // Folder the music is loaded from, with a subfolder for each game song.
    extern const std::string MusicFolder;

    // Internal names of the game's songs, indexed by music ID.
    extern const std::vector<std::string> MusicList;

    // Title of a game song from its internal name, e.g. "bat" is "Fighting". Unknown names are returned as is.
    std::string getSongTitle(const std::string& name);

    // Fills tracks from the music folder. Returns false if the folder doesn't exist.
    bool scan(TrackMap& tracks);

    // Loads a track along with the playback settings in its matching .cfg file.
    Track loadTrack(const std::string& path);
}
