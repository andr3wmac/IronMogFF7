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

    if (ImGui::Button("Force Update", ImVec2(120, 0)))
    {
        waitingForField = true;
        lastFieldID = -1;
        waitingForWorld = true;
        waitingForBattle = true;
    }

    if (ImGui::CollapsingHeader("Current Models"))
    {
        std::vector<ModelEditor::ModelEditorModel> openModels = modelEditor.getOpenModels();
        for (int i = 0; i < openModels.size(); ++i)
        {
            ImGui::Text(openModels[i].modelName.c_str());
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

                DEBUG_LOG("Clicked color %zu %d: RGB(%d, %d, %d)", clickedIndex, addr, clickedColor.r, clickedColor.g, clickedColor.b);
            });
    }
}

Utilities::Color getRandomColor(std::mt19937& rng)
{
    /*
    std::uniform_int_distribution<uint32_t> dist(0, 255);

    Utilities::Color color;
    color.r = dist(rng);
    color.g = dist(rng);
    color.b = dist(rng);
    return color;
    */

    // Uniformly pick any color on the rainbow
    std::uniform_real_distribution<float> hueDist(0.0f, 360.0f);

    // Keep saturation high so colors aren't gray/muddy
    std::uniform_real_distribution<float> satDist(0.6f, 0.9f);

    // Keep value high so colors aren't black/dim
    std::uniform_real_distribution<float> valDist(0.7f, 1.0f);

    float h = hueDist(rng);
    float s = satDist(rng);
    float v = valDist(rng);

    return Utilities::HSVtoRGB(h, s, v);
}

bool RandomizeColors::onSettingsGUI()
{
    ImGui::BeginDisabled(game == nullptr);
    if (ImGui::Button("Reroll Colors", ImVec2(120, 0)))
    {
        rerollOffset++;
        std::mt19937 rng(game->getSeed() + rerollOffset);
        randomModelColors.clear();

        // Generate table of random colors
        for (Model& model : GameData::models)
        {
            std::string& modelName = model.name;
            for (int i = 0; i < COLORS_PER_MODEL; ++i)
            {
                randomModelColors[modelName].push_back(getRandomColor(rng));
            }
        }

        applyColors();
    }
    ImGui::EndDisabled();

    return false;
}

void RandomizeColors::onStart()
{
    lastFieldTrigger = 0;
    lastFieldID = -1;
    waitingForField = true;

    std::mt19937 rng(game->getSeed() + rerollOffset);

    // Generate table of random colors
    randomModelColors.clear();
    for (Model& model : GameData::models)
    {
        std::string& modelName = model.name;
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

        // Hack fix for base of tower transition after wedge falls
        if (game->getFieldID() == 156 && game->getGameMoment() == 218 && lastFieldTrigger == 256)
        {
            // Waiting 240 frames was chosen arbitrarily and tested. Could be fragile.
            if (game->getFramesInField() == 240)
            {
                shouldUpdate = true;
            }
        }

        // Hack fix for tifa and cloud scene before northern crater
        if (game->getFieldID() == 771 && game->getGameMoment() == 1612 && lastFieldTrigger > 1 && lastFieldTrigger < 120)
        {
            shouldUpdate = true;
        }

        if (shouldUpdate)
        {
            modelEditor.findFieldModels();
            applyColors();

            waitingForField = false;
            lastFieldID = game->getFieldID();
        }

        lastFieldTrigger = fieldTrigger;
    }

    // Hack fix for aerith forest scene after demons gate.
    // Her model seems to have the colors reloaded after they do a bright white
    // effect to it. Its reloaded behind the tree, so we detect when her models
    // at the position behind the tree then reapply coloring.
    if (gameModule == GameModule::Field && !waitingForField)
    {
        if (game->getFieldID() == 618 && game->getGameMoment() < 638)
        {
            int32_t aerithPositionX = game->read<int32_t>(0x7503C);
            int32_t aerithPositionY = game->read<int32_t>(0x75040);
            if (aerithPositionX > 975000 && aerithPositionY == -499712)
            {
                applyColors();
            }
        }
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
        if (modelEditor.areBattleModelsLoaded())
        {
            modelEditor.openBattleModels();
            applyColors();
            waitingForBattle = false;
        }
    }
}

