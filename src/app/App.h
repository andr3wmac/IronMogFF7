#pragma once

#include "core/game/GameManager.h"
#include "core/gui/GUI.h"

#include <atomic>
#include <thread>

#define APP_WINDOW_WIDTH 497
#define APP_WINDOW_HEIGHT 632
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 6
#define APP_VERSION_PATCH 0
#define APP_VERSION_STRING "v0.6.0"

class App
{
public:
    enum Panels : uint8_t
    {
        Settings    = 0,
        Tracker     = 1,
        Debug       = 2
    };

    enum EmulatorType : uint8_t
    {
        DuckStation = 0,
        BizHawk     = 1,
        Custom      = 2
    };

    enum ConnectionState : uint8_t
    {
        NotConnected = 0,
        Connecting   = 1,
        Connected    = 2,
        Error        = 3
    };

    void run();
    void generateSeed();

    void drawSettingsPanel();
    void drawTrackerPanel();
    void drawBottomPanel();
    void drawDebugPanel();

    void connected();
    void disconnect();
    void runGameManager();

protected:
    GUI gui;
    GUIImage logo;
    std::vector<GUIImage> characterPortraits;
    GUIImage deadIcon;
    Panels currentPanel = Panels::Settings;

    GameManager* game = nullptr;
    std::thread* managerThread = nullptr;
    std::atomic<bool> managerRunning = false;

    EmulatorType selectedEmulatorType = EmulatorType::DuckStation;

    std::vector<std::string> runningProcesses;
    std::vector<const char*> runningProcessesCStr;
    int selectedProcessIdx = 0;
    char processMemoryOffset[20];
    char seedValue[9];

    ConnectionState connectionState = ConnectionState::NotConnected;
    std::string connectionStatus = "Not Connected";

    void onKeyPress(int key, int mods);
    void onStart();

    // Debug Panel variables
    char debugWarpFieldID[5];
};