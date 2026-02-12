#include "app/App.h"
#include "core/audio/AudioManager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    AudioManager::initialize();

    App app;
    app.run();

    return 0;
}