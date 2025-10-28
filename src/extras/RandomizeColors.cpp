#include "RandomizeColors.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/GUI.h"
#include "core/utilities/Logging.h"
#include "core/utilities/MemorySearch.h"
#include "core/utilities/ModelEditor.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>

REGISTER_EXTRA("Randomize Colors", RandomizeColors)

#define COLORS_PER_MODEL 16

void RandomizeColors::setup()
{
    BIND_EVENT(game->onStart, RandomizeColors::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeColors::onFieldChanged);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeColors::onFrame);

    debugStartNum[0] = '\0';
    debugCount[0] = '\0';

    modelEditor.setup(game);
}

void RandomizeColors::onDebugGUI()
{
    if (game->getGameModule() != GameModule::World)
    {
       // ImGui::Text("Not currently on world map.");
        //return;
    }

    uint16_t fieldFade = game->read<uint16_t>(GameOffsets::ScreenFade);
    std::string fieldFadeStr = "Screen Fade: " + std::to_string(fieldFade);
    ImGui::Text(fieldFadeStr.c_str());

    if (ImGui::Button("Force Update", ImVec2(120, 0)))
    {
        waitingForField = true;
        lastFieldID = -1;
        waitingForWorld = true;
    }

    if (ImGui::CollapsingHeader("Current Models"))
    {
        std::vector<std::string> openModels = modelEditor.getOpenModelNames();
        for (int i = 0; i < openModels.size(); ++i)
        {
            ImGui::Text(openModels[i].c_str());
        }
    }

    if (ImGui::CollapsingHeader("Random Table"))
    {
        for (auto kv : randomModelColors)
        {
            ImGui::Text(kv.first.c_str());
            GUI::drawColorGrid(kv.first, randomModelColors[kv.first], {}, 16.0f, 2.0f, 16);
        }
    }

    if (ImGui::CollapsingHeader("Color Scanner"))
    {
        ImGui::Text("Start Num:");
        ImGui::SameLine();
        ImGui::InputText("##StartNum", debugStartNum, 20);

        ImGui::Text("Count:");
        ImGui::SameLine();
        ImGui::InputText("##Count", debugCount, 20);

        uintptr_t startIndex = atoi(debugStartNum);
        uintptr_t count = atoi(debugCount);

        if (ImGui::Button("Scan", ImVec2(120, 0)))
        {
            MemorySearch polySearch(game);
            std::vector<uintptr_t> results = polySearch.searchForPolygons();

            debugAddresses.clear();
            if (startIndex < results.size())
            {
                uintptr_t endIndex = std::min(startIndex + count, results.size());
                debugAddresses.insert(debugAddresses.end(), results.begin() + startIndex, results.begin() + endIndex);
            }
        }

        if (debugAddresses.size() == 0)
        {
            return;
        }

        std::vector<Utilities::Color> debugColors;
        debugColors.resize(debugAddresses.size());
        for (size_t i = 0; i < debugAddresses.size(); ++i)
        {
            uintptr_t addr = debugAddresses[i];

            debugColors[i].r = game->read<uint8_t>(addr + 0);
            debugColors[i].g = game->read<uint8_t>(addr + 1);
            debugColors[i].b = game->read<uint8_t>(addr + 2);
        }

        GUI::drawColorGrid("debugColors", debugColors, [this](int clickedIndex, Utilities::Color clickedColor)
            {
                LOG("Clicked color %zu: RGB(%d, %d, %d)", clickedIndex, clickedColor.r, clickedColor.g, clickedColor.b);

                uintptr_t addr = debugAddresses[clickedIndex];
                game->write<uint8_t>(addr + 0, 0xFF);
                game->write<uint8_t>(addr + 1, 0x00);
                game->write<uint8_t>(addr + 2, 0xFF);
            });
    }
}

Utilities::Color getRandomColor(std::mt19937& rng)
{
    std::uniform_int_distribution<uint32_t> dist(0, 255);

    Utilities::Color color;
    color.r = dist(rng);
    color.g = dist(rng);
    color.b = dist(rng);
    return color;
}

void RandomizeColors::onStart()
{
    lastFieldFade = 0;
    lastFieldID = -1;
    waitingForField = true;

    std::mt19937 rng(game->getSeed());

    // Generate table of random colors
    for (auto kv : GameData::models)
    {
        std::string modelName = kv.first;
        for (int i = 0; i < COLORS_PER_MODEL; ++i)
        {
            randomModelColors[modelName].push_back(getRandomColor(rng));
        }
    }
}

void RandomizeColors::onFieldChanged(uint16_t fieldID)
{
    uint16_t fieldFade = game->read<uint16_t>(GameOffsets::ScreenFade);
    lastFieldFade = fieldFade;
    waitingForField = true;
}

