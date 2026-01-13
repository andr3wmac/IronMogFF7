#include "App.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/IconsFontAwesome5.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Platform.h"
#include "core/utilities/Utilities.h"
#include "extras/Extra.h"
#include "extras/RandomizeMusic.h"
#include "rules/Permadeath.h"
#include "rules/Rule.h"

#include <imgui.h>

static const char* emulators[]{ "DuckStation", "BizHawk", "Custom" };

static ImColor dotRed(1.0f, 0.0f, 0.0f, 1.0f);
static ImColor dotYellow(1.0f, 1.0f, 0.0f, 1.0f);
static ImColor dotGreen(0.0f, 1.0f, 0.0f, 1.0f);

void App::drawSettingsPanel()
{
    GUI::drawImage(logo, logo.width / 2, logo.height / 2);

    // We lock settings if we're both connected and in game.
    bool lockSettings = connectionState > ConnectionState::NotConnected && connectionState < ConnectionState::Error;
    if (lockSettings && connectionState == ConnectionState::Connected)
    {
        GameManager::GameState state = game->getState();
        lockSettings &= (state == GameManager::GameState::InGame);
    }

    ImGui::Spacing();
    ImGui::BeginChild("##ScrollBox", ImVec2(0, APP_WINDOW_HEIGHT - 212));
    ImGui::BeginDisabled(lockSettings);
    {
        ImGui::SeparatorText("Game");
        {
            ImGui::Text("Emulator:");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(393.0f);
            int emulatorIndex = (int)selectedEmulatorType;
            if (ImGui::Combo("##EmulatorList", &emulatorIndex, emulators, IM_ARRAYSIZE(emulators)))
            {
                selectedEmulatorType = (EmulatorType)emulatorIndex;

                // Update list of processes if Custom is selected.
                if (selectedEmulatorType == EmulatorType::Custom)
                {
                    runningProcesses = Platform::getRunningProcesses();
                }
            }

            if (selectedEmulatorType == EmulatorType::Custom)
            {
                ImGui::Text("Process:");
                ImGui::SameLine();
                ImGui::Combo("##ProcessList", &selectedProcessIdx, runningProcesses.data(), (int)runningProcesses.size());
                ImGui::Text("Memory Offset:");
                ImGui::SameLine();
                ImGui::InputText("##MemoryOffset", processMemoryOffset, 20);
            }
        }
        ImGui::Spacing();

        ImGui::SeparatorText("Seed");
        {
            ImGui::SetNextItemWidth(378.0f);
            ImGui::InputText("##Seed", seedValue, 9);
            ImGui::SameLine();
            if (ImGui::Button("Regenerate"))
            {
                generateSeed();
            }
        }
        ImGui::Spacing();

        ImGui::SeparatorText("Settings");
        {
            ImGui::SetNextItemWidth(410.0f);
            if (ImGui::Combo("##SettingsList", &selectedSettingsIdx, availableSettings.data(), (int)availableSettings.size()))
            {
                loadSettings("settings/" + availableSettings[selectedSettingsIdx] + ".cfg");
            }

            ImGui::SameLine();
            ImGui::PushID("OPEN_SETTINGS_FILE");
            if (ImGui::Button(ICON_FA_FOLDER_OPEN))
            {
                std::string openPath = gui.openFileDialog();
                if (openPath != "")
                {
                    loadSettings(openPath);
                }
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID("SAVE_SETTINGS_FILE");
            if (ImGui::Button(ICON_FA_SAVE))
            {
                std::string savePath = gui.saveFileDialog();
                if (savePath != "")
                {
                    saveSettings(savePath);
                }
            }
            ImGui::PopID();
        }
        ImGui::Spacing();

        ImGui::SeparatorText("Rules");
        {
            bool changed = false;
            int ruleIndex = 0;
            for (auto& rule : Rule::getList())
            {
                changed |= ImGui::Checkbox(rule->name.c_str(), &rule->enabled);

                if (rule->hasSettings())
                {
                    ImGui::SameLine();

                    std::string ruleID = "RuleSettings" + std::to_string(ruleIndex);
                    ImGui::PushID(ruleID.c_str());
                    if (ImGui::Button(ICON_FA_COG))
                    {
                        rule->settingsVisible = !rule->settingsVisible;
                    }
                    ImGui::PopID();
                    ruleIndex++;

                    if (rule->settingsVisible)
                    {
                        ImGui::Indent(25.0f);
                        changed |= rule->onSettingsGUI();
                        ImGui::Unindent(25.0f);
                    }
                }
            }

            if (changed)
            {
                // Reset to custom.
                selectedSettingsIdx = 0;
            }
        }
        ImGui::Spacing();
    }
    ImGui::EndDisabled();

    // Note: extras can be changed during gameplay
    ImGui::SeparatorText("Extras");
    {
        bool changed = false;
        int extraIndex = 0;
        for (auto& extra : Extra::getList())
        {
            changed |= ImGui::Checkbox(extra->name.c_str(), &extra->enabled);

            if (extra->hasSettings())
            {
                ImGui::SameLine();

                std::string extraID = "ExtraSettings" + std::to_string(extraIndex);
                ImGui::PushID(extraID.c_str());
                if (ImGui::Button(ICON_FA_COG))
                {
                    extra->settingsVisible = !extra->settingsVisible;
                }
                ImGui::PopID();
                extraIndex++;

                if (extra->settingsVisible)
                {
                    ImGui::Indent(25.0f);
                    changed |= extra->onSettingsGUI();
                    ImGui::Unindent(25.0f);
                }
            }
        }

        if (changed)
        {
            // Reset to custom.
            selectedSettingsIdx = 0;
        }
    }

    ImGui::EndChild();
    ImGui::Spacing();

    drawBottomPanel();
}

void App::drawTrackerPanel()
{
    GUI::drawImage(logo, logo.width / 2, logo.height / 2);

    ImGui::Spacing();
    ImGui::BeginChild("##ScrollBox", ImVec2(0, APP_WINDOW_HEIGHT - 212));
    {
        if (connectionState == ConnectionState::Connected)
        {
            // Permadeath Character Portraits
            {
                Permadeath* permadeathRule = (Permadeath*)game->getRule("Permadeath");
                uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);

                const int imgWidth = 46;
                const int imgHeight = 53;

                for (int i = 0; i < 9; ++i)
                {
                    uint8_t characterID = CharacterDataOffsets::CharacterIDs[i];

                    float iconAlpha = 0.25f;
                    if (Utilities::isBitSet(phsVisMask, i))
                    {
                        iconAlpha = 1.0f;
                    }

                    ImVec2 p = ImGui::GetCursorScreenPos();
                    GUI::drawImage(characterPortraits[i], imgWidth, imgHeight, iconAlpha);
                    ImGui::SameLine();

                    if (permadeathRule != nullptr)
                    {
                        if (permadeathRule->isCharacterDead(characterID))
                        {
                            ImGui::GetWindowDrawList()->AddImage((ImTextureID)deadIcon.textureID, p, ImVec2(p.x + imgWidth, p.y + imgHeight), ImVec2(0, 0), ImVec2(1, 1));
                        }
                    }
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();

            // In Game Time
            uint32_t igt = game->read<uint32_t>(GameOffsets::InGameTime);
            std::string igtText = "In Game Time: " + Utilities::formatTime(igt);
            ImGui::Text(igtText.c_str());

            // Current Song
            if (game->isExtraEnabled("Randomize Music"))
            {
                RandomizeMusic* musicRando = (RandomizeMusic*)game->getExtra("Randomize Music");
                if (musicRando->isPlaying())
                {
                    std::string currentSong = musicRando->getCurrentlyPlaying();
                    std::string currentSongText = "Current Song: " + currentSong;
                    ImGui::Text(currentSongText.c_str());
                }
            }
        }
    }
    ImGui::EndChild();
    ImGui::Spacing();

    drawBottomPanel();
}

void App::drawBottomPanel()
{
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (connectionState == ConnectionState::NotConnected || connectionState == ConnectionState::Error)
    {
        drawList->AddCircleFilled(ImVec2(p.x + 135, p.y + 10), 5.0f, dotRed);
    }
    if (connectionState == ConnectionState::Connecting)
    {
        drawList->AddCircleFilled(ImVec2(p.x + 135, p.y + 10), 5.0f, dotYellow);
    }
    if (connectionState == ConnectionState::Connected)
    {
        drawList->AddCircleFilled(ImVec2(p.x + 135, p.y + 10), 5.0f, dotGreen);
    }

    if (connectionState == ConnectionState::NotConnected || connectionState == ConnectionState::Error)
    {
        if (ImGui::Button("Connect", ImVec2(120, 0)))
        {
            connected();
        }
    }
    else
    {
        ImGui::BeginDisabled(connectionState == ConnectionState::Connecting);
        if (ImGui::Button("Disconnect", ImVec2(120, 0)))
        {
            disconnect();
        }
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::Indent(150.0f);
    ImGui::Text(connectionStatus.c_str());

    ImGui::SameLine();
    ImGui::Indent(210.0f);

    if (currentPanel == Panels::Settings)
    {
        ImGui::BeginDisabled(connectionState != ConnectionState::Connected);
        if (ImGui::Button("Tracker", ImVec2(120, 0)))
        {
            currentPanel = Panels::Tracker;
        }
        ImGui::EndDisabled();
    }
    else if (currentPanel == Panels::Tracker)
    {
        if (ImGui::Button("Settings", ImVec2(120, 0)))
        {
            currentPanel = Panels::Settings;
        }
    }

    ImGui::Unindent(150.0f);
}

void App::drawDebugPanel()
{
    std::string connectionText = "Connection: " + connectionStatus;
    ImGui::Text(connectionText.c_str());
    
    if (connectionState != ConnectionState::Connected)
    {
        return;
    }

    GameManager::GameState gameState = game->getState();
    {
        std::string gameStateText = "State: ";
        if (gameState == GameManager::GameState::BootScreen) gameStateText += "Boot";
        if (gameState == GameManager::GameState::MainMenuCold) gameStateText += "Main Menu (Cold)";
        if (gameState == GameManager::GameState::MainMenuWarm) gameStateText += "Main Menu (Warm)";
        if (gameState == GameManager::GameState::InGame) gameStateText += "In Game";
        ImGui::Text(gameStateText.c_str());
    }

    // IronMog Frame Update Time
    double updateDuration = game->getLastUpdateDuration();
    std::string updateDurationText = "IronMog Update Time: " + std::to_string(updateDuration) + "ms";
    ImGui::Text(updateDurationText.c_str());

    // Frame Number
    uint32_t frameNumber = game->read<uint32_t>(GameOffsets::FrameNumber);
    std::string frameNumberText = "Frame Number: " + std::to_string(frameNumber);
    ImGui::Text(frameNumberText.c_str());

    // In Game Time
    uint32_t igt = game->read<uint32_t>(GameOffsets::InGameTime);
    std::string igtText = "In Game Time: " + Utilities::formatTime(igt);
    ImGui::Text(igtText.c_str());

    // Game Moment
    uint16_t moment = game->getGameMoment();
    std::string momentText = "Game Moment: " + std::to_string(moment);
    ImGui::Text(momentText.c_str());

    // Module ID
    uint8_t gameModule = game->read<uint8_t>(GameOffsets::CurrentModule);
    std::string moduleText = "Module: " + std::to_string(gameModule);
    ImGui::Text(moduleText.c_str());

    // Music ID
    uint16_t musicID = game->read<uint16_t>(GameOffsets::MusicID);
    std::string musicText = "Music: " + std::to_string(musicID);
    ImGui::Text(musicText.c_str());

    // Field ID
    uint16_t fieldID = game->read<uint16_t>(GameOffsets::FieldID);
    std::string fieldText = "Field ID: " + std::to_string(fieldID);
    ImGui::Text(fieldText.c_str());

    // Formation
    uint16_t formationID = game->read<uint16_t>(BattleOffsets::FormationID);
    std::string formationText = "Formation: " + std::to_string(formationID);
    ImGui::Text(formationText.c_str());

    // Party Members
    {
        uint8_t party0 = game->read<uint8_t>(GameOffsets::PartyIDList + 0);
        uint8_t party1 = game->read<uint8_t>(GameOffsets::PartyIDList + 1);
        uint8_t party2 = game->read<uint8_t>(GameOffsets::PartyIDList + 2);
        std::string partyText = "Party: " + std::to_string(party0) + " " + std::to_string(party1) + " " + std::to_string(party2);
        ImGui::Text(partyText.c_str());
    }

    // Player Position
    int position[3];
    if (game->getGameModule() == GameModule::World)
    {
        position[0] = game->read<int>(WorldOffsets::WorldX);
        position[1] = game->read<int>(WorldOffsets::WorldY);
        position[2] = game->read<int>(WorldOffsets::WorldZ);

        std::string positionText = "Position: " + std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]);
        ImGui::Text(positionText.c_str());
    }
    else 
    {
        position[0] = game->read<int>(FieldOffsets::FieldX);
        position[1] = game->read<int>(FieldOffsets::FieldY);
        position[2] = game->read<int>(FieldOffsets::FieldZ);

        float fx = position[0] / 4096.0f;
        float fy = position[1] / 4096.0f;
        float fz = position[2] / 4096.0f;

        std::string positionText = "Position: " + std::to_string(fx) + ", " + std::to_string(fy) + ", " + std::to_string(fz);
        ImGui::Text(positionText.c_str());
    }

    if (ImGui::CollapsingHeader("Cheats"))
    {
        ImGui::Indent(25.0f);

        if (ImGui::Button("Disable Encounters"))
        {
            game->write<uint8_t>(0x9AC2F, 0xFF);
        }

        if (ImGui::Button("Add 10000 Gil"))
        {
            uint32_t gil = game->read<uint32_t>(GameOffsets::Gil);
            game->write<uint32_t>(GameOffsets::Gil, gil + 10000);
        }

        if (ImGui::Button("Recover Party"))
        {
            std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
            for (int i = 0; i < 3; ++i)
            {
                uint8_t id = partyIDs[i];
                if (id == 0xFF)
                {
                    continue;
                }

                uintptr_t characterOffset = getCharacterDataOffset(id);
                uint16_t maxHP = game->read<uint16_t>(characterOffset + CharacterDataOffsets::MaxHP);
                game->write<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP, maxHP);
            }
        }

        ImGui::Unindent(25.0f);
    }

    if (ImGui::CollapsingHeader("Warp"))
    {
        ImGui::Indent(25.0f);

        // Debug Panel variables
        static char debugWarpFieldID[5] = "";
        ImGui::InputText("##DebugWarpFieldID", debugWarpFieldID, 5);
        ImGui::SameLine();
        if (ImGui::Button("Warp To Field"))
        {
            uint16_t warpFieldID = atoi(debugWarpFieldID);
            game->write<uint16_t>(GameOffsets::FieldWarpID, warpFieldID);
            game->write<uint8_t>(GameOffsets::FieldWarpTrigger, 1);
        }

        ImGui::Unindent(25.0f);
    }

    for (auto& rule : Rule::getList())
    {
        if (!rule->hasDebugGUI())
        {
            continue;
        }

        if (ImGui::CollapsingHeader(rule->name.c_str()))
        {
            ImGui::Indent(25.0f);
            rule->onDebugGUI();
            ImGui::Unindent(25.0f);
        }
    }

    for (auto& extra : Extra::getList())
    {
        if (!extra->hasDebugGUI())
        {
            continue;
        }

        if (ImGui::CollapsingHeader(extra->name.c_str()))
        {
            ImGui::Indent(25.0f);
            extra->onDebugGUI();
            ImGui::Unindent(25.0f);
        }
    }
}