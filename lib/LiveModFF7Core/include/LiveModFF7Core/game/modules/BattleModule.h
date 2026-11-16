#pragma once

#include "LiveModFF7Core/game/modules/BattleLimitManager.h"
#include "LiveModFF7Core/game/modules/BattleModelManager.h"
#include <cstdint>

class GameManager;

class BattleModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);

    bool inBattle();

    // Replaces party members' battle models during native battle loads.
    BattleModelManager models;

    // Gives party members another character's limit breaks.
    BattleLimitManager limits;

private:
    void onBattleEnter();
    void onBattleTransition(uint16_t formation);
    bool isBattleDataLoaded();
    bool isFormationLoaded(uint16_t formationID);

    GameManager* game = nullptr;
    uint8_t gameModule = 0;
    bool seenFirstModule = false;

    uint16_t lastBattleFormation = 0;
    bool waitingForBattleData = false;
    bool waitingForFormation = false;
};