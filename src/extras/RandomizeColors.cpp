#include "RandomizeColors.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/MemorySearch.h"
#include "core/utilities/ModelEditor.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>

REGISTER_EXTRA("Randomize Colors", RandomizeColors)

void RandomizeColors::setup()
{
    BIND_EVENT(game->onStart, RandomizeColors::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeColors::onFieldChanged);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeColors::onFrame);

    debugStartNum[0] = '\0';
    debugCount[0] = '\0';
}

void RandomizeColors::onDebugGUI()
{
    if (game->getGameModule() != GameModule::World)
    {
       // ImGui::Text("Not currently on world map.");
        //return;
    }

    if (ImGui::CollapsingHeader("Character Finder"))
    {
        if (ImGui::Button("Find Cloud", ImVec2(120, 0)))
        {
            /*MemorySearch polySearch(game);
            std::vector<uintptr_t> results = polySearch.searchForPolygons();

            ModelEditor modelEditor(game);
            for (int i = 0; i < results.size(); ++i)
            {
                if (modelEditor.openModel(results[i], "CLOUD"))
                {
                    LOG("Cloud found at: %d", results[i]);
                }
            }*/
            
            uint64_t startTime = Utilities::getTimeMS();
            ModelEditor modelEditor(game);
            modelEditor.findModel("CLOUD");
            uint64_t endTime = Utilities::getTimeMS();
            LOG("Find cloud took: %d ms", endTime - startTime);
        }
    }

    if (ImGui::CollapsingHeader("Color Browser"))
    {
        ImGui::Text("Start Num:");
        ImGui::SameLine();
        ImGui::InputText("##StartNum", debugStartNum, 20);

        ImGui::Text("Count:");
        ImGui::SameLine();
        ImGui::InputText("##Count", debugCount, 20);

        uintptr_t startIndex = atoi(debugStartNum);
        uintptr_t count = atoi(debugCount);

        if (ImGui::Button("Load", ImVec2(120, 0)))
        {
            MemorySearch polySearch(game);
            std::vector<uintptr_t> results = polySearch.searchForPolygons();

            debugAddresses.clear();
            if (startIndex < results.size())
            {
                int endIndex = std::min(startIndex + count, results.size());
                debugAddresses.insert(debugAddresses.end(), results.begin() + startIndex, results.begin() + endIndex);
            }
        }

        if (debugAddresses.size() == 0)
        {
            return;
        }

        const float boxSize = 16.0f;   // size of each color square
        const float spacing = 2.0f;    // gap between squares
        const int colorsPerRow = 24;   // wrap every 20 colors

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 startPos = ImGui::GetCursorScreenPos();

        for (size_t i = 0; i < debugAddresses.size(); ++i)
        {
            uintptr_t addr = debugAddresses[i];

            uint8_t r = game->read<uint8_t>(addr + 0);
            uint8_t g = game->read<uint8_t>(addr + 1);
            uint8_t b = game->read<uint8_t>(addr + 2);

            // compute position in grid
            int row = static_cast<int>(i / colorsPerRow);
            int col = static_cast<int>(i % colorsPerRow);
            ImVec2 p0 = ImVec2(startPos.x + col * (boxSize + spacing),
                startPos.y + row * (boxSize + spacing));
            ImVec2 p1 = ImVec2(p0.x + boxSize, p0.y + boxSize);

            // draw filled rect
            ImU32 color = IM_COL32(r, g, b, 255);
            drawList->AddRectFilled(p0, p1, color);
            drawList->AddRect(p0, p1, IM_COL32(60, 60, 60, 255)); // border

            // Make it an invisible button for interaction
            ImGui::SetCursorScreenPos(p0);
            ImGui::InvisibleButton(("color" + std::to_string(i)).c_str(), ImVec2(boxSize, boxSize));

            if (ImGui::IsItemClicked())
            {
                // Do something, e.g. print to console
                LOG("Clicked color %zu: RGB(%d, %d, %d)", i, r, g, b);

                game->write<uint8_t>(addr + 0, 0xFF);
                game->write<uint8_t>(addr + 1, 0x00);
                game->write<uint8_t>(addr + 2, 0xFF);
            }
        }

        // advance cursor so the next ImGui item doesn't overlap
        int totalRows = static_cast<int>((debugAddresses.size() + colorsPerRow - 1) / colorsPerRow);
        ImGui::Dummy(ImVec2(0.0f, totalRows * (boxSize + spacing)));
    }
}

void RandomizeColors::onStart()
{

}

void RandomizeColors::onFieldChanged(uint16_t fieldID)
{
    frameWait = 120;
}

void RandomizeColors::onFrame(uint32_t frameNumber)
{
    if (frameWait == 0)
    {
        return;
    }

    frameWait--;
    if (frameWait == 0)
    {
        ModelEditor modelEditor(game);
        if (modelEditor.findModel("CLOUD"))
        {
            uint8_t r = 255;
            uint8_t g = 0;
            uint8_t b = 0;

            modelEditor.setPartTint(0, r, g, b);
            modelEditor.setPartTint(1, r, g, b);
            modelEditor.setPartTint(9, r, g, b);
            modelEditor.setPartTint(10, r, g, b);
            modelEditor.setPartTint(12, r, g, b);
            modelEditor.setPartTint(13, r, g, b);

            LOG("UPDATED CLOUD");
        }
        else
        {
            LOG("COULD NOT FIND CLOUD!");
        }
    }
}