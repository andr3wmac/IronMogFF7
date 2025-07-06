#include "gui/GUI.h"
#include "gui/IconsFontAwesome5.h"
#include "game/GameData.h"
#include "game/GameManager.h"
#include "rules/RandomizeFieldItems.h"
#include "rules/Restrictions.h"
#include "utilities/Logging.h"
#include "utilities/MemorySearch.h"
#include "utilities/Utilities.h"

#include <imgui.h>
#include <iostream>
#include <thread>
#include <random>
#include <iomanip>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

GameManager* game = nullptr;

const char* emulators[] { "DuckStation", "BizHawk", "Custom" };
int selectedEmulatorIdx = 0;

std::vector<std::string> runningProcesses;
std::vector<const char*> runningProcessesCStr;
int selectedProcessIdx = 0;
char processMemoryOffset[20];
char seedValue[9];

int connectionState = 0;
std::string connectionStatus = "Not Attached";

ImColor dotRed(1.0f, 0.0f, 0.0f, 1.0f);
ImColor dotYellow(1.0f, 1.0f, 0.0f, 1.0f);
ImColor dotGreen(0.0f, 1.0f, 0.0f, 1.0f);

std::thread* managerThread = nullptr;
std::atomic<bool> managerRunning = false;

void generateSeed() 
{
    std::random_device rd;
    uint32_t seed = (static_cast<uint32_t>(rd()) << 16) ^ rd();
    snprintf(seedValue, sizeof(seedValue), "%08X", seed);
    LOG("Seed generated: %s", seedValue);
}

uint32_t hexStringToUint32(const std::string& hex) 
{
    uint32_t seed;
    std::istringstream iss(hex);
    iss >> std::hex >> seed;
    return seed;
}

void onStart()
{
    uint32_t chosenSeed = game->getSeed();
    snprintf(seedValue, 9, "%08X", chosenSeed);
}

void runGameManager()
{
    if (game != nullptr)
    {
        delete game;
        game = nullptr;
    }

    game = new GameManager();
    game->onStart.AddListener(onStart);
    connectionState = 1;
    connectionStatus = "Attaching to Emulator..";

    bool connected = false;

    if (selectedEmulatorIdx == 0)
    {
        connectionStatus = "Attaching to DuckStation..";
        std::string targetProcess = "duckstation-qt-x64-ReleaseLTCG.exe";
        connected = game->attachToEmulator(targetProcess);
    }
    if (selectedEmulatorIdx == 1)
    {
        connectionStatus = "Attaching to BizHawk..";
        std::string targetProcess = "EmuHawk.exe";
        connected = game->attachToEmulator(targetProcess);
    }
    if (selectedEmulatorIdx == 2)
    {
        uintptr_t customAddress = Utilities::parseAddress(processMemoryOffset);
        connected = game->attachToEmulator(runningProcesses[selectedProcessIdx], customAddress);
    }

    if (connected)
    {
        connectionState = 2;
        connectionStatus = "Attached to emulator.";
    }
    else 
    {
        connectionState = 0;
        connectionStatus = "Failed to attach to emulator.";
    }

    // Reset any global restrictions as we might be using a different set of rules on this run.
    Restrictions::reset();

    game->setup(hexStringToUint32(seedValue));

    MemorySearch search(game);
    std::vector<uint8_t> searchEskill = { 0xAA, 0x55, 0xAA };
    std::vector<uintptr_t> results = search.search(searchEskill);

    managerRunning = true;
    while (managerRunning.load())
    {
        game->update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void attach()
{
    if (managerThread != nullptr)
    {
        return;
    }

    managerThread = new std::thread(runGameManager);
}

void detach()
{
    connectionState = 0;
    connectionStatus = "Not Attached";

    if (managerThread == nullptr || !managerRunning.load())
    {
        return;
    }

    managerRunning = false;
    managerThread->join();
    delete managerThread;
    managerThread = nullptr;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    GameData::loadGameData();

    GUI gui;
    gui.initialize();

    generateSeed();

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
            gui.drawLogo();

            ImGui::Spacing();
            ImGui::BeginChild("##ScrollBox", ImVec2(0, 300));
            ImGui::BeginDisabled(connectionState > 0);
            {
                ImGui::SeparatorText("Game");
                {
                    ImGui::Text("Emulator:");
                    ImGui::SameLine();

                    if (ImGui::Combo("##EmulatorList", &selectedEmulatorIdx, emulators, IM_ARRAYSIZE(emulators)))
                    {
                        // Update list of processes if Custom is selected.
                        if (selectedEmulatorIdx == 2)
                        {
                            runningProcesses = Utilities::getRunningProcesses();
                            runningProcessesCStr.clear();
                            for (const auto& name : runningProcesses)
                            {
                                runningProcessesCStr.push_back(name.c_str());
                            }
                        }
                    }

                    if (selectedEmulatorIdx == 2)
                    {
                        ImGui::Text("Process:");
                        ImGui::SameLine();
                        ImGui::Combo("##ProcessList", &selectedProcessIdx, runningProcessesCStr.data(), (int)runningProcessesCStr.size());
                        ImGui::Text("Memory Offset:");
                        ImGui::SameLine();
                        ImGui::InputText("##MemoryOffset", processMemoryOffset, 20);
                    }
                }
                ImGui::Spacing();

                ImGui::SeparatorText("Seed");
                {
                    ImGui::InputText("##Seed", seedValue, 9);
                    ImGui::SameLine();
                    if (ImGui::Button("Regenerate"))
                    {
                        generateSeed();
                    }
                }
                ImGui::Spacing();

                ImGui::SeparatorText("Rules");
                {
                    int ruleIndex = 0;
                    for (auto& rule : Rule::getList())
                    {
                        ImGui::Checkbox(rule->name.c_str(), &rule->enabled);
                        
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
                                rule->onSettingsGUI();
                                ImGui::Unindent(25.0f);
                            }
                        }
                    }
                }
            }
            ImGui::EndDisabled();
            ImGui::EndChild();
            ImGui::Spacing();

            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            if (connectionState == 0)
            {
                drawList->AddCircleFilled(ImVec2(p.x + 135, p.y + 10), 5.0f, dotRed);
            }
            if (connectionState == 1)
            {
                drawList->AddCircleFilled(ImVec2(p.x + 135, p.y + 10), 5.0f, dotYellow);
            }
            if (connectionState == 2)
            {
                drawList->AddCircleFilled(ImVec2(p.x + 135, p.y + 10), 5.0f, dotGreen);
            }

            if (connectionState == 0)
            {
                if (ImGui::Button("Attach", ImVec2(120, 0)))
                {
                    attach();
                }
            }
            else 
            {
                ImGui::BeginDisabled(connectionState == 1);
                if (ImGui::Button("Detach", ImVec2(120, 0)))
                {
                    detach();
                }
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            ImGui::Indent(150.0f);
            ImGui::Text(connectionStatus.c_str());
            ImGui::Unindent(150.0f);

        }
        ImGui::End();

        gui.endFrame();
    }

    gui.destroy();
    return 0;
}