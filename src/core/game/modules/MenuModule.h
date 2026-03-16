#pragma once

#include <cstdint>

class GameManager;

class MenuModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);
    void onFrame(int frameNumber);

private:
    bool isShopDataLoaded();
    void onShopMenuChanged(uint8_t menuIndex);

    GameManager* game = nullptr;
    uint8_t gameModule = 0;

    int shopMenuIndex = -1;
    bool waitingForShopData = false;
    bool inShopMenu = false;
};