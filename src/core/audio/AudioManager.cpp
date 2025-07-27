#include "AudioManager.h"
#include "miniaudio.h"
#include "core/utilities/Logging.h"

ma_engine gAudioEngine;
ma_sound gCurrentMusic;

bool AudioManager::initialize()
{
    ma_result result;

    ma_engine_config config = ma_engine_config_init();
    config.sampleRate = 44100;
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

bool AudioManager::playMusic(std::string path, uint64_t start, uint64_t loopStart, uint64_t loopEnd)
{
    if (gCurrentMusic.pDataSource != NULL)
    {
        ma_sound_stop(&gCurrentMusic);
        ma_sound_uninit(&gCurrentMusic);
    }

    ma_result result = ma_sound_init_from_file(&gAudioEngine, path.c_str(), MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &gCurrentMusic);
    if (result != MA_SUCCESS)
    {
        return false;
    }

    ma_data_source* dataSrc = ma_sound_get_data_source(&gCurrentMusic);
    ma_data_source_set_looping(dataSrc, MA_TRUE);

    if (start > 0)
    {
        ma_data_source_seek_pcm_frames(dataSrc, start, NULL);
    }
    
    if (loopStart > 0 || loopEnd < UINT64_MAX)
    {
        ma_data_source_set_loop_point_in_pcm_frames(dataSrc, loopStart, loopEnd);
    }

    ma_sound_start(&gCurrentMusic);
    return true;
}

void AudioManager::pauseMusic()
{
    ma_sound_stop(&gCurrentMusic);
}

void AudioManager::resumeMusic()
{
    ma_sound_start(&gCurrentMusic);
}

void AudioManager::testPlay()
{
    ma_result result = ma_sound_init_from_file(&gAudioEngine, "music/oa/ff8_joriku.mp3", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &gCurrentMusic);
    ma_data_source* dataSrc = ma_sound_get_data_source(&gCurrentMusic);

    ma_data_source_set_looping(dataSrc, MA_TRUE);
    ma_data_source_seek_pcm_frames(dataSrc, 2579850, NULL);
    ma_data_source_set_loop_point_in_pcm_frames(dataSrc, 2447550, 11555543);
    ma_sound_start(&gCurrentMusic);
}

void AudioManager::testUpdate()
{
    ma_uint32 sampleRate;
    ma_result result;
    result = ma_sound_get_data_format(&gCurrentMusic, NULL, NULL, &sampleRate, NULL, 0);

    ma_uint64 currentCursor = 0;
    ma_uint64 trackLength = 0;
    ma_data_source_get_cursor_in_pcm_frames(ma_sound_get_data_source(&gCurrentMusic), &currentCursor);
    ma_data_source_get_length_in_pcm_frames(ma_sound_get_data_source(&gCurrentMusic), &trackLength);
    LOG("AUDIO CURSOR: %d / %d @ %d", currentCursor, trackLength, sampleRate);
}