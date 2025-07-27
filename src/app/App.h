#pragma once

#include "core/game/GameManager.h"
#include "core/gui/GUI.h"

#include <atomic>
#include <thread>

class App
{
public:
    void run();
    void generateSeed();

    void drawMainPanel();
    void drawStatusPanel();
    void drawDebugPanel();

    void attach();
    void detach();
    void runGameManager();

protected:
    GUI gui;
    GUIImage logo;
    int currentPanel = 0;

    GameManager* game = nullptr;
    std::thread* managerThread = nullptr;
    std::atomic<bool> managerRunning = false;

    int selectedEmulatorIdx = 0;

    std::vector<std::string> runningProcesses;
    std::vector<const char*> runningProcessesCStr;
    int selectedProcessIdx = 0;
    char processMemoryOffset[20];
    char seedValue[9];

    int connectionState = 0;
    std::string connectionStatus = "Not Attached";

    void onKeyPress(int key, int mods);
    void onStart();
};