#pragma once

#include "app/audio/MusicLibrary.h"

#include <string>
#include <vector>

class GameManager;
class RandomizeMusic;

// Tools panel that lists the music folder by the game song each track replaces and controls playback.
class MusicManager
{
public:
    // The game is null while not connected.
    void draw(GameManager* game);

private:
    struct LibraryTrack
    {
        Track track;
        std::string title;      // File name without the game prefix, e.g. "Battle Theme"
        std::string source;     // Game prefix of the file name, e.g. "FF6"
        std::string searchText; // Lowercase file name
    };

    // The tracks that replace one game song.
    struct LibraryCategory
    {
        std::string name;       // Internal name of the game song, e.g. "bat"
        std::string title;      // Title of the game song, e.g. "Fighting"
        std::string searchText; // Lowercase title and name, also used to sort the list
        std::vector<LibraryTrack> tracks;
    };

    // A track's place in the library.
    struct TrackRef
    {
        int category = -1;
        int track = -1;

        bool isValid() const { return category >= 0; }
        bool operator==(const TrackRef& other) const { return category == other.category && track == other.track; }
    };

    void scan();
    void play(TrackRef ref);
    void playAdjacent(int direction);
    const LibraryTrack* getTrack(TrackRef ref) const;
    TrackRef findTrack(const std::string& path) const;
    bool categoryMatches(int category) const;
    bool trackMatches(TrackRef ref) const;

    void drawNowPlaying(GameManager* game, RandomizeMusic* musicMod);
    void drawTransport(RandomizeMusic* musicMod);
    void drawTrackList(RandomizeMusic* musicMod);

    bool scanned = false;
    bool folderFound = false;
    int trackCount = 0;
    std::vector<LibraryCategory> library;

    TrackRef selected;
    TrackRef playing;
    std::string playingPath;    // Song playing was looked up from, so we only search again when it changes

    char filter[64] = "";
    std::string filterLower;

    // While the seek bar is held we show where it's being dragged to, and seek once it's let go.
    bool seeking = false;
    float seekPosition = 0.0f;
};
