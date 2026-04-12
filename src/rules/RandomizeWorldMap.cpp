#include "RandomizeWorldMap.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>

REGISTER_RULE(RandomizeWorldMap, "Randomize World Map", "World map entrances are shuffled so entering Kalm might take you to Midgar.")

void RandomizeWorldMap::setup()
{
    BIND_EVENT(game->onStart, RandomizeWorldMap::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeWorldMap::onFrame);
    BIND_EVENT(game->onWorldMapEnter, RandomizeWorldMap::onWorldMapEnter);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeWorldMap::onFieldChanged);
    BIND_EVENT_ONE_ARG(game->onModuleChanged, RandomizeWorldMap::onModuleChanged);
    BIND_EVENT(game->onUpdate, RandomizeWorldMap::onUpdate);
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
        std::string closestText = "Closest Entrance: " + closestEntrance.fieldName + " (" + std::to_string(closestEntrance.fieldID) + ")";
        ImGui::Text(closestText.c_str());

        uint16_t randomEntIndex = getRandomEntrance(closestIndex);
        WorldMapEntrance& randomizedEntrance = GameData::worldMapEntrances[randomEntIndex];
        std::string randomizedText = "Randomized to: " + randomizedEntrance.fieldName + " (" + std::to_string(randomizedEntrance.fieldID) + ")";
        ImGui::Text(randomizedText.c_str());

        int groupIndex = -1;
        for (int i = 0; i < entranceGroups.size(); ++i)
        {
            std::set<uint16_t>& group = entranceGroups[i];
            if (group.count(closestEntrance.fieldID) > 0)
            {
                groupIndex = i;
                break;
            }
        }
        std::string groupText = "Group: " + std::to_string(groupIndex);
        ImGui::Text(groupText.c_str());
    }
}

std::vector<std::string> RandomizeWorldMap::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Randomized)
    {
        return { "World Map" };
    }

    return {};
}

void RandomizeWorldMap::onStart()
{
    // Clear state
    lastClosestIndex = -1;
    lastGameMoment = game->getGameMoment();
    entranceGroups.clear();
    randomizedEntrances.clear();

    // Clear any existing entrance randomization that may be stale.
    uint8_t gameModule = game->read<uint8_t>(GameOffsets::CurrentModule);
    if (gameModule == GameModule::World)
    {
        for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
        {
            WorldMapEntrance& entrance = GameData::worldMapEntrances[i];
            uintptr_t entScriptStart = WorldOffsets::ScriptStart + entrance.offset;

            game->write<uint16_t>(entScriptStart, 0x0100);

            if (lastClosestIndex == 29)
            {
                game->write<uint16_t>(entScriptStart + 2, 0x0114);
            }
            else if (lastClosestIndex == 30)
            {
                game->write<uint16_t>(entScriptStart + 2, 0x011c);
            }
            else
            {
                game->write<uint16_t>(entScriptStart + 2, 0x011b);
            }
        }
    }

    // We break entrances up into groups and randomize among them
    // to prevent randomizing to places you can't get to.
    // - Zolom field is excluded because its not worth the effort to make it work right.
    // - Corel Desert needs the buggy to access which gets weird so its excluded.

    entranceGroups.push_back({ 0x01, 0x02, 0x03, 0x04 });   // Midgar, Kalm, Chocobo Ranch, Mithril Mine
    entranceGroups.push_back({ 0x05, 0x06, 0x07 });         // Mithril Mine, Fort Condor, 0x39
    entranceGroups.push_back({ 0x0D, 0x0E });               // Costa Del Sol, Mount Corel
    entranceGroups.push_back({ 0x0A, 0x11, 0x12 });         // Weapon Seller, Gongaga, Cosmo Canyon
    entranceGroups.push_back({ 0x14, 0x2E });               // Rocket Town, Mount Nibel
    entranceGroups.push_back({ 0x08, 0x17, 0x19 });         // Temple of Ancients, Wutai, Bone Village
    entranceGroups.push_back({ 0x0B, 0x1C });               // Mideel, Mystery House
    entranceGroups.push_back({ 0x0C, 0x16, 0x18, 0x1D });   // Materia Caves

    for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
    {
        randomizedEntrances[i] = i;
    }

    // Random generator
    uint32_t seed = game->getSeed();
    std::random_device rd;
    std::mt19937 rng(seed);

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

            // Only log if we haven't already for this seed, otherwise this is just log spam.
            if (lastLoggedSeed != seed)
            {
                LOG("World Map Entrance %s (%d) -> %s (%d)", entrance1.fieldName.c_str(), entrance1.fieldID, entrance2.fieldName.c_str(), entrance2.fieldID);
            }
        }
    }

    // Hack: disable the dead zolom scene to simplify things
    uint8_t seenZolom = game->read<uint8_t>(0x9D457);
    seenZolom |= (1u << 3);
    game->write<uint8_t>(0x9D457, seenZolom);

    lastLoggedSeed = seed;
}

