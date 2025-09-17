#include "RandomizeWorldMap.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>

REGISTER_RULE("Randomize World Map", RandomizeWorldMap)

void RandomizeWorldMap::setup()
{
    BIND_EVENT(game->onStart, RandomizeWorldMap::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeWorldMap::onFrame);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeWorldMap::onFieldChanged);
}

void RandomizeWorldMap::onDebugGUI()
{
    if (game->getGameModule() != GameModule::World)
    {
        ImGui::Text("Not currently on world map.");
        return;
    }

    // Display closest entrance
    {
        int worldX = game->read<int>(WorldOffsets::WorldX);
        int worldZ = game->read<int>(WorldOffsets::WorldZ);

        // Find nearest entrance to the player
        float closestDistance = FLT_MAX;
        int closestIndex = -1;
        for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
        {
            // Skip Zolom entrance
            if (i == 29)
            {
                continue;
            }

            WorldMapEntrance& entrance = GameData::worldMapEntrances[i];
            float dist = Utilities::getDistance(worldX, worldZ, entrance.centerX, entrance.centerZ);

            if (dist < closestDistance)
            {
                closestDistance = dist;
                closestIndex = i;
            }
        }

        WorldMapEntrance& closestEntrance = GameData::worldMapEntrances[closestIndex];
        std::string debugText = "Closest Entrance " + std::to_string(closestIndex) + " Field ID: " + std::to_string(closestEntrance.fieldID);
        ImGui::Text(debugText.c_str());
    }
}

void RandomizeWorldMap::onStart()
{
    // We break entrances up into groups and randomize among them
    // to prevent randomizing to places you can't get to.
    entranceGroups.push_back({ 0x01, 0x02, 0x03, 0x04 });   // Midgar, Kalm, Chocobo Ranch, Mithril Mine
    entranceGroups.push_back({ 0x05, 0x06, 0x07 }); // Mithril Mine, Fort Condor, Junon
    entranceGroups.push_back({ 0x0D, 0x0E }); // Costa Del Sol, North Corel

    randomizedEntrances.clear();
    for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
    {
        randomizedEntrances[i] = i;
    }

    // Random generator
    std::random_device rd;
    std::mt19937 rng(game->getSeed());

    for (int i = 0; i < entranceGroups.size(); ++i)
    {
        std::set<uint16_t>& group = entranceGroups[i];

        std::vector<int> groupKeys;
        std::vector<int> groupValues;

        for (int j = 0; j < GameData::worldMapEntrances.size(); ++j)
        {
            WorldMapEntrance& entrance = GameData::worldMapEntrances[j];
            if (group.count(entrance.fieldID) > 0)
            {
                groupKeys.push_back(j);
                groupValues.push_back(j);
            }
        }

        std::shuffle(groupValues.begin(), groupValues.end(), rng);
        for (int j = 0; j < groupKeys.size(); ++j)
        {
            randomizedEntrances[groupKeys[j]] = groupValues[j];

            WorldMapEntrance& entrance1 = GameData::worldMapEntrances[groupKeys[j]];
            WorldMapEntrance& entrance2 = GameData::worldMapEntrances[groupValues[j]];

            LOG("World Map Entrance %d (%d) -> %d (%d)", entrance1.fieldID, groupKeys[j], entrance2.fieldID, groupValues[j]);
        }
    }

    // Hack: disable the dead zolom scene to simplify things
    uint8_t seenZolom = game->read<uint8_t>(0x9D457);
    seenZolom |= (1u << 3);
    game->write<uint8_t>(0x9D457, seenZolom);
}

uint16_t getJumpAddress(uintptr_t memAddress)
{
    uintptr_t jumpStart = memAddress - WorldOffsets::JumpStart;
    uint16_t jumpValue = (uint16_t)jumpStart / 2;

    return jumpValue;
}

