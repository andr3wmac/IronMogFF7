#include "App.h"
#include "core/audio/AudioManager.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/ConfigFile.h"
#include "core/utilities/MemoryMonitor.h"
#include "core/utilities/MemorySearch.h"
#include "core/utilities/ModelEditor.h"
#include "core/utilities/Platform.h"
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

    // We embed the app settings in the same app.ini that ImGui uses.
    GUI::registerSettingsHandler("IronMogFF7",
        [this](const char* section, const char* line) { this->guiSettingsRead(section, line); },
        [this](ImGuiTextBuffer* buf) { this->guiSettingsWrite(buf); }
    );

    if (!gui.initialize(APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, "IronMog FF7 " APP_VERSION_STRING))
    {
        LOG("Graphics failure: could not initialize GUI.");
        return;
    }

    Platform::initialize();

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
    scanSettings(APP_SETTINGS_FOLDER, "Default");

    while (true)
    {
        if (gui.wasWindowClosed())
        {
            break;
        }

        // Pick up a seed change made on the manager thread (e.g. the seed stored in a loaded save).
        if (seedPending.exchange(false))
        {
            snprintf(seedValue, sizeof(seedValue), "%08X", pendingSeed.load());
        }

        draw();

        // Check to see if the game manager thread exited from an error and clean up.
        if (connectionState == ConnectionState::Error && !managerRunning && managerThread != nullptr)
        {
            stopGameManager();
            AudioManager::pauseMusic();
        }

        // Run the GUI at 60fps
        Platform::sleep(16.67);
    }

    gui.destroy();
    Platform::shutdown();
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

    startGameManager();
}

void App::disconnect()
{
    setConnectionStatus(ConnectionState::NotConnected, "Not Connected");

    if (managerThread == nullptr)
    {
        return;
    }

    stopGameManager();
    AudioManager::pauseMusic();
}

void App::reconnect()
{
    stopGameManager();
    startGameManager();
}

bool App::startGameManager()
{
    // Resolve where we're connecting to up front so bad input is reported here rather than crashing the manager thread.
    ConnectionTarget target;
    target.emulatorType = selectedEmulatorType;

    std::string emulatorName;
    if (selectedEmulatorType == EmulatorType::DuckStation)
    {
        emulatorName = "DuckStation";
        target.processName = "duckstation-qt-x64-ReleaseLTCG.exe";
    }
    if (selectedEmulatorType == EmulatorType::BizHawk)
    {
        emulatorName = "BizHawk";
        target.processName = "EmuHawk.exe";
    }
    if (selectedEmulatorType == EmulatorType::Custom)
    {
        if (selectedProcessIdx < 0 || selectedProcessIdx >= (int)runningProcesses.size())
        {
            setConnectionStatus(ConnectionState::Error, "Select an emulator process.");
            return false;
        }

        if (!Utilities::tryParseAddress(processMemoryOffset, target.memoryAddress))
        {
            setConnectionStatus(ConnectionState::Error, "Invalid memory offset.");
            return false;
        }

        emulatorName = runningProcesses[selectedProcessIdx];
        target.processName = runningProcesses[selectedProcessIdx];
    }

    // Reset any global restrictions as we might be using a different set of rules on this run.
    Restrictions::reset();

    // Setup doesn't touch emulator memory, so it's done here on the GUI thread. That keeps every rule
    // and extra's manager pointer owned by this thread and never changing while the GUI reads it.
    game = new GameManager();
    BIND_EVENT(game->onStart, App::onStart);
    game->setup(selectedGameVersion, Utilities::hexStringToSeed(seedValue));
    tracker.setup(game);

    setConnectionStatus(ConnectionState::Connecting, "Connecting to " + emulatorName + "..");

    stopRequested = false;
    managerRunning = true;
    managerThread = new std::thread(&App::runGameManager, this, target);
    return true;
}

void App::runGameManager(ConnectionTarget target)
{
    bool connected = false;
    if (target.emulatorType == EmulatorType::Custom)
    {
        connected = game->connectToEmulator(target.processName, target.memoryAddress);
    }
    else
    {
        connected = game->connectToEmulator(target.processName);
    }

    if (!connected)
    {
        setConnectionStatus(ConnectionState::Error, "Failed to connect to emulator.");
        managerRunning = false;
        return;
    }

    setConnectionStatus(ConnectionState::Connected, "Connected to emulator.");

    while (!stopRequested.load())
    {
        if (!game->update())
        {
            // If update returns false then a fatal error occurred.
            setConnectionStatus(ConnectionState::Error, "Connection lost.");
            break;
        }

        // Sleep longer if the emulator is paused so we lower our CPU usage.
        if (game->isPaused())
        {
            Platform::sleep(16.67);
        }
        else
        {
            Platform::sleep(1.0);
        }
    }
    managerRunning = false;
}

