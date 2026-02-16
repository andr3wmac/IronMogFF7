#include "AudioManager.h"
#include "miniaudio.h"
#include "core/utilities/Logging.h"

ma_engine gAudioEngine;
ma_sound gMusicA;
ma_sound gMusicB;
ma_sound* pActiveMusic = nullptr;

bool AudioManager::initialize()
{
    ma_result result;

    ma_engine_config config = ma_engine_config_init();
    config.sampleRate = 48000;
    result = ma_engine_init(&config, &gAudioEngine);

    if (result != MA_SUCCESS)
    {
        return false;
    }

    return true;
}

bool AudioManager::playMusic(std::string path)
{
    return playMusic(path, 0, 0, UINT64_MAX);
}

bool AudioManager::playMusic(std::string path, uint64_t start, uint64_t loopStart, uint64_t loopEnd, bool playOnce, bool noFade)
{
    // Determine which slot to use for the new song
    ma_sound* pOldMusic = pActiveMusic;
    ma_sound* pNewMusic = (pActiveMusic == &gMusicA) ? &gMusicB : &gMusicA;

    int fadeTimeMS = noFade ? 0 : 500;

    // Fade out the old song
    if (pOldMusic != nullptr && ma_sound_is_playing(pOldMusic))
    {
        ma_sound_set_fade_in_milliseconds(pOldMusic, ma_sound_get_volume(pOldMusic), 0.0f, fadeTimeMS);
    }

    // Clean up the song we're about to overwrite
    if (ma_sound_is_playing(pNewMusic) || pNewMusic->pDataSource != NULL)
    {
        ma_sound_stop(pNewMusic);
        ma_sound_uninit(pNewMusic);
    }

    // Initialize the new song
    ma_result result = ma_sound_init_from_file(&gAudioEngine, path.c_str(), MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, pNewMusic);
    if (result != MA_SUCCESS)
    {
        return false;
    }

    // Configure looping and start position
    ma_data_source* dataSrc = ma_sound_get_data_source(pNewMusic);
    ma_data_source_set_looping(dataSrc, playOnce ? MA_FALSE : MA_TRUE);
    if (start > 0)
    {
        ma_data_source_seek_pcm_frames(dataSrc, start, NULL);
    }
    if (!playOnce && (loopStart > 0 || loopEnd < UINT64_MAX))
    {
        ma_data_source_set_loop_point_in_pcm_frames(dataSrc, loopStart, loopEnd);
    }

    // Fade in the new song
    ma_sound_set_fade_in_milliseconds(pNewMusic, 0.0f, 1.0f, fadeTimeMS);
    ma_sound_start(pNewMusic);
    pActiveMusic = pNewMusic;

    return true;
}

void AudioManager::setMusicVolume(float volume)
{
    ma_engine_set_volume(&gAudioEngine, volume);
}

void AudioManager::pauseMusic()
{
    ma_sound_stop(pActiveMusic);
}

void AudioManager::resumeMusic()
{
    ma_sound_start(pActiveMusic);
}