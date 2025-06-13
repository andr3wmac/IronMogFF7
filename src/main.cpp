#include "gui/GUI.h"
#include "game/GameManager.h"
#include "rules/RandomizeFieldItems.h"
#include "utilities/MemorySearch.h"
#include "utilities/Process.h"

#include <imgui.h>
#include <iostream>
#include <thread>
#include <random>
#include <iomanip>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

GameManager* game = nullptr;

const char* emulators[] { "Duckstation", "Custom" };
int selectedEmulatorIdx = 0;

std::vector<std::string> runningProcesses;
std::vector<const char*> runningProcessesCStr;
int selectedProcessIdx = 0;
char processMemoryOffset[20];

char seedValue[9];
std::vector<std::pair<std::string, bool>> ruleData;

int connectionState = 0;
std::string connectionStatus = "Not Attached";

ImColor dotRed(1.0f, 0.0f, 0.0f, 1.0f);
ImColor dotYellow(1.0f, 1.0f, 0.0f, 1.0f);
ImColor dotGreen(0.0f, 1.0f, 0.0f, 1.0f);

std::thread* managerThread = nullptr;
std::atomic<bool> managerRunning = false;

void generateSeed() 
{
    const int numBytes = 8;

    // Use a secure, high-entropy source
    std::random_device rd;
    std::array<uint8_t, numBytes> buffer;

    for (size_t i = 0; i < numBytes; ++i) 
    {
        buffer[i] = static_cast<uint8_t>(rd());
    }

    std::ostringstream oss;
    for (size_t i = 0; i < numBytes; ++i) 
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    std::string finalStr = oss.str();

    for (int i = 0; i < 8; ++i)
    {
        seedValue[i] = finalStr[i];
    }
    seedValue[8] = '\0';
}

uint32_t hexStringToUint32(const std::string& hex) 
{
    uint32_t seed;
    std::istringstream iss(hex);
    iss >> std::hex >> seed;
    return seed;
}

void runGameManager()
{
    if (game != nullptr)
    {
        delete game;
        game = nullptr;
    }

    game = new GameManager();
    connectionState = 1;
    connectionStatus = "Attaching to Duckstation..";

    bool connected = false;

    if (selectedEmulatorIdx == 0)
    {
        std::string targetProcess = "duckstation-qt-x64-ReleaseLTCG.exe";
        connected = game->attachToEmulator(targetProcess);
    }
    if (selectedEmulatorIdx == 1)
    {
        uintptr_t customAddress = Process::ParseAddress(processMemoryOffset);
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

    MemorySearch search(game);
    std::vector<uint8_t> searchValues = { 0x58, 0, 0, 0, 0x01 };
    std::vector<uintptr_t> results = search.search(searchValues);

    auto ruleRegistry = Rule::registry();
    for (auto& rule : ruleData) 
    {
        // Check if rule is enabled
        if (rule.second)
        {
            game->addRule(ruleRegistry[rule.first]());
        }
    }

    game->start(hexStringToUint32(seedValue));

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
    GUI gui;
    gui.initialize();

    generateSeed();

    // Populate list of rules
    {
        for (const auto& factory : Rule::registry())
        {
            ruleData.push_back({ factory.first, true });
        }
        std::sort(ruleData.begin(), ruleData.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    }

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
            ImGui::BeginChild("##ScrollBox", ImVec2(0, 250));
            ImGui::BeginDisabled(connectionState > 0);
            {
                ImGui::SeparatorText("Game");
                {
                    ImGui::Text("Emulator:");
                    ImGui::SameLine();

                    if (ImGui::Combo("##EmulatorList", &selectedEmulatorIdx, emulators, IM_ARRAYSIZE(emulators)))
                    {
                        // Update list of processes if Custom is selected.
                        if (selectedEmulatorIdx == 1)
                        {
                            runningProcesses = Process::GetRunningProcesses();
                            runningProcessesCStr.clear();
                            for (const auto& name : runningProcesses)
                            {
                                runningProcessesCStr.push_back(name.c_str());
                            }
                        }
                    }

                    if (selectedEmulatorIdx == 1)
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
                    for (auto& rule : ruleData)
                    {
                        ImGui::Checkbox(rule.first.c_str(), &rule.second);
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