void RandomizeColors::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() == GameModule::Field && waitingForField)
    {
        uint16_t fieldFade = game->read<uint16_t>(GameOffsets::ScreenFade);

        // Update if we've hit peak fade out and started coming back down
        // or if the last field is unset.
        bool shouldUpdate = (lastFieldFade == 0x100 && fieldFade < lastFieldFade);
        shouldUpdate |= (lastFieldID == -1);

        if (shouldUpdate)
        {
            modelEditor.findModels();
            applyColors();

            waitingForField = false;
            lastFieldID = game->getFieldID();
        }

        lastFieldFade = fieldFade;
    }

    if (game->getGameModule() == GameModule::World && waitingForWorld)
    {
        modelEditor.findModels();
        applyColors();
        waitingForWorld = false;
    }
}

void RandomizeColors::applyColors()
{
    std::vector<std::string> modelNames = modelEditor.getOpenModelNames();
    for (std::string& modelName : modelNames)
    {
        // The parts and polys below were manually chosen using gamedata/fieldModelViewer.py in the tools directory

        if (modelName == "CLOUD" || modelName == "CLOUD_WORLD")
        {
            // Make sure we use the same color table for cloud and cloud world.
            std::vector<Utilities::Color> randomColors = randomModelColors["CLOUD"];
            Utilities::Color& outfitColor = randomColors[0];

            modelEditor.tintPart(modelName, 0, outfitColor);
            modelEditor.tintPart(modelName, 1, outfitColor);
            modelEditor.tintPart(modelName, 9, outfitColor);
            modelEditor.tintPart(modelName, 10, outfitColor, { 4, 5, 6, 7, 8, 9 });
            modelEditor.tintPart(modelName, 12, outfitColor);
            modelEditor.tintPart(modelName, 13, outfitColor, { 4, 5, 6, 7, 8, 9 });
        }

        if (modelName == "BALLET")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& pantsColor = randomColors[0];
            Utilities::Color& shirtColor = randomColors[1];

            modelEditor.tintPart(modelName, 0, pantsColor);
            modelEditor.tintPart(modelName, 9, pantsColor);
            modelEditor.tintPart(modelName, 10, pantsColor, { 4, 5, 6, 7, 8, 9 });
            modelEditor.tintPart(modelName, 12, pantsColor);
            modelEditor.tintPart(modelName, 13, pantsColor, { 4, 5, 6, 7, 8, 9 });

            modelEditor.tintPart(modelName, 1, shirtColor, { 0, 1, 2, 3, 4, 52, 53, 54, 55, 64, 65, 66, 67, 68, 69, 70, 71, 79, 80, 81, 82 });
        }

        if (modelName == "TIFA")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& skirtColor = randomColors[0];
            Utilities::Color& shirtColor = randomColors[1];

            modelEditor.tintPart(modelName, 0, skirtColor);
            modelEditor.tintPart(modelName, 1, shirtColor, { 6, 7, 8, 36, 37, 38, 39, 40, 41, 42 });
        }

        if (modelName == "EARITH")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& dressColor = randomColors[0];
            Utilities::Color& jacketColor = randomColors[1];

            modelEditor.tintPart(modelName, 0, dressColor);
            modelEditor.tintPolys(modelName, 1, dressColor, { 17, 18, 19, 20, 21, 22, 23, 37, 38, 39, 40, 41, 42, 43 });
            modelEditor.tintPolys(modelName, 2, dressColor, { 104, 105, 106, 107, 108, 109, 110, 111, 112 });
            modelEditor.tintPart(modelName, 10, dressColor);
            modelEditor.tintPolys(modelName, 11, dressColor, { 0, 1, 2, 3, 4, 5 });
            modelEditor.tintPart(modelName, 13, dressColor);
            modelEditor.tintPolys(modelName, 14, dressColor, { 0, 1, 2, 3, 4, 5 });

            modelEditor.tintPolys(modelName, 1, dressColor, { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36 });
            modelEditor.tintPart(modelName, 4, jacketColor, { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 });
            modelEditor.tintPart(modelName, 7, jacketColor, { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 });
        }

        if (modelName == "RED")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& skinColor = randomColors[0];
            Utilities::Color& hairColor = randomColors[1];

            // Torso and Head
            modelEditor.tintPart(modelName, 0, skinColor);
            modelEditor.tintPolys(modelName, 1, skinColor, { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 35, 36, 37, 38 });
            modelEditor.tintPolyRange(modelName, 2, skinColor, 3, 59);
            modelEditor.tintPolyRange(modelName, 2, skinColor, 120, 130);

            // Front Left Leg
            modelEditor.tintPart(modelName, 3, skinColor);
            modelEditor.tintPart(modelName, 4, skinColor);
            modelEditor.tintPart(modelName, 5, skinColor, { 7, 8, 9, 10, 11 });
            modelEditor.tintPart(modelName, 6, skinColor);

            // Front Right Leg
            modelEditor.tintPart(modelName, 7, skinColor);
            modelEditor.tintPart(modelName, 8, skinColor);
            modelEditor.tintPart(modelName, 9, skinColor, { 7, 8, 9, 10, 11 });
            modelEditor.tintPart(modelName, 10, skinColor);

            // Back Left Leg
            modelEditor.tintPart(modelName, 11, skinColor);
            modelEditor.tintPart(modelName, 12, skinColor);
            modelEditor.tintPart(modelName, 13, skinColor, { 7, 8, 9, 10, 11 });
            modelEditor.tintPart(modelName, 14, skinColor);

            // Tail
            modelEditor.tintPart(modelName, 15, skinColor);
            modelEditor.tintPart(modelName, 16, skinColor);

            // Back Right Leg
            modelEditor.tintPart(modelName, 18, skinColor);
            modelEditor.tintPart(modelName, 19, skinColor);
            modelEditor.tintPart(modelName, 20, skinColor, { 7, 8, 9, 10, 11 });
            modelEditor.tintPart(modelName, 21, skinColor);

            // Hair (Head, Torso, Tail)
            modelEditor.tintPolys(modelName, 1, hairColor, { 0, 1, 2, 3, 28, 29, 30, 31, 32, 33, 34 });
            modelEditor.tintPolyRange(modelName, 2, skinColor, 62, 99);
            modelEditor.tintPart(modelName, 17, hairColor);
        }

        if (modelName == "CID")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& pantsColor = randomColors[0];
            Utilities::Color& shirtColor = randomColors[1];

            modelEditor.tintPart(modelName, 0, pantsColor);
            modelEditor.tintPart(modelName, 9, pantsColor);
            modelEditor.tintPart(modelName, 10, pantsColor, { 4, 5, 6, 7, 8, 9, 10, 11, 20 });
            modelEditor.tintPart(modelName, 12, pantsColor);
            modelEditor.tintPart(modelName, 13, pantsColor, { 4, 5, 6, 7, 8, 9, 10, 11, 20 });

            modelEditor.tintPolyRange(modelName, 1, shirtColor, 12, 35);
            modelEditor.tintPolyRange(modelName, 1, shirtColor, 43, 57);
            modelEditor.tintPart(modelName, 3, shirtColor, { 14, 15, 16, 17, 18 });
            modelEditor.tintPart(modelName, 6, shirtColor, { 14, 15, 16, 17, 18 });
        }

        if (modelName == "KETCY")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& moogleColor = randomColors[0];

            modelEditor.tintPolyRange(modelName, 0, moogleColor, 19, 95);
            modelEditor.tintPolyRange(modelName, 0, moogleColor, 110, 137);
            modelEditor.tintPart(modelName, 1, moogleColor);
            modelEditor.tintPart(modelName, 2, moogleColor);
            modelEditor.tintPart(modelName, 12, moogleColor);
            modelEditor.tintPart(modelName, 13, moogleColor);
            modelEditor.tintPart(modelName, 14, moogleColor);
            modelEditor.tintPart(modelName, 15, moogleColor);
        }

        if (modelName == "YUFI")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& shirtColor = randomColors[0];
            Utilities::Color& pantsColor = randomColors[1];

            modelEditor.tintPart(modelName, 1, shirtColor, { 12, 13, 14, 15, 16, 17, 35, 36, 37, 38, 39, 40 });
            modelEditor.tintPart(modelName, 3, shirtColor);
            modelEditor.tintPart(modelName, 4, shirtColor);

            modelEditor.tintPart(modelName, 0, pantsColor);
            modelEditor.tintPart(modelName, 11, pantsColor, { 7, 8, 9, 10 });
            modelEditor.tintPart(modelName, 14, pantsColor, { 7, 8, 9, 10 });

            // Shield
            modelEditor.tintPolyRange(modelName, 5, pantsColor, 17, 26);
            modelEditor.tintPart(modelName, 6, pantsColor);
        }

        if (modelName == "VINCENT")
        {
            std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
            Utilities::Color& capeColor = randomColors[0];

            modelEditor.tintPart(modelName, 1, capeColor, { 11, 23, 24, 25, 26, 27, 28, 29, 30, 31 });
            modelEditor.tintPart(modelName, 4, capeColor);
            modelEditor.tintPart(modelName, 5, capeColor, { 12, 13, 14, 15, 16 });
            modelEditor.tintPart(modelName, 8, capeColor, { 12, 13, 14, 15, 16 });
        }
    }
}