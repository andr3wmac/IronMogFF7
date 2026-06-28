#include "AppFrame/Application.h"

#include <chrono>
#include <thread>

namespace AppFrame
{
    int Application::run()
    {
        currentConfig = configure();
        currentConfig.styleCallback = [this]() { applyStyle(); };

        if (!gui.initialize(currentConfig))
        {
            return 1;
        }

        gui.onKeyPress.addListener([this](int key, int mods) { onKeyPress(key, mods); });
        gui.onResize.addListener([this](int width, int height)
        {
            onResize(width, height);
            if (initialized && currentConfig.redrawOnResize)
            {
                renderFrame(false);
            }
        });

        if (!onInitialize())
        {
            onShutdown();
            gui.destroy();
            return 1;
        }
        initialized = true;

        while (!gui.wasWindowClosed())
        {
            renderFrame(true);

            onAfterFrame();

            if (currentConfig.frameDelayMS > 0.0)
            {
                std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(currentConfig.frameDelayMS));
            }
        }

        initialized = false;
        onShutdown();
        gui.destroy();

        return 0;
    }

    void Application::renderFrame(bool pollEvents)
    {
        if (gui.beginFrame(pollEvents))
        {
            onFrame();
            gui.endFrame();
        }
    }

    void Application::applyStyle()
    {
    }
}
