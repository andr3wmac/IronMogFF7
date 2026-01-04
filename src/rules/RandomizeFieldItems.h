#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>

class RandomizeFieldItems : public Rule
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    struct MessageOverwrite
    {
        FieldScriptMessage fieldMsg;
        std::string text;
    };

    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);

    // Shuffles items and materia between maps based on the game seed.
    void generateRandomizedItems();

    // Applies randomization to current field.
    void apply();
    void overwriteMessage(const FieldData& fieldData, const FieldScriptItem& oldItem, const FieldScriptItem& newItem, const std::string& oldName, const std::string& newName);

    // Generated randomization mapping
    std::unordered_map<uint32_t, FieldScriptItem> randomizedItems;
    std::unordered_map<uint32_t, FieldScriptItem> randomizedMateria;

    // List of messages that should be overwritten in real time rather than on field change.
    // This is for items that share the same message in memory and thus would conflict.
    std::vector<MessageOverwrite> overwriteMessages;
    std::vector<FieldScriptMessage> messagesToClear;
};