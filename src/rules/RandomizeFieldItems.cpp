#include "RandomizeFieldItems.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "rules/Restrictions.h"

#include <algorithm>
#include <imgui.h>
#include <random>

REGISTER_RULE("Randomize Field Items", RandomizeFieldItems)

void RandomizeFieldItems::setup()
{
    BIND_EVENT(game->onStart, RandomizeFieldItems::onStart);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeFieldItems::onFrame);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeFieldItems::onFieldChanged);
}

bool RandomizeFieldItems::onSettingsGUI()
{
    bool changed = false;

    int* randomModeInt = (int*)(&randomMode);
    changed |= ImGui::RadioButton("Shuffle", randomModeInt, 0);
    ImGui::SetItemTooltip("Items will be replaced by an item from another field.");
    changed |= ImGui::RadioButton("Random", randomModeInt, 1);
    ImGui::SetItemTooltip("Items are replaced with a random selection.");

    return changed;
}

void RandomizeFieldItems::loadSettings(const ConfigFile& cfg)
{
    randomMode = (RandomMode)cfg.get<int>("randomMode", 0);
}

void RandomizeFieldItems::saveSettings(ConfigFile& cfg)
{
    cfg.set<int>("randomMode", (int)randomMode);
}

void RandomizeFieldItems::onDebugGUI()
{
    if (game->getGameModule() != GameModule::Field)
    {
        ImGui::Text("Not currently in field.");
        return;
    }
    
    // Display field items and their values
    {
        FieldData fieldData = GameData::getField(game->getFieldID());
        if (!fieldData.isValid())
        {
            ImGui::Text("Invalid field.");
            return;
        }

        for (int i = 0; i < fieldData.items.size(); ++i)
        {
            FieldScriptItem& item = fieldData.items[i];
            uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
            uintptr_t itemQuantityOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemQuantity;

            uint16_t oldItemID = game->read<uint16_t>(itemIDOffset);
            uint8_t oldItemQuantity = game->read<uint8_t>(itemQuantityOffset);

            std::string debugText = "Item Orig: " + std::to_string(item.id) + " (" + std::to_string(item.quantity) + ") ";
            debugText += "Cur: " + std::to_string(oldItemID) + " (" + std::to_string(oldItemQuantity) + ")";
            ImGui::Text(debugText.c_str());
        }

        for (int i = 0; i < fieldData.materia.size(); ++i)
        {
            FieldScriptItem& materia = fieldData.materia[i];
            uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;

            uint8_t oldMateriaID = game->read<uint8_t>(idOffset);

            std::string debugText = "Materia Orig: " + std::to_string(materia.id) + " Cur: " + std::to_string(oldMateriaID);
            ImGui::Text(debugText.c_str());
        }
    }
}

void RandomizeFieldItems::onStart()
{
    generateRandomizedItems();
}

uint32_t makeKey(uint16_t fieldID, uint8_t index)
{
    return (uint32_t(fieldID) << 16 | index);
}

void RandomizeFieldItems::onFrame(uint32_t frameNumber)
{
    for (int i = 0; i < overwriteMessages.size(); ++i)
    {
        const MessageOverwrite& overwriteMsg = overwriteMessages[i];
        
        // Check if the current dialog is overwrite message
        uint16_t fieldScriptPtr = game->getScriptExecutionPointer(overwriteMsg.fieldMsg.group);
        if (fieldScriptPtr == overwriteMsg.fieldMsg.offset)
        {
            game->writeString(GameOffsets::DialogText, overwriteMsg.fieldMsg.strLength, overwriteMsg.text);
        }
    }
}

void RandomizeFieldItems::onFieldChanged(uint16_t fieldID)
{
    apply();
}

