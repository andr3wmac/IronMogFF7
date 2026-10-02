#include "App.h"
#include "app/audio/AudioManager.h"
#include "app/ModManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "LiveModFF7Core/tools/MemoryMonitor.h"
#include "LiveModFF7Core/tools/MemorySearch.h"
#include "LiveModFF7Core/tools/ModelEditor.h"
#include "LiveModFF7Core/utilities/Platform.h"
#include "LiveModFF7Core/tools/ScriptUtilities.h"
#include "LiveModFF7Core/utilities/Utilities.h"
#include "mods/Restrictions.h"
#include "mods/Mod.h"
#include "utilities/ConfigFile.h"
#include "utilities/Randomizer.h"

#include "AppFrame/AppFrame.h"
#include <random>

#include <filesystem>
namespace fs = std::filesystem;

using GUI = AppFrame::GUI;

AppFrame::AppConfig App::configure() const
{
    AppFrame::AppConfig config;
    config.windowWidth = APP_WINDOW_WIDTH;
    config.windowHeight = APP_WINDOW_HEIGHT;
    config.minWindowWidth = APP_WINDOW_MIN_WIDTH;
    config.minWindowHeight = APP_WINDOW_MIN_HEIGHT;
    config.windowTitle = APP_NAME " " APP_VERSION_STRING;
    config.iniFilename = "settings/app.ini";
    config.windowIconPath = "resources/icon.png";
    config.defaultFont = { "Inter", "resources/Inter_18pt-Regular.ttf", 15.0f };
    config.fonts.push_back({ "Reactor7", "resources/Reactor7.ttf", 18.0f });
    config.iconFontPath = "resources/fa-solid-900.ttf";
    return config;
}

