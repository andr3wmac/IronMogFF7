#pragma once

#include <cstdint>

class GameManager;

class BattleModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);

    bool inBattle();

private:
    void onBattleEnter();
    void onBattleTransition(uint16_t formation);
    void deleteBannedDrops();
    bool isBattleDataLoaded();
    bool isFormationLoaded(uint16_t formationID);

    GameManager* game = nullptr;
    uint8_t gameModule = 0;

    uint16_t lastBattleFormation = 0;
    bool waitingForBattleData = false;
    bool waitingForFormation = false;
};