// Generate a map of fieldID and item index to the item data then shuffle those pairings
// using the seed. This ensures any pickup is a swap of one from another field and that its
// the same swap everytime you enter the field given the same seed.
void RandomizeFieldItems::generateRandomizedItems()
{
    randomizedItems.clear();
    randomizedMateria.clear();

    // Sort the field IDs to get a deterministic order
    std::vector<uint32_t> sortedFieldIDs;
    for (const auto& kv : GameData::fieldData)
    {
        sortedFieldIDs.push_back(kv.first);
    }
    std::sort(sortedFieldIDs.begin(), sortedFieldIDs.end());

    struct SourceLoc
    {
        uint32_t fieldID;
        size_t index;
    };

    std::vector<std::pair<FieldScriptItem, SourceLoc>> allItems;
    std::vector<std::pair<FieldScriptItem, SourceLoc>> allMateria;

    for (uint32_t fieldID : sortedFieldIDs) 
    {
        FieldData& field = GameData::fieldData[fieldID];

        // Skip any fields with names that start with "black" as those are debug rooms and 
        // loaded with all kinds of items we don't want put into rotation.
        if (field.name.find("black") == 0)
        {
            continue;
        }

        for (size_t i = 0; i < field.items.size(); ++i)
        {
            allItems.push_back({ field.items[i], { fieldID, i } });
        }

        for (size_t i = 0; i < field.materia.size(); ++i)
        {
            allMateria.push_back({ field.materia[i], { fieldID, i } });
        }
    }

    std::mt19937 rng(game->getSeed());
    std::vector<std::pair<FieldScriptItem, SourceLoc>> shuffledItems = allItems;
    std::shuffle(shuffledItems.begin(), shuffledItems.end(), rng);
    std::vector<std::pair<FieldScriptItem, SourceLoc>> shuffledMateria = allMateria;
    std::shuffle(shuffledMateria.begin(), shuffledMateria.end(), rng);

    for (size_t i = 0; i < allItems.size(); ++i) 
    {
        const SourceLoc& newLoc = allItems[i].second;
        const FieldScriptItem& randomItem = shuffledItems[i].first;
        randomizedItems[makeKey(newLoc.fieldID, (uint8_t)newLoc.index)] = randomItem;
    }

    for (size_t i = 0; i < allMateria.size(); ++i)
    {
        const SourceLoc& newLoc = allMateria[i].second;
        const FieldScriptItem& randomMateria = shuffledMateria[i].first;
        randomizedMateria[makeKey(newLoc.fieldID, (uint8_t)newLoc.index)] = randomMateria;
    }
}

