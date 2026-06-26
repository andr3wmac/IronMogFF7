#pragma once

#include "livemod/game/GameData.h"

#include <cstdint>
#include <string>

class GameManager;

class FieldModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);
    void onFrame(uint32_t frameNumber);
    void onWorldMapEnter();

    uint16_t getFieldID() { return fieldID; }

    // Finds the nearest message that contains the item name
    int findPickUpMessage(std::string itemName, uint8_t group, uint8_t script, uint32_t offset);

    void overwriteMessage(int msgIndex, const std::string& newText);

private:
    struct MessageOverwrite
    {
        FieldScriptMessage fieldMsg;
        std::string text;
    };

    void onFieldChanged(uint16_t fieldID);
    bool isFieldDataLoaded(bool justConnected = false);

    GameManager* game = nullptr;
    uint8_t gameModule = 0;
    uint16_t fieldID = 0;

    bool waitingForFieldData = false;
    int lastFieldScreenFade = 0;

    // List of messages that should be overwritten in real time rather than on field change.
    // This is for items that share the same message in memory and thus would conflict.
    std::vector<MessageOverwrite> overwriteMessages;
    std::vector<FieldScriptMessage> messagesToClear;
};