# AppFrame

AppFrame is a small cross-platform desktop application frame for C++. It owns the window, ImGui backend setup, frame loop, DPI scaling, fonts, image loading, native file dialogs, and cross-platform entry-point plumbing so individual apps can focus on their own UI and behavior.

## Included Libraries

AppFrame vendors and wraps:

- Dear ImGui
- GLFW
- nativefiledialog-extended
- stb image
- Font Awesome icon definitions and font asset

The vendor code lives under `lib/`. Public AppFrame headers live under `include/AppFrame/`.

## Features

- Public version constants through `include/AppFrame/AppFrame.h`.
- `AppFrame::Application` lifecycle with `configure`, `onInitialize`, `onFrame`, `onShutdown`, `onResize`, and `onKeyPress` hooks.
- `APPFRAME_MAIN(AppType)` entry macro that expands to `wWinMain` on Windows and `main` elsewhere.
- ImGui + GLFW + OpenGL2 setup and shutdown.
- DPI helper via `AppFrame::DPI()` and compatibility `DPI()`.
- Font registration through `AppConfig::fonts`.
- Optional Font Awesome icon font merge.
- Window icon loading.
- Native open/save file dialogs.
- `GUIImage` texture loading from file or memory.
- ImGui `.ini` settings-handler helpers.
- Live redraw during window resize via `AppConfig::redrawOnResize`.
- Optional horizontal resize locking via `AppConfig::lockHorizontalResize` on Windows.
- Simple `ConfigFile`, `StringList`, and `Event` helpers.

AppFrame intentionally does not contain app-specific themes. Apps can customize style by overriding `Application::applyStyle()`.

## Minimal Usage

```cpp
#include "AppFrame/AppFrame.h"
#include "AppFrame/Application.h"
#include "AppFrame/EntryPoint.h"

class MyApp : public AppFrame::Application
{
protected:
    AppFrame::AppConfig configure() const override
    {
        AppFrame::AppConfig config;
        config.windowWidth = 800;
        config.windowHeight = 600;
        config.windowTitle = "My Tool";
        config.iniFilename = "settings/app.ini";
        config.fonts.push_back({ "Inter", "lib/AppFrame/resources/Inter_18pt-Regular.ttf", 18.0f });
        return config;
    }

    void onFrame() override
    {
        ImGui::Begin("My Tool");
        ImGui::TextUnformatted("Hello from AppFrame.");
        ImGui::End();
    }
};

APPFRAME_MAIN(MyApp)
```

## Demo Application

The repo includes `examples/AppFrameDemo`, a small executable that exercises the AppFrame runtime surface:

- custom font loading through `AppConfig::fonts`
- Font Awesome icon font merge
- `ImGui::ShowDemoWindow()`
- DPI/window-size reporting and resize/key callbacks
- `GUIImage` loading from memory
- native open/save file dialogs
- helper widgets such as color grids, centered text, and wrapped tooltips

Generate project files and run the `AppFrameDemo` project to verify the library integration.

## Premake Integration

When using the vendored release package, `premake5.lua` contains only the reusable AppFrame integration functions and can be loaded with `includeexternal`.

```lua
local APPFRAME_DIR = "./lib/AppFrame/"

includeexternal(APPFRAME_DIR)

project "MyTool"
    includedirs {
        "src/",
        path.join(APPFRAME_DIR, "include/"),
    }

    links { "AppFrame" }
    appFrameLinks(APPFRAME_DIR)

group "lib"

project "AppFrame"
    appFrameProject(APPFRAME_DIR)
```

When using this repository directly instead of the vendored package, include `premake/appframe.lua` from your consuming Premake script so the repo's demo workspace is not imported:

```lua
local APPFRAME_DIR = "./lib/AppFrame/"

include(path.join(APPFRAME_DIR, "premake/appframe.lua"))
```

## Release Packages

GitHub Actions produces two Windows x64 packages:

- `AppFrame-static-win-x64.zip`: headers, resources, `AppFrame.lib`, and the third-party headers/libs needed to link AppFrame into an application.
- `AppFrame-vendor.zip`: source, vendored third-party libraries, resources, and an integration-only `premake5.lua` for projects that vendor AppFrame and build it as part of their own workspace.

The vendor package intentionally excludes `tools/premake5.exe`, generated build files, and the demo application.