void RandomizeWorldMap::onFrame(uint32_t frameNumber)
{
    uint16_t currentGameMoment = game->getGameMoment();
    if (lastGameMoment < 1299 && currentGameMoment == 1299)
    {
        // When we get the submarine its going to put us into an area where we 
        // may not be able to get to the highwind due to world map randomization.
        // To get around this we move the highwind to be next to junon.

        uintptr_t offset = SavemapOffsets::BuggyHighwindPosition;
        game->write<uint32_t>(offset + 0, 1931124268);
        game->write<uint32_t>(offset + 4, 81415094);
    }
    lastGameMoment = currentGameMoment;

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
                game->write<uint16_t>(oldEntScriptStart, 0x0100);
                
                // 29 and 30 are the only entrance scripts with different first two commands.
                if (lastClosestIndex == 29)
                {
                    game->write<uint16_t>(oldEntScriptStart + 2, 0x0114);
                }
                else if (lastClosestIndex == 30)
                {
                    game->write<uint16_t>(oldEntScriptStart + 2, 0x011c);
                }
                else
                {
                    game->write<uint16_t>(oldEntScriptStart + 2, 0x011b);
                }
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

            WorldMapEntrance& origEntrance = GameData::worldMapEntrances[closestIndex];
            LOG("Randomized world map entrance %d to %d", origEntrance.fieldID, randEntrance.fieldID);
        }
    }
}

void RandomizeWorldMap::onWorldMapEnter()
{
    // Overwrite the scripts that stop you from using vehicles during Yuffie side quest.
    // This prevents a soft lock where you get stuck on Wutai island.
    uint16_t chocoboFix[2] = { 0x0200, 0x14DA };
    game->write(0xD336A, (uint8_t*)chocoboFix, 4);
    uint16_t broncoFix[2] = { 0x0200, 0x20E0 };
    game->write(0xD4B76, (uint8_t*)broncoFix, 4);
    uint16_t highwindFix[2] = { 0x0200, 0x2391 };
    game->write(0xD50D8, (uint8_t*)highwindFix, 4);
}

int findWorldEntranceIndex(uint16_t fieldID)
{
    for (int i = 0; i < GameData::worldMapEntrances.size(); ++i)
    {
        WorldMapEntrance& entrance = GameData::worldMapEntrances[i];
        if (entrance.fieldID == fieldID)
        {
            return i;
        }
    }

    return -1;
}

