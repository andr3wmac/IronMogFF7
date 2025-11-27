#include "App.h"
#include "core/audio/AudioManager.h"
#include "core/utilities/Logging.h"
#include "core/utilities/MemoryMonitor.h"
#include "core/utilities/MemorySearch.h"
#include "core/utilities/ModelEditor.h"
#include "core/utilities/ScriptUtilities.h"
#include "core/utilities/Utilities.h"
#include "rules/Restrictions.h"

#include <imgui.h>
#include <random>

void App::run()
{
    processMemoryOffset[0] = '\0';
    debugWarpFieldID[0] = '\0';

    gui.initialize(APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, "IronMog FF7 " APP_VERSION_STRING);
    BIND_EVENT_TWO_ARG(gui.onKeyPress, App::onKeyPress);
    generateSeed();

    // Load images
    logo.loadFromFile("img/logo.png");
    characterPortraits.resize(9);
    characterPortraits[0].loadFromFile("img/cloud.png");
    characterPortraits[1].loadFromFile("img/barret.png");
    characterPortraits[2].loadFromFile("img/tifa.png");
    characterPortraits[3].loadFromFile("img/aerith.png");
    characterPortraits[4].loadFromFile("img/red.png");
    characterPortraits[5].loadFromFile("img/yuffie.png");
    characterPortraits[6].loadFromFile("img/caitsith.png");
    characterPortraits[7].loadFromFile("img/vincent.png");
    characterPortraits[8].loadFromFile("img/cid.png");

    deadIcon.loadFromFile("img/dead.png");

    while (true)
    {
        if (gui.wasWindowClosed())
        {
            break;
        }

        if (!gui.beginFrame())
        {
            continue;
        }

        ImGui::Begin("IronMogFF7", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        {
            switch (currentPanel)
            {
                case Panels::Settings:
                    drawSettingsPanel();
                    break;

                case Panels::Tracker:
                    drawTrackerPanel();
                    break;

                case Panels::Debug:
                    drawDebugPanel();
                    break;
            }
        }
        ImGui::End();

        gui.endFrame();
    }

    gui.destroy();
}

void App::connected()
{
    if (managerThread != nullptr)
    {
        if (connectionState == ConnectionState::Error)
        {
            managerRunning = false;
            managerThread->join();
            delete managerThread;
            managerThread = nullptr;
        }
        else 
        {
            return;
        }
    }

    managerThread = new std::thread(&App::runGameManager, this);
}

void App::disconnect()
{
    connectionState = ConnectionState::NotConnected;
    connectionStatus = "Not Connected";

    if (managerThread == nullptr || !managerRunning.load())
    {
        return;
    }

    managerRunning = false;
    managerThread->join();
    delete managerThread;
    managerThread = nullptr;

    AudioManager::pauseMusic();
}

void App::runGameManager()
{
    if (game != nullptr)
    {
        delete game;
        game = nullptr;
    }

    game = new GameManager();
    BIND_EVENT(game->onStart, App::onStart);

    connectionState = ConnectionState::Connecting;
    connectionStatus = "Connecting to Emulator..";

    bool connected = false;

    if (selectedEmulatorType == EmulatorType::DuckStation)
    {
        connectionStatus = "Connecting to DuckStation..";
        std::string targetProcess = "duckstation-qt-x64-ReleaseLTCG.exe";
        connected = game->connectToEmulator(targetProcess);
    }
    if (selectedEmulatorType == EmulatorType::BizHawk)
    {
        connectionStatus = "Connecting to BizHawk..";
        std::string targetProcess = "EmuHawk.exe";
        connected = game->connectToEmulator(targetProcess);
    }
    if (selectedEmulatorType == EmulatorType::Custom)
    {
        uintptr_t customAddress = Utilities::parseAddress(processMemoryOffset);
        connected = game->connectToEmulator(runningProcesses[selectedProcessIdx], customAddress);
    }

    if (connected)
    {
        connectionState = ConnectionState::Connected;
        connectionStatus = "Connected to emulator.";
    }
    else
    {
        connectionState = ConnectionState::Error;
        connectionStatus = "Failed to connect to emulator.";
        return;
    }

    // Reset any global restrictions as we might be using a different set of rules on this run.
    Restrictions::reset();

    game->setup(Utilities::hexStringToSeed(seedValue));

    managerRunning = true;
    while (managerRunning.load())
    {
        game->update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void App::generateSeed()
{
    std::random_device rd;
    uint32_t seed = (static_cast<uint32_t>(rd()) << 16) ^ rd();
    snprintf(seedValue, sizeof(seedValue), "%08X", seed);
    LOG("Seed generated: %s", seedValue);
}

void App::onKeyPress(int key, int mods)
{
    // Ctrl + D
    if (key == 68 && (mods & 2))
    {
        if (currentPanel == Panels::Settings)
        {
            currentPanel = Panels::Debug;
        }
        else if (currentPanel == Panels::Debug)
        {
            currentPanel = Panels::Settings;
        }
    }
}

void App::onStart()
{
    uint32_t chosenSeed = game->getSeed();
    snprintf(seedValue, 9, "%08X", chosenSeed);
}