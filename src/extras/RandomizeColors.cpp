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

// Give each model 16 colors each to future proof against later changes
#define COLORS_PER_MODEL 16

// This was found using the PatternMemoryMonitor looking for decreasing values when leaving a field
// and going onto the world map. I thought this was a screen fade value because it jumps to 34 then
// decreases down to 0 however when we leave the northern crater it actually doesn't jump in value
// until we land the airship, then it goes down to 0. It works for our use case but I don't know what
// it actually is.
#define WORLD_TRIGGER_ADDR 0x9C65C

#define BATTLE_TRIGGER_ADDR 0xF39E8

void RandomizeColors::setup()
{
    BIND_EVENT(game->onStart, RandomizeColors::onStart);
    BIND_EVENT_ONE_ARG(game->onModuleChanged, RandomizeColors::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeColors::onFieldChanged);
    BIND_EVENT(game->onBattleEnter, RandomizeColors::onBattleEnter);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeColors::onFrame);

    debugStartNum[0] = '\0';
    debugCount[0] = '\0';

    modelEditor.setup(game);
}

void RandomizeColors::onDebugGUI()
{
    uint16_t fieldTrigger = game->read<uint16_t>(GameOffsets::ScreenFade);
    std::string fieldTriggerStr = "Field Trigger: " + std::to_string(fieldTrigger);
    ImGui::Text(fieldTriggerStr.c_str());

    uint8_t worldTrigger = game->read<uint8_t>(WORLD_TRIGGER_ADDR);
    std::string worldTriggerStr = "World Trigger: " + std::to_string(worldTrigger);
    ImGui::Text(worldTriggerStr.c_str());

    uint16_t battleTrigger = game->read<uint16_t>(BATTLE_TRIGGER_ADDR);
    std::string battleTriggerStr = "Battle Trigger: " + std::to_string(battleTrigger);
    ImGui::Text(battleTriggerStr.c_str());

    if (ImGui::Button("Force Update", ImVec2(120, 0)))
    {
        waitingForField = true;
        lastFieldID = -1;
        waitingForWorld = true;
        waitingForBattle = true;
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
                uintptr_t addr = debugAddresses[clickedIndex];
                game->write<uint8_t>(addr + 0, 0xFF);
                game->write<uint8_t>(addr + 1, 0x00);
                game->write<uint8_t>(addr + 2, 0xFF);

                LOG("Clicked color %zu %d: RGB(%d, %d, %d)", clickedIndex, addr, clickedColor.r, clickedColor.g, clickedColor.b);
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
    lastFieldTrigger = 0;
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

void RandomizeColors::onModuleChanged(uint8_t newModule)
{
    if (newModule == GameModule::World)
    {
        lastWorldTrigger = game->read<uint8_t>(WORLD_TRIGGER_ADDR);
        waitingForWorld = true;
    }
}

void RandomizeColors::onFieldChanged(uint16_t fieldID)
{
    lastFieldTrigger = game->read<uint16_t>(GameOffsets::ScreenFade);
    waitingForField = true;
}

void RandomizeColors::onBattleEnter()
{
    lastBattleTrigger = game->read<uint16_t>(BATTLE_TRIGGER_ADDR);
    waitingForBattle = true;
}

void RandomizeColors::onFrame(uint32_t frameNumber)
{
    uint8_t gameModule = game->getGameModule();

    if (gameModule == GameModule::Field && waitingForField)
    {
        uint16_t fieldTrigger = game->read<uint16_t>(GameOffsets::ScreenFade);

        // Update if we've hit peak fade out and started coming back down
        // or if the last field is unset.
        bool shouldUpdate = (lastFieldTrigger == 0x100 && fieldTrigger < lastFieldTrigger);
        shouldUpdate |= (lastFieldID == -1);

        if (shouldUpdate)
        {
            modelEditor.findFieldModels();
            applyColors();

            waitingForField = false;
            lastFieldID = game->getFieldID();
        }

        lastFieldTrigger = fieldTrigger;
    }

    if (gameModule == GameModule::World && waitingForWorld)
    {
        uint8_t worldTrigger = game->read<uint8_t>(WORLD_TRIGGER_ADDR);
        
        // Update if the world trigger value was higher than 10 and is now 
        // lower than 10. In practice this seems to be enough buffer to ensure
        // the model has been loaded.
        if (lastWorldTrigger >= 10 && worldTrigger < 10)
        {
            modelEditor.findFieldModels();
            applyColors();

            waitingForWorld = false;
        }

        lastWorldTrigger = worldTrigger;
    }

    if (gameModule == GameModule::Battle && waitingForBattle)
    {
        uint16_t battleTrigger = game->read<uint16_t>(BATTLE_TRIGGER_ADDR);
        
        if (lastBattleTrigger < battleTrigger && battleTrigger > 50)
        {
            modelEditor.openBattleModels();
            applyColors();
            waitingForBattle = false;
        }

        lastBattleTrigger = battleTrigger;
    }
}

void RandomizeColors::applyColors()
{
    // The parts and polys below were manually chosen using gamedata/fieldModelViewer.py in the tools directory

    uint8_t gameModule = game->getGameModule();

    if (gameModule == GameModule::Field || gameModule == GameModule::World)
    {
        std::vector<std::string> modelNames = modelEditor.getOpenModelNames();
        for (std::string& modelName : modelNames)
        {
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

    if (gameModule == GameModule::Battle)
    {
        std::vector<std::string> modelNames = modelEditor.getOpenModelNames();
        for (std::string& modelName : modelNames)
        {
            if (modelName == "CLOUD")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["CLOUD"];
                Utilities::Color& outfitColor = randomColors[0];

                modelEditor.tintPart(modelName, 0, outfitColor);
                modelEditor.tintPolys(modelName, 1, outfitColor, { 17, 20, 113, 119, 120, 121, 122, 123, 124, 125, 126, 127 });
                modelEditor.tintPolyRange(modelName, 1, outfitColor, 43, 107 );
                modelEditor.tintPart(modelName, 9, outfitColor);
                modelEditor.tintPolyRange(modelName, 10, outfitColor, 2, 49);
                modelEditor.tintPart(modelName, 13, outfitColor);
                modelEditor.tintPolyRange(modelName, 14, outfitColor, 2, 49);
            }

            if (modelName == "BARRETT")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["BALLET"];
                Utilities::Color& pantsColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                modelEditor.tintPolyRange(modelName, 0, pantsColor, 0, 25);
                modelEditor.tintPolys(modelName, 0, pantsColor, { 34, 35, 36 });
                modelEditor.tintPart(modelName, 8, pantsColor);
                modelEditor.tintPolyRange(modelName, 9, pantsColor, 10, 37);
                modelEditor.tintPolys(modelName, 9, pantsColor, { 44 });
                modelEditor.tintPart(modelName, 12, pantsColor);
                modelEditor.tintPolyRange(modelName, 13, pantsColor, 10, 37);
                modelEditor.tintPolys(modelName, 13, pantsColor, { 44 });

                modelEditor.tintPolyRange(modelName, 1, shirtColor, 40, 195);
            }

            if (modelName == "TIFA")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["TIFA"];
                Utilities::Color& skirtColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                modelEditor.tintPolyRange(modelName, 0, shirtColor, 51, 145);
                modelEditor.tintPolyRange(modelName, 0, shirtColor, 195, 199);

                modelEditor.tintPart(modelName, 7, skirtColor);
                modelEditor.tintPart(modelName, 8, skirtColor);
                modelEditor.tintPart(modelName, 13, skirtColor);
            }
        }
    }
}