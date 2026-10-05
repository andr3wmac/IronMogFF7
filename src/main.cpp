#include "app/App.h"
#include "app/audio/AudioManager.h"
#include "AppFrame/EntryPoint.h"

class LiveModFF7App : public App
{
protected:
    bool onInitialize() override
    {
        AudioManager::initialize();
        return App::onInitialize();
    }

    void onShutdown() override
    {
        // App stops the manager thread first, so nothing is still using audio when it shuts down.
        App::onShutdown();
        AudioManager::shutdown();
    }
};

APPFRAME_MAIN(LiveModFF7App)
