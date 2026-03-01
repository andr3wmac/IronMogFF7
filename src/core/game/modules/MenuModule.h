#pragma once

#include <cstdint>

class GameManager;

class MenuModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);

private:
    bool isShopDataLoaded();

    GameManager* game = nullptr;
    uint8_t gameModule = 0;

    bool waitingForShopData = false;
    bool wasInShopMenu = false;
};