#include "App.h"
#include "core/audio/AudioManager.h"
#include "core/utilities/Logging.h"
#include "core/utilities/ConfigFile.h"
#include "core/utilities/MemoryMonitor.h"
#include "core/utilities/MemorySearch.h"
#include "core/utilities/ModelEditor.h"
#include "core/utilities/ScriptUtilities.h"
#include "core/utilities/Utilities.h"
#include "extras/Extra.h"
#include "rules/Restrictions.h"
#include "rules/Rule.h"

#include <imgui.h>
#include <random>

#include <filesystem>
namespace fs = std::filesystem;

void App::run()
{
    LOG("IronMog FF7 %s", APP_VERSION_STRING);

    processMemoryOffset[0] = '\0';

    if (!gui.initialize(APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, "IronMog FF7 " APP_VERSION_STRING))
    {
        LOG("Graphics failure: could not initialize GUI.");
        return;
    }

    BIND_EVENT_TWO_ARG(gui.onKeyPress, App::onKeyPress);
    BIND_EVENT_TWO_ARG(gui.onResize, App::onResize);
    generateSeed();

    // Load images
    logo.loadFromFile("resources/logo.png");
    characterPortraits.resize(9);
    characterPortraits[0].loadFromFile("resources/cloud.png");
    characterPortraits[1].loadFromFile("resources/barret.png");
    characterPortraits[2].loadFromFile("resources/tifa.png");
    characterPortraits[3].loadFromFile("resources/aerith.png");
    characterPortraits[4].loadFromFile("resources/red.png");
    characterPortraits[5].loadFromFile("resources/yuffie.png");
    characterPortraits[6].loadFromFile("resources/caitsith.png");
    characterPortraits[7].loadFromFile("resources/vincent.png");
    characterPortraits[8].loadFromFile("resources/cid.png");

    deadIcon.loadFromFile("resources/dead.png");

    // Load any settings files
    {
        const std::string settingsDir = "settings";

        // Always first in the list so we can switch to it when settings are changed.
        availableSettings.push_back("Custom");

        // Special case for default settings file
        if (fs::exists(settingsDir + "/Default.cfg"))
        {
            availableSettings.push_back("Default");
            selectedSettingsIdx = 1;
            loadSettings(settingsDir + "/Default.cfg");
        }

        if (fs::exists(settingsDir) && fs::is_directory(settingsDir))
        {
            for (const auto& entry : fs::directory_iterator(settingsDir))
            {
                if (entry.path().stem() == "Default")
                {
                    continue;
                }

                if (entry.is_regular_file() && entry.path().extension() == ".cfg")
                {
                    availableSettings.push_back(entry.path().stem().string());
                }
            }
        }
    }

    while (true)
    {
        if (gui.wasWindowClosed())
        {
            break;
        }

        draw();

        // Check to see if the game manager thread exited from an error and clean up.
        if (connectionState == ConnectionState::Error && !managerRunning && managerThread != nullptr)
        {
            stopGameManager();
            AudioManager::pauseMusic();
        }
    }

    gui.destroy();
}

void App::connect()
{
    if (managerThread != nullptr)
    {
        if (connectionState == ConnectionState::Error)
        {
            stopGameManager();
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

    stopGameManager();
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

    // Connect
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
        if (!game->update())
        {
            // If update returns false then a fatal error occured.
            connectionState = ConnectionState::Error;
            connectionStatus = "Connection lost.";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    managerRunning = false;
}

void App::stopGameManager()
{
    managerRunning = false;
    managerThread->join();
    delete managerThread;
    managerThread = nullptr;
}

void App::generateSeed()
{
    std::random_device rd;
    uint32_t seed = (static_cast<uint32_t>(rd()) << 16) ^ rd();
    snprintf(seedValue, sizeof(seedValue), "%08X", seed);
    LOG("Seed generated: %s", seedValue);
}

void App::loadSettings(const std::string& filePath)
{
    ConfigFile cfg;

    if (cfg.load(filePath))
    {
        LOG("Loaded settings from: %s", filePath.c_str());

        std::string seedStr = cfg.get<std::string>("seed", seedValue);
        snprintf(seedValue, sizeof(seedValue), "%s", seedStr.c_str());

        for (auto& rule : Rule::getList())
        {
            cfg.keyPrefix = Utilities::sanitizeName(rule->name) + ".";
            rule->loadSettings(cfg);
            rule->enabled = cfg.get<bool>("enabled", rule->enabled);
            cfg.keyPrefix = "";

        }
        for (auto& extra : Extra::getList())
        {
            cfg.keyPrefix = Utilities::sanitizeName(extra->name) + ".";
            extra->loadSettings(cfg);
            extra->enabled = cfg.get<bool>("enabled", extra->enabled);
            cfg.keyPrefix = "";
        }
    }
}

void App::saveSettings(const std::string& filePath, bool saveSeed)
{
    ConfigFile cfg;

    if (saveSeed)
    {
        std::string seedStr(seedValue);
        cfg.set<std::string>("seed", seedStr);
    }

    for (auto& rule : Rule::getList())
    {
        std::string name = Utilities::sanitizeName(rule->name);
        cfg.set<bool>(name + ".enabled", rule->enabled);
        cfg.keyPrefix = name + ".";
        rule->saveSettings(cfg);
        cfg.keyPrefix = "";
    }
    for (auto& extra : Extra::getList())
    {
        std::string name = Utilities::sanitizeName(extra->name);
        cfg.set<bool>(name + ".enabled", extra->enabled);
        cfg.keyPrefix = name + ".";
        extra->saveSettings(cfg);
        cfg.keyPrefix = "";
    }

    cfg.save(filePath);
    LOG("Saved settings to: %s", filePath.c_str());
}

void App::onKeyPress(int key, int mods)
{
    // Ctrl + D
    if (key == 68 && (mods & 2))
    {
        if (currentPanel == Panels::Settings || currentPanel == Panels::Tracker)
        {
            currentPanel = Panels::Debug;
        }
        else if (currentPanel == Panels::Debug)
        {
            currentPanel = Panels::Settings;
        }
    }
}

void App::onResize(int width, int height)
{
    // This makes the UI redraw as we're resizing so it looks nice and smooth.
    draw();
}

void App::onStart()
{
    uint32_t chosenSeed = game->getSeed();
    snprintf(seedValue, 9, "%08X", chosenSeed);
}