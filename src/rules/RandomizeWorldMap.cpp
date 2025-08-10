#include "RandomizeWorldMap.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"

#include <random>

REGISTER_RULE("Randomize World Map", RandomizeWorldMap)

void RandomizeWorldMap::setup()
{
    BIND_EVENT(game->onStart, RandomizeWorldMap::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeWorldMap::onFrame);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeWorldMap::onFieldChanged);

    GameData::worldMapEntrances.clear();

    ADD_WORLDMAP_ENTRANCE(0x5436, 0x01, 185570, 120050);
    ADD_WORLDMAP_ENTRANCE(0x546c, 0x02, 202382, 111542);
    ADD_WORLDMAP_ENTRANCE(0x54a2, 0x03, 244049, 132736);
    ADD_WORLDMAP_ENTRANCE(0x558c, 0x04, 219198, 148615);
    ADD_WORLDMAP_ENTRANCE(0x55c2, 0x05, 206458, 160336);
    ADD_WORLDMAP_ENTRANCE(0x55f8, 0x06, 202304, 170568);
    ADD_WORLDMAP_ENTRANCE(0x562e, 0x07, 170941, 145274);
    ADD_WORLDMAP_ENTRANCE(0x5670, 0x08, 165895, 186365);
    ADD_WORLDMAP_ENTRANCE(0x56a6, 0x09, 198982, 133190);
    ADD_WORLDMAP_ENTRANCE(0x56dc, 0x0A, 124157, 178432);
    ADD_WORLDMAP_ENTRANCE(0x5712, 0x0B, 218072, 203773);
    ADD_WORLDMAP_ENTRANCE(0x5768, 0x0C, 271186, 143196);
    ADD_WORLDMAP_ENTRANCE(0x579e, 0x0D, 141220, 124482);
    ADD_WORLDMAP_ENTRANCE(0x57e0, 0x0E, 113555, 121177);
    ADD_WORLDMAP_ENTRANCE(0x5816, 0x0F, 109018, 125721);
    ADD_WORLDMAP_ENTRANCE(0x5866, 0x10, 121350, 153151);
    ADD_WORLDMAP_ENTRANCE(0x5884, 0x11, 108910, 186642);
    ADD_WORLDMAP_ENTRANCE(0x58ba, 0x12, 88317, 169166);
    ADD_WORLDMAP_ENTRANCE(0x58f0, 0x13, 95332, 133489);
    ADD_WORLDMAP_ENTRANCE(0x5926, 0x14, 87967, 116554);
    ADD_WORLDMAP_ENTRANCE(0x597c, 0x15, 100214, 143154);
    ADD_WORLDMAP_ENTRANCE(0x59b2, 0x16, 117215, 108068);
    ADD_WORLDMAP_ENTRANCE(0x59e8, 0x17, 39324, 87078);
    ADD_WORLDMAP_ENTRANCE(0x5a90, 0x18, 54567, 127403);
    ADD_WORLDMAP_ENTRANCE(0x5ac6, 0x19, 163016, 82661);
    ADD_WORLDMAP_ENTRANCE(0x5afc, 0x1A, 161539, 68127);
    ADD_WORLDMAP_ENTRANCE(0x5b32, 0x1B, 129706, 71665);
    ADD_WORLDMAP_ENTRANCE(0x5b68, 0x1C, 144109, 72407);
    ADD_WORLDMAP_ENTRANCE(0x5b9e, 0x1D, 267145, 9773);
    ADD_WORLDMAP_ENTRANCE(0x5238, 0x20, 218473, 149471);
    ADD_WORLDMAP_ENTRANCE(0x5286, 0x22, 49838, 159447);
    ADD_WORLDMAP_ENTRANCE(0x5bd4, 0x2B, 94857, 133349);
    ADD_WORLDMAP_ENTRANCE(0x5c0a, 0x2C, 93129, 126853);
    ADD_WORLDMAP_ENTRANCE(0x5c40, 0x2E, 83294, 137231);
    ADD_WORLDMAP_ENTRANCE(0x5c76, 0x2F, 129326, 71724);
    ADD_WORLDMAP_ENTRANCE(0x5cbc, 0x30, 135443, 54938);
    ADD_WORLDMAP_ENTRANCE(0x5cda, 0x39, 160085, 89266);
    ADD_WORLDMAP_ENTRANCE(0x5d10, 0x3A, 163057, 80131);
}

void RandomizeWorldMap::onStart()
{
    // We break entrances up into groups and randomize among them
    // to prevent randomizing to places you can't get to.
    entranceGroups.push_back({ 0x01, 0x02, 0x03, 0x04 });   // Midgar, Kalm, Chocobo Ranch, Mithril Mine

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
}

float getDistance(int x1, int y1, int x2, int y2) 
{
    int64_t dx = static_cast<int64_t>(x2) - x1;
    int64_t dy = static_cast<int64_t>(y2) - y1;
    return std::sqrt(static_cast<float>(dx * dx + dy * dy));
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
        WorldMapEntrance& entrance = GameData::worldMapEntrances[i];
        float dist = getDistance(worldX, worldZ, entrance.centerX, entrance.centerZ);

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
                uintptr_t oldEentScriptStart = WorldOffsets::ScriptStart + oldEntrance.offset;
                game->write<uint16_t>(oldEentScriptStart, lastCmd0);
                game->write<uint16_t>(oldEentScriptStart + 2, lastCmd1);
            }

            // Only overwrite the script if we actually got a random index, otherwise it'll spinlock.
            uint16_t randomEntIndex = getRandomEntrance(closestIndex);
            WorldMapEntrance& randEntrance = GameData::worldMapEntrances[randomEntIndex];
            if (randomEntIndex != closestIndex)
            {
                uintptr_t randEntScriptStart = WorldOffsets::ScriptStart + randEntrance.offset;
                LOG("JUMP TO ADDRESS: %d", randEntScriptStart);
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