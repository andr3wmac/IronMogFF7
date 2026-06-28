#pragma once

#include "app/Tracker.h"
#include "AppFrame/Application.h"
#include "LiveModFF7/game/GameManager.h"
#include "utilities/StringList.h"

#include <atomic>
#include <thread>

#define APP_WINDOW_WIDTH 497
#define APP_WINDOW_HEIGHT 665
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 8
#define APP_VERSION_PATCH 2
#define APP_VERSION_STRING "v0.8.2"
#define APP_SETTINGS_FOLDER "settings"

class App : public AppFrame::Application
{
public:
    enum class EmulatorType : uint8_t
    {
        DuckStation = 0,
        BizHawk     = 1,
        Custom      = 2
    };

    enum class ConnectionState : uint8_t
    {
        NotConnected = 0,
        Connecting   = 1,
        Connected    = 2,
        Error        = 3
    };

    void generateSeed();
    void scanSettings(std::string settingsFolder, std::string loadIfAvailable = "Default");
    void loadSettings(const std::string& filePath);
    void saveSettings(const std::string& filePath, bool saveSeed = false);

    void draw();
    void drawSetupPanel();
    void drawTrackerPanel();
    void drawAppSettingsPanel();
    void drawDebugPanel();

    void connect();
    void disconnect();
    void reconnect();
    void runGameManager();
    void stopGameManager();

protected:
    AppFrame::GUIImage logo;
    Tracker tracker;
    std::vector<AppFrame::GUIImage> characterPortraits;
    AppFrame::GUIImage deadIcon;
    bool showDebugTab = false;

    // Setup
    GameManager* game = nullptr;
    std::thread* managerThread = nullptr;
    std::atomic<bool> managerRunning = false;
    GameManager::GameState previousState = GameManager::GameState::BootScreen;

    GameVersion selectedGameVersion = GameVersion::PlayStationUS;
    EmulatorType selectedEmulatorType = EmulatorType::DuckStation;

    StringList availableSettings;
    int selectedSettingsIdx = 0;

    StringList runningProcesses;
    int selectedProcessIdx = 0;
    char processMemoryOffset[20];
    char seedValue[9];

    ConnectionState connectionState = ConnectionState::NotConnected;
    std::string connectionStatus = "Not Connected";

    AppFrame::AppConfig configure() const override;
    bool onInitialize() override;
    void onShutdown() override;
    void onFrame() override;
    void onAfterFrame() override;
    void onKeyPress(int key, int mods) override;
    void onResize(int width, int height) override;
    void onStart();

    void guiSettingsRead(const char* section, const char* line);
    void guiSettingsWrite(ImGuiTextBuffer* buf);
};
