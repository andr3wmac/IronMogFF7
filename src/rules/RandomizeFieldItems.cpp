#include "RandomizeFieldItems.h"
#include "game/GameData.h"
#include "game/MemoryOffsets.h"
#include "utilities/Logging.h"

#include <algorithm>
#include <random>

REGISTER_RULE("Randomize Field Items", RandomizeFieldItems)

void RandomizeFieldItems::onStart()
{
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeFieldItems::onFieldChanged);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeFieldItems::onFrame);
    BIND_EVENT(game->onBattleExit, RandomizeFieldItems::onBattleExit);

    generateRandomizedItems();
}

void RandomizeFieldItems::onFieldChanged(uint16_t fieldID)
{
    fieldNeedsRandomize = true;
}

void RandomizeFieldItems::onBattleExit()
{
    fieldNeedsRandomize = true;
}

void RandomizeFieldItems::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Field)
    {
        return;
    }

    if (fieldNeedsRandomize)
    {
        randomizeFieldItems(game->getFieldID());
    }
}

uint32_t makeKey(uint16_t fieldID, uint8_t itemIndex)
{
    return (uint32_t(fieldID) << 16 | itemIndex);
}

// Generate a map of fieldID and item index to the item data then shuffle those pairings
// using the seed. This ensures any pickup is a swap of one from another field and that its
// the same swap everytime you enter the field given the same seed.
void RandomizeFieldItems::generateRandomizedItems()
{
    randomizedItems.clear();

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
        size_t itemIndex;
    };

    std::vector<std::pair<FieldItemData, SourceLoc>> allItems;

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
    }

    std::mt19937 rng(game->getSeed());
    std::vector<std::pair<FieldItemData, SourceLoc>> shuffled = allItems;
    std::shuffle(shuffled.begin(), shuffled.end(), rng);

    for (size_t i = 0; i < allItems.size(); ++i) 
    {
        const SourceLoc& newLoc = allItems[i].second;
        const FieldItemData& randomizedItem = shuffled[i].first;
        randomizedItems[makeKey(newLoc.fieldID, newLoc.itemIndex)] = randomizedItem;
    }

    // TODO: generate spoiler log information here?
}

void RandomizeFieldItems::randomizeFieldItems(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    int randomizedFieldItems = 0;

    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldItemData& item = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
        uintptr_t itemQuantityOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemQuantity;

        uint16_t oldItemID = game->read<uint16_t>(itemIDOffset);
        uint8_t oldItemQuantity = game->read<uint8_t>(itemQuantityOffset);

        if (oldItemID != item.id || oldItemQuantity != item.quantity)
        {
            // Data isn't loaded yet.
            continue;
        }

        uint32_t randomKey = makeKey(fieldID, i);
        if (randomizedItems.count(randomKey) == 0)
        {
            continue;
        }

        // Select a different item from the already randomized table and overwrite.
        FieldItemData newItem = randomizedItems[randomKey];
        game->write<uint16_t>(itemIDOffset, newItem.id);
        game->write<uint8_t>(itemQuantityOffset, newItem.quantity);

        ItemData oldItemData = GameData::getItemDataFromFieldItemID(oldItemID);
        ItemData newItemData = GameData::getItemDataFromFieldItemID(newItem.id);
        LOG("Randomized item on field %d: %s (%d) changed to: %s (%d)", fieldID, oldItemData.name.c_str(), oldItemQuantity, newItemData.name.c_str(), newItem.quantity);
        
        randomizedFieldItems++;
    }

    if (randomizedFieldItems == fieldData.items.size())
    {
        fieldNeedsRandomize = false;
    }
}