void RandomizeWorldMap::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    // If Cosmo Canyon has been randomized to something else and we let the buggy break down
    // then we'll be stuck on that side of the river with no way back over. To fix this we
    // reset the buggy to not being broken and place it in front of cosmo canyon.
    uint16_t currentGameMoment = game->getGameMoment();
    if (currentGameMoment >= 469 && currentGameMoment < 514)
    {
        Flags buggyFlags = game->read<uint8_t>(0x9D457);
        if (buggyFlags.isBitSet(1))
        {
            game->write<uint32_t>(SavemapOffsets::BuggyHighwindPosition + 0, 0xd031543e);
            game->write<uint32_t>(SavemapOffsets::BuggyHighwindPosition + 4, 0x18a6a0c6);

            buggyFlags.setBit(1, false);
            game->write<uint8_t>(0x9D457, buggyFlags.value());
            LOG("Repaired buggy and moved it in front of Cosmo Canyon.");
        }
    }

    // Chocobo ranch exit on chocobo needs to be patched.
    if (fieldID == 345)
    {
        uint16_t exitIndex = getRandomEntrance(3);
        WorldMapEntrance& randEntrance = GameData::worldMapEntrances[exitIndex];

        // Overwrite the MAPJUMP command to jump to the field we want.
        game->write<uint16_t>(FieldScriptOffsets::ScriptStart + 0x35A8 + 1, randEntrance.fieldID);
        LOG("Changed chocobo stable exit to: %d", randEntrance.fieldID);
    }

    // Weapons seller will teleport us back onto world map, we need to patch it.
    if (fieldID == 79 && currentGameMoment < 566)
    {
        uint16_t exitIndex = getRandomEntrance(9);
        WorldMapEntrance& randEntrance = GameData::worldMapEntrances[exitIndex];

        // Overwrite the MAPJUMP command to jump to the field we want.
        game->write<uint16_t>(FieldScriptOffsets::ScriptStart + 0x31E + 1, randEntrance.fieldID);
        LOG("Changed weapon seller exit to: %d", randEntrance.fieldID);
    }

    // When doing the Yuffie Wutai side quest it ends by teleporting us onto the world map
    // next to Wutai we need to correct that to the randomized location.
    if (fieldID == 581)
    {
        uint16_t exitIndex = getRandomEntrance(22);
        WorldMapEntrance& randEntrance = GameData::worldMapEntrances[exitIndex];

        // Overwrite the MAPJUMP command to jump to the field we want.
        game->write<uint16_t>(FieldScriptOffsets::ScriptStart + 0xDFE + 1, randEntrance.fieldID);
        LOG("Changed Wutai side quest ending cutscene exit to: %d", randEntrance.fieldID);
    }

    for (int i = 0; i < fieldData.worldExits.size(); ++i)
    {
        FieldWorldExit& exit = fieldData.worldExits[i];
        int exitIndex = findWorldEntranceIndex(exit.fieldID);
        if (exitIndex == -1)
        {
            continue;
        }

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

void RandomizeWorldMap::onModuleChanged(uint8_t newModule)
{
    if (newModule == GameModule::World)
    {
        enteringWorld = true;
    }
}

// We're trying to hot patch world scripts before they get executed so 
// this takes recognizing they're loaded as early as possible.
void RandomizeWorldMap::onUpdate()
{
    if (!enteringWorld)
    {
        return;
    }

    // We only need to patch if we're exiting chocobo ranch stables
    uint16_t fieldId = game->getFieldID();
    if (fieldId != 345)
    {
        enteringWorld = false;
        return;
    }

    // Check the four gotos we're expecting for the chocobo ranch possibilities.
    static uintptr_t gotoAddr[4] = { 0xD0DB4, 0xD0DE8, 0xD0E9E, 0xD0ED2 };
    for (int i = 0; i < 4; ++i)
    {
        uint16_t gotoOp = game->read<uint16_t>(gotoAddr[i] + 0);
        uint16_t gotoVal = game->read<uint16_t>(gotoAddr[i] + 2);

        if (gotoOp != 0x0200 || gotoVal != 0x083D)
        {
            return;
        }
    }

    // Find which randomized entrance takes us to chocobo ranch
    int ranchEntranceIdx = -1;
    for (auto entry : randomizedEntrances)
    {
        // Index 2 is exit 3 (chocobo ranch)
        if (entry.second == 2)
        {
            ranchEntranceIdx = entry.first;
        }
    }

    // Don't need to patch if it wasn't randomized.
    if (ranchEntranceIdx == -1 || ranchEntranceIdx == 2)
    {
        enteringWorld = false;
        return;
    }

    // The patch works as follows:
    // - Patch the final GOTO in whatever exit we came out at to jump to the chocobo ranch exit code.
    // - Patch the chocobo ranch exit code to move character to the randomized exit location.
    // This way we hit both sections of exit code but still get the chocobo riding.

    WorldMapEntrance& randEntrance = GameData::worldMapEntrances[ranchEntranceIdx];
    LOG("Patching world map script for chocobo stable exit: %d", randEntrance.fieldID);

    // These are location data thats used to set the player position on exit.
    static uintptr_t posAddr[4] = { 0xD0D96, 0xD0DCA, 0xD0E80, 0xD0EB4 };
    static uint16_t patchValues[4][4] = {
        {0x0016, 0x000F, 0x1524, 0x01BD}, // Midgar
        {0x0018, 0x000D, 0x13D6, 0x1920}, // Kalm
        {0x001D, 0x0010, 0x0712, 0x1D23}, // Chocobo Ranch
        {0x001A, 0x0012, 0x073D, 0x15BA}  // Mithril Mine 
    };

    if (randEntrance.fieldID == 1 || randEntrance.fieldID == 2 || randEntrance.fieldID == 4)
    {
        int idx = randEntrance.fieldID - 1;

        // Jump to chocobo setup
        game->write<uint16_t>(gotoAddr[idx] + 2, 0x0208);

        // Set position to randomized location
        game->write<uint16_t>(0xD0E82, patchValues[idx][0]);
        game->write<uint16_t>(0xD0E86, patchValues[idx][1]);
        game->write<uint16_t>(0xD0E8E, patchValues[idx][2]);
        game->write<uint16_t>(0xD0E92, patchValues[idx][3]);
    }
    else 
    {
        LOG("Chocobo ranch randomized to unexpected entrance: %d", randEntrance.fieldID);
    }

    enteringWorld = false;
}