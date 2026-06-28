#include "app/App.h"
#include "app/audio/AudioManager.h"
#include "AppFrame/EntryPoint.h"

class IronMogApp : public App
{
protected:
    bool onInitialize() override
    {
        AudioManager::initialize();
        return App::onInitialize();
    }
};

APPFRAME_MAIN(IronMogApp)
