#pragma once

#include <string>

class AudioManager
{
public:
    static bool initialize();
    static void shutdown();
    static bool playMusic(std::string path);
    static bool playMusic(std::string path, uint64_t start, uint64_t loopStart, uint64_t loopEnd, bool playOnce = false, bool noFade = false);
    static void setMusicVolume(float volume);
    static void pauseMusic();
    static void resumeMusic();
    static void seekMusic(float seconds);

    static bool isMusicPlaying();
    static std::string getMusicPath();      // Path of the current song, empty if nothing has been played
    static float getMusicPosition();        // Seconds
    static float getMusicLength();          // Seconds, 0 while still unknown
};