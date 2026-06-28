#include "LiveModFF7/game/modules/WorldModule.h"
#include "LiveModFF7/game/GameManager.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"

void WorldModule::setup(GameManager* game)
{
    this->game = game;
    gameModule = game->getGameModule();

    BIND_EVENT_ONE_ARG(game->onModuleChanged, WorldModule::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onUpdate, WorldModule::onUpdate);
}

void WorldModule::onModuleChanged(uint8_t newGameModule)
{
    if (gameModule != GameModule::World && newGameModule == GameModule::World)
    {
        waitingForWorldData = true;
        waitingForWorldChange = false;
        lastWorldScreenFade = game->read<uint8_t>(GameOffsets::WorldScreenFade);
        lastWorldMapID = game->read<uint32_t>(WorldOffsets::ScriptStart);
    }

    gameModule = newGameModule;
}

void WorldModule::onUpdate(bool justConnected)
{
    if (gameModule == GameModule::World)
    {
        uint32_t worldMapID = game->read<uint32_t>(WorldOffsets::ScriptStart);
        if (worldMapID != lastWorldMapID)
        {
            waitingForWorldChange = true;
            lastWorldMapID = worldMapID;
        }

        if (waitingForWorldChange && isWorldDataLoaded(justConnected, true))
        {
            LOG("Changed world maps.");
            game->onWorldMapEnter.invoke();
            waitingForWorldChange = false;
        }

        if (waitingForWorldData && isWorldDataLoaded(justConnected))
        {
            LOG("Entered world map.");
            game->onWorldMapEnter.invoke();
            waitingForWorldData = false;
        }
    }
}

bool WorldModule::isWorldDataLoaded(bool justConnected, bool ignoreEncounterTable)
{
    game->read(WorldOffsets::EncounterStart, 2048, (uint8_t*)worldMapEncounterTable);

    // When changing world maps to underwater the encounter table remains unchanged,
    // so this check becomes invalid.
    if (!ignoreEncounterTable)
    {
        for (int r = 0; r < 16; ++r)
        {
            WorldMapEncounters& origEncounters = GameData::worldMapEncounters[r];

            for (int s = 0; s < 4; ++s)
            {
                std::vector<Encounter>& origEncSet = origEncounters.sets[s];
                if (origEncSet.size() == 0)
                {
                    continue;
                }

                // worldMapEncounterTable is uint16_t so these are two byte strides.
                uintptr_t dataOffset = (r * 64) + (s * 16) + 1;

                for (int i = 0; i < 14; ++i)
                {
                    Encounter& origEnc = origEncSet[i];
                    Encounter& encData = worldMapEncounterTable[dataOffset + i];

                    if (origEnc.raw != encData.raw)
                    {
                        return false;
                    }
                }
            }
        }
    }

    // World is ready if we've hit peak fade out and started coming back down.
    uint8_t screenFade = game->read<uint8_t>(GameOffsets::WorldScreenFade);
    bool isScreenReady = (lastWorldScreenFade == 0xFF && screenFade < lastWorldScreenFade);
    lastWorldScreenFade = screenFade;

    if (justConnected && screenFade == 0)
    {
        isScreenReady = true;
    }

    return isScreenReady;
}