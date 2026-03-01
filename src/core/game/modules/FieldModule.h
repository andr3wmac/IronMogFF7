#pragma once

#include <cstdint>

class GameManager;

class FieldModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);
    void onWorldMapEnter();

    uint16_t getFieldID() { return fieldID; }

private:
    void onFieldChanged(uint16_t fieldID);
    bool isFieldDataLoaded(bool justConnected = false);

    GameManager* game = nullptr;
    uint8_t gameModule = 0;
    uint16_t fieldID = 0;

    bool waitingForFieldData = false;
    int lastFieldScreenFade = 0;
};