#pragma once

#include "app/Tracker.h"
#include "core/game/GameManager.h"
#include "core/gui/GUI.h"
#include "core/utilities/StringList.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#define APP_WINDOW_WIDTH 497
#define APP_WINDOW_HEIGHT 665
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 8
#define APP_VERSION_PATCH 3
#define APP_VERSION_STRING "v0.8.3"
#define APP_SETTINGS_FOLDER "settings"

class App
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

    void run();
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
    void stopGameManager();

protected:
    // Everything the manager thread needs to connect, resolved on the GUI thread before it starts.
    struct ConnectionTarget
    {
        EmulatorType emulatorType = EmulatorType::DuckStation;
        std::string processName;
        uintptr_t memoryAddress = 0;
    };

    // Creates and sets up the GameManager on the GUI thread, then starts the manager thread to connect
    // and run it. Returns false (and reports an error status) if the connection settings are invalid.
    bool startGameManager();
    void runGameManager(ConnectionTarget target);

    void setConnectionStatus(ConnectionState state, const std::string& status);
    std::string getConnectionStatus();

    GUI gui;
    GUIImage logo;
    Tracker tracker;
    std::vector<GUIImage> characterPortraits;
    GUIImage deadIcon;
    bool showDebugTab = false;

    // Setup
    // The GameManager is created, set up, and deleted on the GUI thread only. The manager thread
    // connects and runs updates in between.
    GameManager* game = nullptr;
    std::thread* managerThread = nullptr;
    std::atomic<bool> managerRunning = false;
    std::atomic<bool> stopRequested = false;
    GameManager::GameState previousState = GameManager::GameState::BootScreen;

    GameVersion selectedGameVersion = GameVersion::PlayStationUS;
    EmulatorType selectedEmulatorType = EmulatorType::DuckStation;

    StringList availableSettings;
    int selectedSettingsIdx = 0;

    StringList runningProcesses;
    int selectedProcessIdx = 0;
    char processMemoryOffset[20];
    char seedValue[9];

    // Written by both threads. connectionStatus is guarded by connectionStatusMutex.
    std::atomic<ConnectionState> connectionState = ConnectionState::NotConnected;
    std::mutex connectionStatusMutex;
    std::string connectionStatus = "Not Connected";

    // The seed can change on the manager thread (loaded from a save), it's handed to the GUI through these.
    std::atomic<uint32_t> pendingSeed = 0;
    std::atomic<bool> seedPending = false;

    void onKeyPress(int key, int mods);
    void onResize(int width, int height);
    void onStart();

    void guiSettingsRead(const char* section, const char* line);
    void guiSettingsWrite(ImGuiTextBuffer* buf);
};