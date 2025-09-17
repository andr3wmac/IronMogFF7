#include "App.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/IconsFontAwesome5.h"
#include "core/utilities/Utilities.h"
#include "extras/Extra.h"
#include "rules/Permadeath.h"
#include "rules/Rule.h"

#include <imgui.h>

static const char* emulators[]{ "DuckStation", "BizHawk", "Custom" };

static ImColor dotRed(1.0f, 0.0f, 0.0f, 1.0f);
static ImColor dotYellow(1.0f, 1.0f, 0.0f, 1.0f);
static ImColor dotGreen(0.0f, 1.0f, 0.0f, 1.0f);

void App::drawMainPanel()
{
    gui.drawImage(logo, logo.width / 2, logo.height / 2);

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
        ImGui::Spacing();
    }
    ImGui::EndDisabled();

    // Note: extras can be changed during gameplay
    ImGui::SeparatorText("Extras");
    {
        int extraIndex = 0;
        for (auto& extra : Extra::getList())
        {
            ImGui::Checkbox(extra->name.c_str(), &extra->enabled);

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
                    extra->onSettingsGUI();
                    ImGui::Unindent(25.0f);
                }
            }
        }
    }

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

    ImGui::SameLine();
    ImGui::Indent(210.0f);
    if (ImGui::Button("Status", ImVec2(120, 0)))
    {
        currentPanel = 1;
    }

    ImGui::Unindent(150.0f);
}

void App::drawStatusPanel()
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
        gui.drawImage(characterPortraits[i], imgWidth, imgHeight, iconAlpha);
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

void App::drawDebugPanel()
{
    std::string connectionText = "Connection: " + connectionStatus;
    ImGui::Text(connectionText.c_str());
    
    if (connectionState != 2)
    {
        return;
    }

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

    if (ImGui::Button("Disable Encounters"))
    {
        game->write<uint8_t>(0x9AC2F, 0xFF);
    }

    if (ImGui::Button("Add 10000 Gil"))
    {
        uint32_t gil = game->read<uint32_t>(0x9D260);
        game->write<uint32_t>(0x9D260, gil + 10000);
    }

    int ruleIndex = 0;
    for (auto& rule : Rule::getList())
    {
        if (!rule->hasDebugGUI())
        {
            continue;
        }

        if (ImGui::CollapsingHeader(rule->name.c_str()))
        {
            rule->onDebugGUI();
        }
    }
}