#pragma once

#include "AppFrame/GUI.h"

#include <functional>
#include <string>

namespace AppFrame
{
    struct AppConfig
    {
        int windowWidth = 800;
        int windowHeight = 600;
        std::string windowTitle = "AppFrame";
        std::string iniFilename = "settings/app.ini";
        std::string windowIconPath;
        double frameDelayMS = 16.67;
        bool enableKeyboardNavigation = true;
        bool enableIconFont = true;
        bool redrawOnResize = true;
        bool lockHorizontalResize = false;
        std::string iconFontPath = "lib/AppFrame/resources/fa-solid-900.ttf";
        std::vector<FontSpec> fonts;
        std::function<void()> styleCallback;
    };

    class Application
    {
    public:
        virtual ~Application() = default;

        int run();

    protected:
        GUI gui;

        virtual AppConfig configure() const = 0;
        virtual bool onInitialize() { return true; }
        virtual void onShutdown() {}
        virtual void onFrame() = 0;
        virtual void onAfterFrame() {}
        virtual void onKeyPress(int key, int mods) {}
        virtual void onResize(int width, int height) {}
        virtual void applyStyle();

    private:
        AppConfig currentConfig;
        bool initialized = false;

        void renderFrame(bool pollEvents);
    };

    template<typename TApplication>
    int runApplication()
    {
        TApplication app;
        return app.run();
    }
}
