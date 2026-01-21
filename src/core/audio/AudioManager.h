#pragma once

#include <string>

class AudioManager
{
public:
    static bool initialize();
    static bool playMusic(std::string path);
    static bool playMusic(std::string path, uint64_t start, uint64_t loopStart, uint64_t loopEnd, bool playOnce = false);
    static void setMusicVolume(float volume);
    static void pauseMusic();
    static void resumeMusic();

    static void testPlay();
    static void testUpdate();
};