void RandomizeWorldMap::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::World)
    {
        lastClosestIndex = -1;
        return;
    }

    int worldX = game->read<int>(WorldOffsets::WorldX);
    int worldZ = game->read<int>(WorldOffsets::WorldZ);

    // Find nearest entrance to the player
    float closestDistance = FLT_MAX;
    int closestIndex = -1;
    for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
    {
        // Skip Zolom entrance
        if (i == 29)
        {
            continue;
        }

        WorldMapEntrance& entrance = GameData::worldMapEntrances[i];
        float dist = Utilities::getDistance(worldX, worldZ, entrance.centerX, entrance.centerZ);

        if (dist < closestDistance)
        {
            closestDistance = dist;
            closestIndex = i;
        }
    }

    // Only update when the closest entrance changes.
    if (closestIndex != -1 && closestIndex != lastClosestIndex)
    {
        WorldMapEntrance& entrance = GameData::worldMapEntrances[closestIndex];
        uintptr_t entScriptStart = WorldOffsets::ScriptStart + entrance.offset;

        uint16_t cmd0 = game->read<uint16_t>(entScriptStart);
        uint16_t cmd1 = game->read<uint16_t>(entScriptStart + 2);
        if (cmd0 == 0x100)
        {
            // Undo the randomization to the previous entrance so we can't get caught in a loop.
            if (lastClosestIndex >= 0)
            {
                WorldMapEntrance& oldEntrance = GameData::worldMapEntrances[lastClosestIndex];
                uintptr_t oldEntScriptStart = WorldOffsets::ScriptStart + oldEntrance.offset;
                game->write<uint16_t>(oldEntScriptStart, lastCmd0);
                game->write<uint16_t>(oldEntScriptStart + 2, lastCmd1);
            }

            // Only overwrite the script if we actually got a random index, otherwise it'll spinlock.
            uint16_t randomEntIndex = getRandomEntrance(closestIndex);
            WorldMapEntrance& randEntrance = GameData::worldMapEntrances[randomEntIndex];
            if (randomEntIndex != closestIndex)
            {
                uintptr_t randEntScriptStart = WorldOffsets::ScriptStart + randEntrance.offset;
                uint16_t jumpValue = getJumpAddress(randEntScriptStart);
                game->write<uint16_t>(entScriptStart, 0x200);
                game->write<uint16_t>(entScriptStart + 2, jumpValue);
            }

            lastClosestIndex = closestIndex;
            lastCmd0 = cmd0;
            lastCmd1 = cmd1;

            WorldMapEntrance& origEntrance = GameData::worldMapEntrances[closestIndex];
            LOG("Randomized entrance %d to %d", origEntrance.fieldID, randEntrance.fieldID);
        }
    }
}

uint16_t findWorldEntranceIndex(uint16_t fieldID)
{
    for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
    {
        WorldMapEntrance& entrance = GameData::worldMapEntrances[i];
        if (entrance.fieldID == fieldID)
        {
            return i;
        }
    }

    return 0;
}

void RandomizeWorldMap::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    for (int i = 0; i < fieldData.worldExits.size(); ++i)
    {
        FieldWorldExit& exit = fieldData.worldExits[i];
        uint16_t exitIndex = findWorldEntranceIndex(exit.fieldID);
        
        int randIndex = exitIndex;
        for (auto entry : randomizedEntrances)
        {
            if (entry.second == exitIndex)
            {
                randIndex = entry.first;
            }
        }

        if (randIndex != exitIndex)
        {
            WorldMapEntrance& randEntrance = GameData::worldMapEntrances[randIndex];

            uintptr_t fieldIDOffset = FieldScriptOffsets::TriggersStart + exit.offset;
            game->write<uint16_t>(fieldIDOffset, randEntrance.fieldID);

            LOG("Randomized field exit from %d to %d", exit.fieldID, randEntrance.fieldID);
        }
    }
}

uint16_t RandomizeWorldMap::getRandomEntrance(uint16_t entranceIndex)
{
    if (randomizedEntrances.count(entranceIndex) == 0)
    {
        return entranceIndex;
    }

    uint16_t randomEntIndex = randomizedEntrances[entranceIndex];
    return randomEntIndex;
}