void RandomizeFieldItems::apply()
{
    FieldData fieldData = GameData::getField(game->getFieldID());
    if (!fieldData.isValid())
    {
        return;
    }

    overwriteMessages.clear();
    messagesToClear.clear();

    // Randomize items
    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldScriptItem& oldItem = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + oldItem.offset + FieldScriptOffsets::ItemID;
        uintptr_t itemQuantityOffset = FieldScriptOffsets::ScriptStart + oldItem.offset + FieldScriptOffsets::ItemQuantity;

        // Checks whats currently in the item spot
        uint16_t curItemID = game->read<uint16_t>(itemIDOffset);
        uint8_t curItemQuantity = game->read<uint8_t>(itemQuantityOffset);
        if (curItemID != oldItem.id || curItemQuantity != oldItem.quantity)
        {
            // Data isn't loaded yet or has already been randomized.
            continue;
        }

        // Do not randomize Battery in Wall Market.
        if (fieldData.id == 196 && oldItem.id == 85)
        {
            continue;
        }

        uint32_t randomKey = makeKey(fieldData.id, i);
        if (randomizedItems.count(randomKey) == 0)
        {
            continue;
        }

        FieldScriptItem newItem = oldItem;

        if (randomMode == RandomMode::Shuffle)
        {
            // Select a different item from the already randomized table and overwrite.
            newItem = randomizedItems[randomKey];
        }
        else if (randomMode == RandomMode::Random)
        {
            // Pick random one based on key.
            std::mt19937_64 rng64(Utilities::makeSeed64(game->getSeed(), fieldData.id, i));
            newItem.id = GameData::getRandomItemFromID(newItem.id, rng64);
        }
        
        if (Restrictions::isFieldItemBanned(newItem.id))
        {
            std::mt19937_64 rng64(Utilities::makeSeed64(game->getSeed(), fieldData.id, i));
            newItem.id = GameData::getRandomItemFromID(newItem.id, rng64);
        }

        game->write<uint16_t>(itemIDOffset, newItem.id);
        game->write<uint8_t>(itemQuantityOffset, newItem.quantity);

        std::string oldItemName = GameData::getItemNameFromID(oldItem.id);
        std::string newItemName = GameData::getItemNameFromID(newItem.id);
        LOG("Randomized item on field %d: %s (%d) changed to: %s (%d)", fieldData.id, oldItemName.c_str(), oldItem.quantity, newItemName.c_str(), newItem.quantity);

        // Overwrite the popup message
        overwriteMessage(fieldData, oldItem, newItem, oldItemName, newItemName);
    }

    // Randomize materia
    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldScriptItem& oldMateria = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + oldMateria.offset + FieldScriptOffsets::MateriaID;

        // Checks whats currently in the materia spot
        uint8_t curMateriaID = game->read<uint8_t>(idOffset);
        if (curMateriaID != oldMateria.id)
        {
            // Data isn't loaded yet.
            continue;
        }

        // Don't randomize Chocobo Lure at the Chocobo Ranch
        if (fieldData.id == 345 && oldMateria.id == 9)
        {
            continue;
        }

        FieldScriptItem newMateria = oldMateria;
        if (randomMode == RandomMode::Shuffle)
        {
            uint32_t randomKey = makeKey(fieldData.id, i);
            if (randomizedMateria.count(randomKey) == 0)
            {
                continue;
            }

            // Select a different item from the already randomized table and overwrite.
            newMateria = randomizedMateria[randomKey];
        }
        else if (randomMode == RandomMode::Random)
        {
            // Pick random one based on key.
            std::mt19937_64 rng64(Utilities::makeSeed64(game->getSeed(), fieldData.id, (uint8_t)oldMateria.id));
            newMateria.id = GameData::getRandomMateria(rng64);
        }
        
        if (Restrictions::isMateriaBanned((uint8_t)newMateria.id))
        {
            std::mt19937_64 rng64(Utilities::makeSeed64(game->getSeed(), fieldData.id, (uint8_t)newMateria.id));
            newMateria.id = GameData::getRandomMateria(rng64);
        }

        game->write<uint8_t>(idOffset, (uint8_t)newMateria.id);

        std::string oldMateriaName = GameData::getMateriaName((uint8_t)oldMateria.id);
        std::string newMateriaName = GameData::getMateriaName((uint8_t)newMateria.id);
        LOG("Randomized materia on field %d: %s changed to: %s", fieldData.id, oldMateriaName.c_str(), newMateriaName.c_str());

        // Overwrite the popup message
        overwriteMessage(fieldData, oldMateria, newMateria, oldMateriaName, newMateriaName);
    }

    // Clear original messages that will be overwritten in real time
    for (FieldScriptMessage& fieldMsg : messagesToClear)
    {
        game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, "");
    }
}

void RandomizeFieldItems::overwriteMessage(const FieldData& fieldData, const FieldScriptItem& oldItem, const FieldScriptItem& newItem, const std::string& oldName, const std::string& newName)
{
    // Overwrite the popup message
    int msgIndex = game->findPickUpMessage(oldName, oldItem.group, oldItem.script, oldItem.offset);
    if (msgIndex >= 0)
    {
        const FieldScriptMessage& fieldMsg = fieldData.messages[msgIndex];

        int strMsgCount = 0;
        for (const FieldScriptItem& compareItem : fieldData.items)
        {
            std::string compareItemName = GameData::getItemNameFromID(compareItem.id);
            int compareMsgIndex = game->findPickUpMessage(compareItemName, compareItem.group, compareItem.script, compareItem.offset);
            if (compareMsgIndex >= 0)
            {
                const FieldScriptMessage& compareFieldMsg = fieldData.messages[compareMsgIndex];

                if (fieldMsg.strOffset == compareFieldMsg.strOffset)
                {
                    strMsgCount++;
                }
            }
        }

        // If the string has more than one message tied to it then we need to overwrite it
        // in real time rather than just on field change. This is a consequence of randomizing
        // every item separately and the fact the game reuses the same string for duplicates.
        if (strMsgCount > 1)
        {
            overwriteMessages.push_back({ fieldMsg, newName });
            messagesToClear.push_back(fieldMsg);
        }
        else
        {
            game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, newName);
        }
    }
    else 
    {
        LOG("Error: Unable to find message that contains: %s", oldName.c_str());
    }
}