void App::stopGameManager()
{
    stopRequested = true;
    if (managerThread != nullptr)
    {
        managerThread->join();
        delete managerThread;
        managerThread = nullptr;
    }
    managerRunning = false;

    // The manager thread is gone, so the GameManager can be safely torn down here on the GUI thread.
    tracker.reset();
    delete game;
    game = nullptr;

    previousState = GameManager::GameState::BootScreen;
}

void App::setConnectionStatus(ConnectionState state, const std::string& status)
{
    std::lock_guard<std::mutex> lock(connectionStatusMutex);
    connectionStatus = status;
    connectionState = state;
}

std::string App::getConnectionStatus()
{
    std::lock_guard<std::mutex> lock(connectionStatusMutex);
    return connectionStatus;
}

void App::generateSeed()
{
    std::random_device rd;
    uint32_t seed = (static_cast<uint32_t>(rd()) << 16) ^ rd();
    snprintf(seedValue, sizeof(seedValue), "%08X", seed);
    LOG("Seed generated: %s", seedValue);
}

void App::scanSettings(std::string settingsFolder, std::string loadIfAvailable)
{
    availableSettings.clear();
    selectedSettingsIdx = 0;

    // Always first in the list so we can switch to it when settings are changed.
    availableSettings.push_back("Custom");

    if (fs::exists(settingsFolder) && fs::is_directory(settingsFolder))
    {
        for (const auto& entry : fs::directory_iterator(settingsFolder))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".cfg")
            {
                availableSettings.push_back(entry.path().stem().string());
            }
        }
    }

    for (int i = 0; i < availableSettings.size(); ++i)
    {
        if (availableSettings[i] == loadIfAvailable)
        {
            selectedSettingsIdx = i;
            loadSettings(settingsFolder + "/" + availableSettings[i] + ".cfg");
        }
    }
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
        showDebugTab = true;
    }
}

void App::onResize(int width, int height)
{
    // This makes the UI redraw as we're resizing so it looks nice and smooth.
    draw();
}

void App::onStart()
{
    // Runs on the manager thread, the GUI thread applies it to seedValue.
    pendingSeed = game->getSeed();
    seedPending = true;
}

void App::guiSettingsRead(const char* section, const char* line)
{
    auto readInt = [&](const char* key, int* out) -> bool 
    {
        char fmt[64];
        snprintf(fmt, sizeof(fmt), "%s=%%d", key);
        return sscanf(line, fmt, out) == 1;
    };

    auto readBool = [&](const char* key, bool* out) -> bool 
    {
        int val;
        if (readInt(key, &val)) { *out = (val != 0); return true; }
        return false;
    };

    if (strcmp(section, "Tracker") == 0)
    {
        if (readBool("ShowLogo", &tracker.showLogo)) return;
        if (readBool("ShowCharacters", &tracker.showCharacters)) return;
        if (readBool("ShowSeed", &tracker.showSeed)) return;
        if (readBool("ShowTime", &tracker.showTime)) return;
        if (readBool("ShowSong", &tracker.showSong)) return;
        if (readBool("ShowRuleSummary", &tracker.showRuleSummary)) return;

        int attemptsDisplayMode = 0;
        if (readInt("AttemptsDisplayMode", &attemptsDisplayMode))
        {
            tracker.attemptsDisplayMode = (AttemptsDisplayMode)attemptsDisplayMode;
            return;
        }
        int counter = 0;
        if (readInt("Attempts", &counter))
        {
            tracker.attemptCounter = counter;
            return;
        }
        if (readInt("GameOvers", &counter))
        {
            tracker.gameOverCounter = counter;
            return;
        }
    }
}

void App::guiSettingsWrite(ImGuiTextBuffer* buf)
{
    buf->appendf("[%s][%s]\n", "IronMogFF7", "Tracker");
    buf->appendf("ShowLogo=%d\n", tracker.showLogo ? 1 : 0);
    buf->appendf("ShowCharacters=%d\n", tracker.showCharacters ? 1 : 0);
    buf->appendf("ShowSeed=%d\n", tracker.showSeed ? 1 : 0);
    buf->appendf("ShowTime=%d\n", tracker.showTime ? 1 : 0);
    buf->appendf("ShowSong=%d\n", tracker.showSong ? 1 : 0);
    buf->appendf("ShowRuleSummary=%d\n", tracker.showRuleSummary ? 1 : 0);
    buf->appendf("AttemptsDisplayMode=%d\n", (int)tracker.attemptsDisplayMode);
    buf->appendf("Attempts=%d\n", tracker.attemptCounter.load());
    buf->appendf("GameOvers=%d\n", tracker.gameOverCounter.load());
    buf->append("\n");
}