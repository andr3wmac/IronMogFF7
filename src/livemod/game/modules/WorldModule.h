#pragma once

#include "livemod/game/GameData.h"
#include <cstdint>

class GameManager;

class WorldModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);

private:
    bool isWorldDataLoaded(bool justConnected = false, bool ignoreEncounterTable = false);

    GameManager* game = nullptr;
    uint8_t gameModule = 0;

    Encounter worldMapEncounterTable[1024];

    bool waitingForWorldData = false;
    bool waitingForWorldChange = false;
    int lastWorldScreenFade = 0;
    uint32_t lastWorldMapID = 0;
};