void RandomizeColors::applyColors()
{
    // The parts and polys below were manually chosen using gamedata/modelViewer.py in the tools directory

    uint8_t gameModule = game->getGameModule();

    if (gameModule == GameModule::Field || gameModule == GameModule::World)
    {
        std::vector<ModelEditor::ModelEditorModel> openModels = modelEditor.getOpenModels();
        for (int i = 0; i < openModels.size(); ++i)
        {
            std::string& modelName = openModels[i].modelName;

            if (modelName == "CLOUD" || modelName == "CLOUD_WORLD" || modelName == "CLOUD_PARACHUTE")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["CLOUD"];
                Utilities::Color& outfitColor = randomColors[0];

                modelEditor.tintPart(i, 0, outfitColor);

                int legStart = 9;
                if (modelName == "CLOUD_PARACHUTE")
                {
                    // Exclude the parachute
                    modelEditor.tintPolyRange(i, 1, outfitColor, 0, 7);
                    modelEditor.tintPolyRange(i, 1, outfitColor, 12, 34);
                    legStart = 10;
                }
                else 
                {
                    modelEditor.tintPart(i, 1, outfitColor);
                }
                
                modelEditor.tintPart(i, legStart + 0, outfitColor);
                modelEditor.tintPart(i, legStart + 1, outfitColor, { 4, 5, 6, 7, 8, 9 });
                modelEditor.tintPart(i, legStart + 3, outfitColor);
                modelEditor.tintPart(i, legStart + 4, outfitColor, { 4, 5, 6, 7, 8, 9 });
            }

            if (modelName == "BARRET" || modelName == "BARRET_COREL" || modelName == "BARRET_PARACHUTE")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["BARRET"];
                Utilities::Color& pantsColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                int legsStart = 9;

                if (modelName == "BARRET")
                {
                    modelEditor.tintPart(i, 1, shirtColor, { 0, 1, 2, 3, 4, 52, 53, 54, 55, 64, 65, 66, 67, 68, 69, 70, 71, 79, 80, 81, 82 });
                }
                if (modelName == "BARRET_COREL")
                {
                    modelEditor.tintPolyRange(i, 1, shirtColor, 0, 49);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 48, 55);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 66, 72);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 77, 81);
                }
                if (modelName == "BARRET_PARACHUTE")
                {
                    modelEditor.tintPolyRange(i, 1, shirtColor, 8, 45);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 72, 89);
                    legsStart = 10;
                }

                modelEditor.tintPart(i, 0, pantsColor);
                modelEditor.tintPart(i, legsStart + 0, pantsColor);
                modelEditor.tintPart(i, legsStart + 1, pantsColor, { 4, 5, 6, 7, 8, 9 });
                modelEditor.tintPart(i, legsStart + 3, pantsColor);
                modelEditor.tintPart(i, legsStart + 4, pantsColor, { 4, 5, 6, 7, 8, 9 });
            }

            if (modelName == "TIFA" || modelName == "TIFA_PARACHUTE")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["TIFA"];
                Utilities::Color& skirtColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                modelEditor.tintPart(i, 0, skirtColor);
                if (modelName == "TIFA_PARACHUTE")
                {
                    modelEditor.tintPolyRange(i, 1, shirtColor, 19, 41);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 55, 60);
                }
                else
                {
                    modelEditor.tintPart(i, 1, shirtColor, { 6, 7, 8, 36, 37, 38, 39, 40, 41, 42 });
                }
            }

            if (modelName == "AERITH" || modelName == "AERITH_INTRO")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["AERITH"];
                Utilities::Color& dressColor = randomColors[0];
                Utilities::Color& jacketColor = randomColors[1];

                modelEditor.tintPart(i, 0, dressColor);
                modelEditor.tintPolys(i, 1, dressColor, { 17, 18, 19, 20, 21, 22, 23, 37, 38, 39, 40, 41, 42, 43 });
                if (modelName == "AERITH")
                {
                    modelEditor.tintPolyRange(i, 2, dressColor, 104, 112);
                }
                if (modelName == "AERITH_INTRO")
                {
                    modelEditor.tintPolyRange(i, 2, dressColor, 108, 116);
                }
                modelEditor.tintPart(i, 10, dressColor);
                modelEditor.tintPolyRange(i, 11, dressColor, 0, 5);
                modelEditor.tintPart(i, 13, dressColor);
                modelEditor.tintPolyRange(i, 14, dressColor, 0, 5);

                modelEditor.tintPolys(i, 1, dressColor, { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36 });
                modelEditor.tintPart(i, 4, jacketColor, { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 });
                modelEditor.tintPart(i, 7, jacketColor, { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 });
            }

            if (modelName == "REDXIII" || modelName == "REDXIII_PARACHUTE")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["REDXIII"];
                Utilities::Color& skinColor = randomColors[0];
                Utilities::Color& hairColor = randomColors[1];

                int bodyStart = 3;

                // Torso and Head
                modelEditor.tintPart(i, 0, skinColor);
                if (modelName == "REDXIII")
                {
                    modelEditor.tintPolys(i, 1, skinColor, { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 35, 36, 37, 38 });
                    modelEditor.tintPolyRange(i, 2, skinColor, 3, 59);
                    modelEditor.tintPolyRange(i, 2, skinColor, 120, 130);
                }
                if (modelName == "REDXIII_PARACHUTE")
                {
                    modelEditor.tintPolyRange(i, 1, skinColor, 4, 41);
                    modelEditor.tintPolyRange(i, 1, skinColor, 51, 63);
                    modelEditor.tintPolyRange(i, 3, skinColor, 3, 59);
                    modelEditor.tintPolyRange(i, 3, skinColor, 120, 130);
                    bodyStart = 4;
                }

                // Front Left Leg
                modelEditor.tintPart(i, bodyStart + 0, skinColor);
                modelEditor.tintPart(i, bodyStart + 1, skinColor);
                modelEditor.tintPart(i, bodyStart + 2, skinColor, { 7, 8, 9, 10, 11 });
                modelEditor.tintPart(i, bodyStart + 3, skinColor);

                // Front Right Leg
                modelEditor.tintPart(i, bodyStart + 4, skinColor);
                modelEditor.tintPart(i, bodyStart + 5, skinColor);
                modelEditor.tintPart(i, bodyStart + 6, skinColor, { 7, 8, 9, 10, 11 });
                modelEditor.tintPart(i, bodyStart + 7, skinColor);

                // Back Left Leg
                modelEditor.tintPart(i, bodyStart + 8, skinColor);
                modelEditor.tintPart(i, bodyStart + 9, skinColor);
                modelEditor.tintPart(i, bodyStart + 10, skinColor, { 7, 8, 9, 10, 11 });
                modelEditor.tintPart(i, bodyStart + 11, skinColor);

                // Tail
                modelEditor.tintPart(i, bodyStart + 12, skinColor);
                modelEditor.tintPart(i, bodyStart + 13, skinColor);

                // Back Right Leg
                modelEditor.tintPart(i, bodyStart + 15, skinColor);
                modelEditor.tintPart(i, bodyStart + 16, skinColor);
                modelEditor.tintPart(i, bodyStart + 17, skinColor, { 7, 8, 9, 10, 11 });
                modelEditor.tintPart(i, bodyStart + 18, skinColor);

                // Hair (Head, Torso, Tail)
                if (modelName == "REDXIII")
                {
                    modelEditor.tintPolys(i, 1, hairColor, { 0, 1, 2, 3, 28, 29, 30, 31, 32, 33, 34 });
                    modelEditor.tintPolyRange(i, 2, hairColor, 62, 99);
                }
                if (modelName == "REDXIII_PARACHUTE")
                {
                    modelEditor.tintPolys(i, 1, hairColor, { 0, 1, 2, 3, 44, 45, 46, 47, 48, 49, 50 });
                    modelEditor.tintPolyRange(i, 3, hairColor, 62, 99);
                }
                modelEditor.tintPart(i, bodyStart + 14, hairColor);
            }

            if (modelName == "CID" || modelName == "CID_PARACHUTE")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["CID"];
                Utilities::Color& pantsColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                int legsStart = 9;
                int armsStart = 3;

                if (modelName == "CID_PARACHUTE")
                {
                    modelEditor.tintPolyRange(i, 1, shirtColor, 2, 20);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 23, 43);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 58, 64);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 67, 78);

                    legsStart = 10;
                    armsStart = 4;
                }
                else 
                {
                    modelEditor.tintPolyRange(i, 1, shirtColor, 12, 35);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 43, 57);
                }

                modelEditor.tintPart(i, 0, pantsColor);
                modelEditor.tintPart(i, legsStart + 0, pantsColor);
                modelEditor.tintPart(i, legsStart + 1, pantsColor, { 4, 5, 6, 7, 8, 9, 10, 11, 20 });
                modelEditor.tintPart(i, legsStart + 3, pantsColor);
                modelEditor.tintPart(i, legsStart + 4, pantsColor, { 4, 5, 6, 7, 8, 9, 10, 11, 20 });

                modelEditor.tintPart(i, armsStart + 0, shirtColor, { 14, 15, 16, 17, 18 });
                modelEditor.tintPart(i, armsStart + 3, shirtColor, { 14, 15, 16, 17, 18 });
            }

            if (modelName == "CAITSITH")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& moogleColor = randomColors[0];

                modelEditor.tintPolyRange(i, 0, moogleColor, 19, 95);
                modelEditor.tintPolyRange(i, 0, moogleColor, 110, 137);
                modelEditor.tintPart(i, 1, moogleColor);
                modelEditor.tintPart(i, 2, moogleColor);
                modelEditor.tintPart(i, 12, moogleColor);
                modelEditor.tintPart(i, 13, moogleColor);
                modelEditor.tintPart(i, 14, moogleColor);
                modelEditor.tintPart(i, 15, moogleColor);
            }

            if (modelName == "YUFFIE" || modelName == "YUFFIE_PARACHUTE")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["YUFFIE"];
                Utilities::Color& shirtColor = randomColors[0];
                Utilities::Color& pantsColor = randomColors[1];

                int offset = 0;
                if (modelName == "YUFFIE_PARACHUTE")
                {
                    modelEditor.tintPolyRange(i, 1, shirtColor, 0, 13);
                    modelEditor.tintPolyRange(i, 1, shirtColor, 34, 52);
                    offset = 1;
                }
                else 
                {
                    modelEditor.tintPart(i, 1, shirtColor, { 12, 13, 14, 15, 16, 17, 35, 36, 37, 38, 39, 40 });
                }

                modelEditor.tintPart(i, 1, shirtColor, { 12, 13, 14, 15, 16, 17, 35, 36, 37, 38, 39, 40 });
                modelEditor.tintPart(i, 3 + offset, shirtColor);
                modelEditor.tintPart(i, 4 + offset, shirtColor);

                modelEditor.tintPart(i, 0, pantsColor);
                modelEditor.tintPart(i, 11 + offset, pantsColor, { 6, 7, 8, 9, 10 });
                modelEditor.tintPart(i, 14 + offset, pantsColor, { 6, 7, 8, 9, 10 });

                // Shield
                modelEditor.tintPolyRange(i, 5 + offset, pantsColor, 17, 26);
                modelEditor.tintPart(i, 6 + offset, pantsColor, { 0, 1, 2 });
            }

            if (modelName == "VINCENT")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& capeColor = randomColors[0];

                modelEditor.tintPart(i, 1, capeColor, { 11, 23, 24, 25, 26, 27, 28, 29, 30, 31 });
                modelEditor.tintPart(i, 4, capeColor);
                modelEditor.tintPart(i, 5, capeColor, { 12, 13, 14, 15, 16 });
                modelEditor.tintPart(i, 8, capeColor, { 12, 13, 14, 15, 16 });
            }

            if (modelName == "CLOUD_DRESS")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& dressColor = randomColors[0];

                modelEditor.tintPart(i, 0, dressColor);
                modelEditor.tintPolyRange(i, 1, dressColor, 2, 7);
                modelEditor.tintPolyRange(i, 1, dressColor, 27, 38);
                modelEditor.tintPart(i, 4, dressColor);
                modelEditor.tintPart(i, 5, dressColor);
                modelEditor.tintPart(i, 7, dressColor);
                modelEditor.tintPart(i, 8, dressColor);
                modelEditor.tintPart(i, 10, dressColor);
                modelEditor.tintPolyRange(i, 11, dressColor, 6, 11);
                modelEditor.tintPart(i, 13, dressColor);
                modelEditor.tintPolyRange(i, 14, dressColor, 6, 11);
            }

            if (modelName == "AERITH_DRESS")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& dressColor = randomColors[0];

                modelEditor.tintPart(i, 0, dressColor);
                modelEditor.tintPolyRange(i, 1, dressColor, 41, 57);
                modelEditor.tintPolyRange(i, 1, dressColor, 62, 76);
                modelEditor.tintPolyRange(i, 2, dressColor, 8, 23);
                modelEditor.tintPolyRange(i, 3, dressColor, 4, 7);
                modelEditor.tintPolyRange(i, 3, dressColor, 135, 143);
                modelEditor.tintPart(i, 11, dressColor);
                modelEditor.tintPolyRange(i, 12, dressColor, 0, 8);
                modelEditor.tintPolyRange(i, 13, dressColor, 5, 22);
                modelEditor.tintPart(i, 14, dressColor);
                modelEditor.tintPolyRange(i, 15, dressColor, 0, 8);
                modelEditor.tintPolyRange(i, 16, dressColor, 5, 22);
            }

            if (modelName == "TIFA_DRESS")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& dressColor = randomColors[0];

                modelEditor.tintPolyRange(i, 0, dressColor, 0, 3);
                modelEditor.tintPolyRange(i, 0, dressColor, 8, 26);
                modelEditor.tintPolyRange(i, 1, dressColor, 34, 69);
                modelEditor.tintPolyRange(i, 1, dressColor, 72, 82);
                modelEditor.tintPolyRange(i, 2, dressColor, 149, 152);
                modelEditor.tintPolyRange(i, 12, dressColor, 5, 22);
                modelEditor.tintPolyRange(i, 15, dressColor, 5, 22);
            }

            if (modelName == "CLOUD_SWORD")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["CLOUD"];
                Utilities::Color& outfitColor = randomColors[0];

                modelEditor.tintPart(i, 0, outfitColor);
                modelEditor.tintPart(i, 1, outfitColor);
                modelEditor.tintPart(i, 10, outfitColor);
                modelEditor.tintPart(i, 11, outfitColor, { 4, 5, 6, 7, 8, 9 });
                modelEditor.tintPart(i, 13, outfitColor);
                modelEditor.tintPart(i, 14, outfitColor, { 4, 5, 6, 7, 8, 9 });
            }

            if (modelName == "CLOUD_WHEELCHAIR")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["CLOUD"];
                Utilities::Color& outfitColor = randomColors[0];

                modelEditor.tintPart(i, 0, outfitColor);
                modelEditor.tintPart(i, 1, outfitColor);
                modelEditor.tintPart(i, 9, outfitColor);
                modelEditor.tintPolyRange(i, 10, outfitColor, 3, 5);
                modelEditor.tintPolyRange(i, 10, outfitColor, 19, 26);
                modelEditor.tintPart(i, 19, outfitColor);
                modelEditor.tintPart(i, 20, outfitColor, { 4, 5, 6, 7, 8, 9 });
            }
        }
    }

    if (gameModule == GameModule::Battle)
    {
        std::vector<ModelEditor::ModelEditorModel> openModels = modelEditor.getOpenModels();
        for (int i = 0; i < openModels.size(); ++i)
        {
            std::string& modelName = openModels[i].modelName;

            if (modelName == "CLOUD")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& outfitColor = randomColors[0];

                modelEditor.tintPart(i, 0, outfitColor);
                modelEditor.tintPolys(i, 1, outfitColor, { 17, 20, 113, 119, 120, 121, 122, 123, 124, 125, 126, 127 });
                modelEditor.tintPolyRange(i, 1, outfitColor, 43, 107 );
                modelEditor.tintPart(i, 9, outfitColor);
                modelEditor.tintPolyRange(i, 10, outfitColor, 2, 49);
                modelEditor.tintPart(i, 13, outfitColor);
                modelEditor.tintPolyRange(i, 14, outfitColor, 2, 49);
            }

            if (modelName == "HICLOUD")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors["CLOUD"];
                Utilities::Color& outfitColor = randomColors[0];

                modelEditor.tintPart(i, 0, outfitColor);
                modelEditor.tintPart(i, 1, outfitColor);
                modelEditor.tintPolyRange(i, 2, outfitColor, 444, 448);
                modelEditor.tintPolyRange(i, 2, outfitColor, 450, 469);
                modelEditor.tintPolyRange(i, 2, outfitColor, 526, 537);
                modelEditor.tintPart(i, 9, outfitColor);
                modelEditor.tintPolyRange(i, 10, outfitColor, 19, 108);
                modelEditor.tintPolyRange(i, 10, outfitColor, 130, 137);
                modelEditor.tintPart(i, 13, outfitColor);
                modelEditor.tintPolyRange(i, 14, outfitColor, 19, 108);
                modelEditor.tintPolyRange(i, 14, outfitColor, 130, 137);
            }

            if (modelName == "BARRET")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& pantsColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                modelEditor.tintPolyRange(i, 0, pantsColor, 0, 25);
                modelEditor.tintPolys(i, 0, pantsColor, { 34, 35, 36 });
                modelEditor.tintPart(i, 8, pantsColor);
                modelEditor.tintPolyRange(i, 9, pantsColor, 10, 37);
                modelEditor.tintPolys(i, 9, pantsColor, { 44 });
                modelEditor.tintPart(i, 12, pantsColor);
                modelEditor.tintPolyRange(i, 13, pantsColor, 10, 37);
                modelEditor.tintPolys(i, 13, pantsColor, { 44 });

                modelEditor.tintPolyRange(i, 1, shirtColor, 40, 195);
            }

            if (modelName == "TIFA")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& skirtColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                modelEditor.tintPolyRange(i, 0, shirtColor, 51, 145);
                modelEditor.tintPolyRange(i, 0, shirtColor, 195, 199);

                modelEditor.tintPart(i, 7, skirtColor);
                modelEditor.tintPart(i, 8, skirtColor);
                modelEditor.tintPart(i, 13, skirtColor);
            }

            if (modelName == "AERITH")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& dressColor = randomColors[0];
                Utilities::Color& jacketColor = randomColors[1];

                modelEditor.tintPart(i, 0, dressColor);
                modelEditor.tintPolyRange(i, 1, dressColor, 0, 34);
                modelEditor.tintPolyRange(i, 1, dressColor, 98, 99);
                modelEditor.tintPolyRange(i, 2, dressColor, 242, 250);
                modelEditor.tintPart(i, 13, dressColor);
                modelEditor.tintPolyRange(i, 14, dressColor, 0, 21);
                modelEditor.tintPolys(i, 14, dressColor, { 23, 25, 32, 33, 34, 35, 36, 37, 38 });
                modelEditor.tintPart(i, 17, dressColor);
                modelEditor.tintPart(i, 18, dressColor);
                modelEditor.tintPart(i, 19, dressColor);
                modelEditor.tintPolyRange(i, 20, dressColor, 0, 21);
                modelEditor.tintPolys(i, 20, dressColor, { 23, 25, 32, 33, 34, 35, 36, 37, 38 });

                modelEditor.tintPolyRange(i, 1, jacketColor, 35, 83);
                modelEditor.tintPolyRange(i, 1, jacketColor, 100, 104);
                modelEditor.tintPolyRange(i, 7, jacketColor, 0, 17);
                modelEditor.tintPolyRange(i, 7, jacketColor, 22, 29);
                modelEditor.tintPolyRange(i, 10, jacketColor, 0, 17);
                modelEditor.tintPolyRange(i, 10, jacketColor, 22, 29);
            }

            if (modelName == "CID")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& pantsColor = randomColors[0];
                Utilities::Color& shirtColor = randomColors[1];

                modelEditor.tintPolyRange(i, 0, pantsColor, 0, 29);
                modelEditor.tintPolyRange(i, 0, pantsColor, 39, 40);
                modelEditor.tintPart(i, 11, pantsColor);
                modelEditor.tintPolyRange(i, 12, pantsColor, 4, 33);
                modelEditor.tintPart(i, 15, pantsColor);
                modelEditor.tintPolyRange(i, 16, pantsColor, 4, 33);

                modelEditor.tintPolyRange(i, 1, shirtColor, 0, 63);
                modelEditor.tintPolyRange(i, 1, shirtColor, 86, 93);
                modelEditor.tintPolyRange(i, 1, shirtColor, 106, 129);
                modelEditor.tintPolyRange(i, 1, shirtColor, 131, 132);
                modelEditor.tintPolyRange(i, 3, shirtColor, 2, 25);
                modelEditor.tintPolyRange(i, 3, shirtColor, 27, 44);
                modelEditor.tintPolys(i, 3, shirtColor, { 0, 54 });
                modelEditor.tintPart(i, 6, shirtColor);
                modelEditor.tintPart(i, 7, shirtColor);
                modelEditor.tintPolyRange(i, 8, shirtColor, 2, 23);
                modelEditor.tintPolyRange(i, 8, shirtColor, 28, 36);
                modelEditor.tintPolys(i, 8, shirtColor, { 39, 54 });
            }

            if (modelName == "CAITSITH")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& moogleColor = randomColors[0];

                modelEditor.tintPart(i, 0, moogleColor);
                modelEditor.tintPart(i, 1, moogleColor);
                modelEditor.tintPart(i, 2, moogleColor);
                modelEditor.tintPart(i, 3, moogleColor);
                modelEditor.tintPolyRange(i, 4, moogleColor, 20, 149);
                modelEditor.tintPolyRange(i, 4, moogleColor, 162, 167);
                modelEditor.tintPart(i, 5, moogleColor);
                modelEditor.tintPart(i, 6, moogleColor);
                modelEditor.tintPolyRange(i, 25, moogleColor, 14, 99);
                modelEditor.tintPolyRange(i, 26, moogleColor, 26, 55);
                modelEditor.tintPart(i, 27, moogleColor);
                modelEditor.tintPart(i, 28, moogleColor);
            }

            if (modelName == "VINCENT")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& capeColor = randomColors[0];

                modelEditor.tintPolyRange(i, 0, capeColor, 8, 83);
                modelEditor.tintPolys(i, 0, capeColor, { 100, 114, 115, 116 });
                modelEditor.tintPolyRange(i, 1, capeColor, 187, 204);
                modelEditor.tintPolyRange(i, 1, capeColor, 280, 283);
                modelEditor.tintPart(i, 7, capeColor);
                modelEditor.tintPart(i, 8, capeColor);
                modelEditor.tintPart(i, 9, capeColor);
                modelEditor.tintPart(i, 10, capeColor);
                modelEditor.tintPart(i, 11, capeColor);
                modelEditor.tintPart(i, 12, capeColor);
                modelEditor.tintPart(i, 13, capeColor);
                modelEditor.tintPart(i, 14, capeColor);
                modelEditor.tintPart(i, 15, capeColor);
                modelEditor.tintPart(i, 16, capeColor);
                modelEditor.tintPart(i, 17, capeColor);
                modelEditor.tintPart(i, 18, capeColor);
                modelEditor.tintPolyRange(i, 28, capeColor, 0, 31);
                modelEditor.tintPolys(i, 28, capeColor, { 33, 34, 35, 37 });
                modelEditor.tintPolyRange(i, 31, capeColor, 0, 31);
            }

            if (modelName == "REDXIII")
            {
                std::vector<Utilities::Color> randomColors = randomModelColors[modelName];
                Utilities::Color& skinColor = randomColors[0];
                Utilities::Color& hairColor = randomColors[1];

                modelEditor.tintPart(i, 0, skinColor);
                modelEditor.tintPolyRange(i, 1, skinColor, 0, 69);
                modelEditor.tintPolys(i, 1, skinColor, { 74 });
                modelEditor.tintPolyRange(i, 2, skinColor, 4, 19);
                modelEditor.tintPolys(i, 2, skinColor, { 23 });
                modelEditor.tintPolyRange(i, 3, skinColor, 0, 13);
                modelEditor.tintPolyRange(i, 3, skinColor, 28, 153);
                modelEditor.tintPolys(i, 3, skinColor, { 188, 190 });
                modelEditor.tintPolyRange(i, 3, skinColor, 197, 233);
                modelEditor.tintPolyRange(i, 3, skinColor, 236, 241);
                modelEditor.tintPart(i, 10, skinColor);
                modelEditor.tintPart(i, 11, skinColor);
                modelEditor.tintPart(i, 12, skinColor);
                modelEditor.tintPolyRange(i, 13, skinColor, 0, 13);
                modelEditor.tintPolyRange(i, 13, skinColor, 24, 27);
                modelEditor.tintPart(i, 14, skinColor);
                modelEditor.tintPart(i, 15, skinColor);
                modelEditor.tintPart(i, 16, skinColor);
                modelEditor.tintPart(i, 17, skinColor);
                modelEditor.tintPolyRange(i, 18, skinColor, 0, 13);
                modelEditor.tintPolyRange(i, 18, skinColor, 24, 27);
                modelEditor.tintPart(i, 19, skinColor);
                modelEditor.tintPart(i, 20, skinColor);
                modelEditor.tintPart(i, 21, skinColor);
                modelEditor.tintPart(i, 22, skinColor);
                modelEditor.tintPolyRange(i, 23, skinColor, 0, 13);
                modelEditor.tintPolyRange(i, 23, skinColor, 24, 26);
                modelEditor.tintPart(i, 24, skinColor);
                modelEditor.tintPart(i, 25, skinColor);
                modelEditor.tintPart(i, 26, skinColor);
                modelEditor.tintPart(i, 27, skinColor);
                modelEditor.tintPart(i, 28, skinColor);
                modelEditor.tintPart(i, 29, skinColor);
                modelEditor.tintPart(i, 30, skinColor);
                modelEditor.tintPart(i, 31, skinColor);
                modelEditor.tintPart(i, 32, skinColor);
                modelEditor.tintPart(i, 33, skinColor);
                modelEditor.tintPolyRange(i, 34, skinColor, 0, 13);
                modelEditor.tintPolyRange(i, 34, skinColor, 24, 26);
                modelEditor.tintPart(i, 35, skinColor);
                modelEditor.tintPart(i, 36, skinColor);

                modelEditor.tintPolyRange(i, 1, hairColor, 70, 73);
                modelEditor.tintPolyRange(i, 1, hairColor, 75, 80);
                modelEditor.tintPolyRange(i, 2, hairColor, 0, 3);
                modelEditor.tintPolyRange(i, 2, hairColor, 20, 22);
                modelEditor.tintPolyRange(i, 3, hairColor, 154, 187);
                modelEditor.tintPart(i, 4, hairColor);
                modelEditor.tintPolyRange(i, 5, hairColor, 0, 3);
                modelEditor.tintPart(i, 6, hairColor);
                modelEditor.tintPart(i, 7, hairColor);
                modelEditor.tintPolyRange(i, 8, hairColor, 0, 3);
                modelEditor.tintPart(i, 9, hairColor);
            }
        }
    }
}