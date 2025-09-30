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
            return;
        }

        for (int i = 0; i < fieldData.items.size(); ++i)
        {
            FieldItemData& item = fieldData.items[i];
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
            FieldItemData& materia = fieldData.materia[i];
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
    // HACK: Every 60 frames we reapply the randomization. This handles a particular situation
    // where the game will update the game moment and then reload the same field. Since the field
    // ID doesn't change we don't detect it, but it reloads the scripts and overwrites our changes.
    if (frameNumber % 60 == 0)
    {
        apply();
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

    std::vector<std::pair<FieldItemData, SourceLoc>> allItems;
    std::vector<std::pair<FieldItemData, SourceLoc>> allMateria;

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
    std::vector<std::pair<FieldItemData, SourceLoc>> shuffledItems = allItems;
    std::shuffle(shuffledItems.begin(), shuffledItems.end(), rng);
    std::vector<std::pair<FieldItemData, SourceLoc>> shuffledMateria = allMateria;
    std::shuffle(shuffledMateria.begin(), shuffledMateria.end(), rng);

    for (size_t i = 0; i < allItems.size(); ++i) 
    {
        const SourceLoc& newLoc = allItems[i].second;
        const FieldItemData& randomItem = shuffledItems[i].first;
        randomizedItems[makeKey(newLoc.fieldID, (uint8_t)newLoc.index)] = randomItem;
    }

    for (size_t i = 0; i < allMateria.size(); ++i)
    {
        const SourceLoc& newLoc = allMateria[i].second;
        const FieldItemData& randomMateria = shuffledMateria[i].first;
        randomizedMateria[makeKey(newLoc.fieldID, (uint8_t)newLoc.index)] = randomMateria;
    }

    // TODO: generate spoiler log information here?
}

void RandomizeFieldItems::apply()
{
    FieldData fieldData = GameData::getField(game->getFieldID());
    if (!fieldData.isValid())
    {
        return;
    }

    // Randomize items
    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldItemData& item = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
        uintptr_t itemQuantityOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemQuantity;

        uint16_t oldItemID = game->read<uint16_t>(itemIDOffset);
        uint8_t oldItemQuantity = game->read<uint8_t>(itemQuantityOffset);

        if (oldItemID != item.id || oldItemQuantity != item.quantity)
        {
            // Data isn't loaded yet or has already been randomized.
            continue;
        }

        // Do not randomize Battery in Wall Market.
        if (fieldData.id == 196 && item.id == 85)
        {
            continue;
        }

        uint32_t randomKey = makeKey(fieldData.id, i);
        if (randomizedItems.count(randomKey) == 0)
        {
            continue;
        }

        // Select a different item from the already randomized table and overwrite.
        FieldItemData newItem = randomizedItems[randomKey];

        // It's possible we randomized to the same item already there.
        if (newItem.id == oldItemID)
        {
            continue;
        }

        game->write<uint16_t>(itemIDOffset, newItem.id);
        game->write<uint8_t>(itemQuantityOffset, newItem.quantity);

        std::string oldItemName = GameData::getNameFromFieldScriptID(oldItemID);
        std::string newItemName = GameData::getNameFromFieldScriptID(newItem.id);
        LOG("Randomized item on field %d: %s (%d) changed to: %s (%d)", fieldData.id, oldItemName.c_str(), oldItemQuantity, newItemName.c_str(), newItem.quantity);

        // Overwrite the popup message
        int msgIndex = game->findPickUpMessage(oldItemName, item.offset);
        if (msgIndex >= 0)
        {
            FieldMessage& fieldMsg = fieldData.messages[msgIndex];
            game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, newItemName);
        }
    }

    // Randomize materia
    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldItemData& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;

        uint8_t oldMateriaID = game->read<uint8_t>(idOffset);
        if (oldMateriaID != materia.id)
        {
            // Data isn't loaded yet.
            continue;
        }

        uint32_t randomKey = makeKey(fieldData.id, i);
        if (randomizedMateria.count(randomKey) == 0)
        {
            continue;
        }

        // Select a different item from the already randomized table and overwrite.
        uint16_t newMateriaID = randomizedMateria[randomKey].id;

        // It's possible we randomized to the same materia already there.
        if (newMateriaID == oldMateriaID)
        {
            continue;
        }

        if (Restrictions::isMateriaBanned((uint8_t)newMateriaID))
        {
            std::mt19937_64 rng64(Utilities::makeKey(game->getSeed(), fieldData.id, (uint8_t)newMateriaID));
            newMateriaID = GameData::getRandomMateria(rng64);
        }
        game->write<uint8_t>(idOffset, (uint8_t)newMateriaID);

        std::string oldMateriaName = GameData::getMateriaName(oldMateriaID);
        std::string newMateriaName = GameData::getMateriaName((uint8_t)newMateriaID);
        LOG("Randomized materia on field %d: %s changed to: %s", fieldData.id, oldMateriaName.c_str(), newMateriaName.c_str());

        // Overwrite the popup message
        int msgIndex = game->findPickUpMessage(oldMateriaName, materia.offset);
        if (msgIndex >= 0)
        {
            FieldMessage& fieldMsg = fieldData.messages[msgIndex];
            game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, newMateriaName);
        }
    }
}