#include "app/App.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    AudioManager::initialize();
    GameData::loadGameData();

    App app;
    app.run();

    return 0;
}