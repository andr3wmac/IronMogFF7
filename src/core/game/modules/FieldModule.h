#pragma once

#include "core/game/GameData.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

    // Finds a byte pattern inside one script of the current field and returns its offset from
    // FieldScriptOffsets::ScriptStart. Groups are looked up by name rather than index since mods
    // like CSR insert groups into some fields. Returns nullopt unless the pattern occurs exactly
    // once in that script. Only valid once the field has loaded, e.g. during onFieldChanged.
    std::optional<uint16_t> findScriptOffset(const std::string& groupName, uint8_t scriptIndex, const std::vector<uint8_t>& pattern);

    // Returns the index of the named script group in the current field, e.g. for getScriptExecutionPointer.
    std::optional<uint8_t> findGroupIndex(const std::string& groupName);

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