#pragma once

#include "app/Tracker.h"
#include "AppFrame/Application.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "utilities/StringList.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#define APP_NAME "LiveMod FF7"
#define APP_WINDOW_WIDTH 1000
#define APP_WINDOW_HEIGHT 640
#define APP_WINDOW_MIN_WIDTH 820
#define APP_WINDOW_MIN_HEIGHT 480
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 8
#define APP_VERSION_PATCH 4
#define APP_VERSION_STRING "v0.8.4"
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

    enum class SetupPage : uint8_t
    {
        General = 0,
        Mod     = 1
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
    void openSettingsFile();
    void saveSettingsFileAs();

    void draw();
    void drawMenuBar();
    void drawLogo();
    void drawHeader();
    void drawAboutPopup();
    void drawSetupPanel();
    void drawSetupGeneral(bool lockSettings);
    void drawTrackerPanel();
    void drawTrackerOptions();
    void drawPreferencesPanel();
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

    // Reconnects when the game goes from the main menu to in game, so settings changed on the main menu apply.
    void checkForGameStart();

    void setConnectionStatus(ConnectionState state, const std::string& status);
    std::string getConnectionStatus();

    AppFrame::GUIImage logo;
    Tracker tracker;
    std::vector<AppFrame::GUIImage> characterPortraits;
    AppFrame::GUIImage deadIcon;
    float accentColor[3] = { 80.0f / 255.0f, 1.0f, 140.0f / 255.0f };
    bool showDebugTab = false;
    bool showPreferencesTab = false;
    bool selectPreferencesTab = false;
    bool openAboutPopup = false;
    SetupPage selectedSetupPage = SetupPage::General;
    int selectedSetupIndex = 0;

    // Setup
    // The GameManager is created, set up, and deleted on the GUI thread only. The manager thread
    // connects and runs updates in between.
    GameManager* game = nullptr;
    std::thread* managerThread = nullptr;
    std::atomic<bool> managerRunning = false;
    std::atomic<bool> stopRequested = false;
    // Last game state seen by checkForGameStart, empty until the first one after connecting.
    std::optional<GameManager::GameState> previousState;

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

    AppFrame::AppConfig configure() const override;
    bool onInitialize() override;
    void onShutdown() override;
    void onFrame() override;
    void onAfterFrame() override;
    void onKeyPress(int key, int mods) override;
    void onResize(int width, int height) override;
    void applyStyle() override;
    void updateAccentColors();
    void onStart();

    void guiSettingsRead(const char* section, const char* line);
    void guiSettingsWrite(ImGuiTextBuffer* buf);
};