void App::applyStyle()
{
    // Roomier spacing and soft rounding to suit the Inter font. Values are pre-DPI, AppFrame scales them afterwards.
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(10.0f, 6.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.CellPadding       = ImVec2(8.0f, 4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;

    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;

    updateAccentColors();
}

void App::updateAccentColors()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    const ImVec4 accent(accentColor[0], accentColor[1], accentColor[2], 1.0f);
    auto shade = [this](float brightness, float alpha = 1.0f)
    {
        return ImVec4(accentColor[0] * brightness, accentColor[1] * brightness, accentColor[2] * brightness, alpha);
    };

    colors[ImGuiCol_FrameBgHovered]      = shade(1.00f, 0.30f);
    colors[ImGuiCol_FrameBgActive]       = shade(0.47f);
    colors[ImGuiCol_TitleBgActive]       = shade(0.31f);
    colors[ImGuiCol_CheckMark]           = accent;
    colors[ImGuiCol_SliderGrab]          = shade(0.78f, 0.70f);
    colors[ImGuiCol_SliderGrabActive]    = accent;
    colors[ImGuiCol_ButtonHovered]       = shade(1.00f, 0.25f);
    colors[ImGuiCol_ButtonActive]        = shade(0.47f);
    colors[ImGuiCol_Header]              = shade(0.38f, 0.55f);
    colors[ImGuiCol_HeaderHovered]       = shade(0.53f, 0.70f);
    colors[ImGuiCol_HeaderActive]        = shade(0.47f, 0.85f);
    colors[ImGuiCol_SeparatorHovered]    = shade(0.78f, 0.78f);
    colors[ImGuiCol_SeparatorActive]     = accent;
    colors[ImGuiCol_TabHovered]          = shade(0.53f, 0.75f);
    colors[ImGuiCol_Tab]                 = shade(0.25f, 0.85f);
    colors[ImGuiCol_TabSelected]         = shade(0.44f);
    colors[ImGuiCol_TabSelectedOverline] = accent;
    colors[ImGuiCol_TabDimmedSelected]   = shade(0.30f);
    colors[ImGuiCol_TextLink]            = accent;
    colors[ImGuiCol_NavCursor]           = accent;
}

bool App::onInitialize()
{
    LOG(APP_NAME " %s", APP_VERSION_STRING);

    processMemoryOffset[0] = '\0';

    // We embed the app settings in the same app.ini that ImGui uses.
    // Note: the section keeps its original name so existing app.ini files still load.
    GUI::registerSettingsHandler("IronMogFF7",
        [this](const char* section, const char* line) { this->guiSettingsRead(section, line); },
        [this](ImGuiTextBuffer* buf) { this->guiSettingsWrite(buf); }
    );

    Platform::initialize();
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

    return true;
}

void App::onShutdown()
{
    // Stop the manager thread before anything it uses is torn down (GUI, statics, audio).
    stopGameManager();
    Platform::shutdown();
}

void App::onFrame()
{
    // Pick up a seed change made on the manager thread (e.g. the seed stored in a loaded save).
    if (seedPending.exchange(false))
    {
        snprintf(seedValue, sizeof(seedValue), "%08X", pendingSeed.load());
    }

    // Done outside of draw() so it happens regardless of which tab is visible.
    checkForGameStart();

    draw();
}

void App::onAfterFrame()
{
    // Check to see if the game manager thread exited from an error and clean up.
    if (connectionState == ConnectionState::Error && !managerRunning && managerThread != nullptr)
    {
        stopGameManager();
        AudioManager::pauseMusic();
    }
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

    // Reset global restrictions before applying the selected mods.
    Restrictions::reset();

    // Teach the Randomizer utility how to recognize a banned id. The utility has no concept of
    // our specific bans; this routes its excludeBanned checks back to our Restrictions set.
    Randomizer::setItemBanFilter(&Restrictions::isItemBanned);
    Randomizer::setMateriaBanFilter(&Restrictions::isMateriaBanned);

    // Setup doesn't touch emulator memory, so it's done here on the GUI thread. That keeps every
    // mod's manager pointer owned by this thread and never changing while the GUI reads it.
    game = new GameManager();
    BIND_EVENT(game->onStart, App::onStart);
    game->setup(selectedGameVersion, Utilities::hexStringToSeed(seedValue));
    tracker.setup(game);

    // Set up the mods after the engine so the seed is ready and their event
    // listeners are bound before the ban-enforcement listeners below.
    ModManager::setup(game);

    // Enforce item/materia bans by deleting anything banned that a randomizer (or
    // nothing) left in place. Bound after the mods so these listeners run last
    // on each event, ensuring we only remove what wasn't already replaced.
    game->onBattleEnter.addListener(this, "Restrictions::enforceBattleBans", [this]() { Restrictions::enforceBattleBans(game); });
    game->onBattleTransition.addListener(this, "Restrictions::enforceBattleBans", [this](uint16_t) { Restrictions::enforceBattleBans(game); });
    game->onFieldChanged.addListener(this, "Restrictions::enforceFieldBans", [this](uint16_t fieldID) { Restrictions::enforceFieldBans(game, fieldID); });
    game->onShopMenuChanged.addListener(this, "Restrictions::enforceShopBans", [this](uint8_t menuIndex) { Restrictions::enforceShopBans(game, menuIndex); });

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
    ModManager::shutdown(game);
    delete game;
    game = nullptr;

    previousState.reset();
}

void App::checkForGameStart()
{
    if (connectionState != ConnectionState::Connected || game == nullptr)
    {
        return;
    }

    GameManager::GameState state = game->getState();

    // The first state seen after connecting is only a baseline. Connecting while already in game has
    // just applied the current settings, so there's nothing to pick up and reconnecting could land mid-battle.
    if (previousState.has_value() && previousState != GameManager::GameState::InGame && state == GameManager::GameState::InGame)
    {
        // Save the current configuration in case of a crash, etc
        // We do not overwrite Last Settings if we're currently on Default. It's too common to press
        // Connect without thinking about it and then lose Last Settings in the process.
        if (availableSettings[selectedSettingsIdx] != "Default")
        {
            saveSettings("settings/Last Settings.cfg", true);
        }

        // Reconnect so any settings changed on the main menu are applied to this run.
        LOG("Detected game start, reconnecting GameManager..");
        reconnect();
        return;
    }

    previousState = state;
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

        for (auto& mod : Mod::getList())
        {
            cfg.keyPrefix = Utilities::sanitizeName(mod->name) + ".";
            mod->loadSettings(cfg);
            mod->enabled = cfg.get<bool>("enabled", mod->enabled);
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

    for (auto& mod : Mod::getList())
    {
        std::string name = Utilities::sanitizeName(mod->name);
        cfg.set<bool>(name + ".enabled", mod->enabled);
        cfg.keyPrefix = name + ".";
        mod->saveSettings(cfg);
        cfg.keyPrefix = "";
    }

    cfg.save(filePath);
    LOG("Saved settings to: %s", filePath.c_str());
}

void App::openSettingsFile()
{
    std::string openPath = gui.openFileDialog();
    if (openPath != "")
    {
        loadSettings(openPath);
        selectedSettingsIdx = 0;
    }
}

void App::saveSettingsFileAs()
{
    std::string savePath = gui.saveFileDialog();
    if (savePath != "")
    {
        saveSettings(savePath);

        if (Utilities::isFileInFolder(APP_SETTINGS_FOLDER, savePath))
        {
            std::string saveFileName = fs::path(savePath).stem().string();
            scanSettings(APP_SETTINGS_FOLDER, saveFileName);
        }
    }
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
    // AppFrame redraws during resize (redrawOnResize), so nothing to do here.
}

void App::onStart()
{
    // Runs on the manager thread, the GUI thread applies it to seedValue.
    pendingSeed = game->getSeed();
    seedPending = true;
}

void App::guiSettingsRead(const char* section, const char* line)
{
    if (strcmp(section, "Appearance") == 0 && strlen(line) == 19)
    {
        unsigned int red, green, blue;
        if (sscanf(line, "AccentColor=#%2x%2x%2x", &red, &green, &blue) == 3)
        {
            accentColor[0] = red / 255.0f;
            accentColor[1] = green / 255.0f;
            accentColor[2] = blue / 255.0f;
            updateAccentColors();
        }
        return;
    }

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
        if (readBool("ShowCharacters", &tracker.showCharacters)) return;
        if (readBool("ShowSeed", &tracker.showSeed)) return;
        if (readBool("ShowTime", &tracker.showTime)) return;
        if (readBool("ShowSong", &tracker.showSong)) return;
        if (readBool("ShowModSummary", &tracker.showModSummary)) return;
        if (readBool("ShowRuleSummary", &tracker.showModSummary)) return;

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
    buf->appendf("ShowCharacters=%d\n", tracker.showCharacters ? 1 : 0);
    buf->appendf("ShowSeed=%d\n", tracker.showSeed ? 1 : 0);
    buf->appendf("ShowTime=%d\n", tracker.showTime ? 1 : 0);
    buf->appendf("ShowSong=%d\n", tracker.showSong ? 1 : 0);
    buf->appendf("ShowModSummary=%d\n", tracker.showModSummary ? 1 : 0);
    buf->appendf("AttemptsDisplayMode=%d\n", (int)tracker.attemptsDisplayMode);
    buf->appendf("Attempts=%d\n", tracker.attemptCounter.load());
    buf->appendf("GameOvers=%d\n", tracker.gameOverCounter.load());
    buf->append("\n");

    buf->append("[IronMogFF7][Appearance]\n");
    buf->appendf("AccentColor=#%02X%02X%02X\n\n",
        (int)(accentColor[0] * 255.0f + 0.5f),
        (int)(accentColor[1] * 255.0f + 0.5f),
        (int)(accentColor[2] * 255.0f + 0.5f));
}
