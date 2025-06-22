#include "RandomizeFieldItems.h"
#include "game/GameData.h"
#include "game/MemoryOffsets.h"
#include "utilities/Logging.h"

#include <algorithm>
#include <random>

// TODO:
// - Update the text boxes when picking up an item

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

uint32_t makeKey(uint16_t fieldID, uint8_t index)
{
    return (uint32_t(fieldID) << 16 | index);
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
        randomizedItems[makeKey(newLoc.fieldID, newLoc.index)] = randomItem;
    }

    for (size_t i = 0; i < allMateria.size(); ++i)
    {
        const SourceLoc& newLoc = allMateria[i].second;
        const FieldItemData& randomMateria = shuffledMateria[i].first;
        randomizedMateria[makeKey(newLoc.fieldID, newLoc.index)] = randomMateria;
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
    int randomizedFieldMateria = 0;

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

        std::string oldItemName = GameData::getNameFromFieldScriptID(oldItemID);
        std::string newItemName = GameData::getNameFromFieldScriptID(newItem.id);
        LOG("Randomized item on field %d: %s (%d) changed to: %s (%d)", fieldID, oldItemName.c_str(), oldItemQuantity, newItemName.c_str(), newItem.quantity);
        
        randomizedFieldItems++;
    }

    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldItemData& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;

        uint8_t oldMaterialID = game->read<uint8_t>(idOffset);

        if (oldMaterialID != materia.id)
        {
            // Data isn't loaded yet.
            continue;
        }

        uint32_t randomKey = makeKey(fieldID, i);
        if (randomizedMateria.count(randomKey) == 0)
        {
            continue;
        }

        // Select a different item from the already randomized table and overwrite.
        FieldItemData newMateria = randomizedMateria[randomKey];
        game->write<uint8_t>(idOffset, newMateria.id);

        std::string oldMateriaName = GameData::getMateriaName(oldMaterialID);
        std::string newMateriaName = GameData::getMateriaName(newMateria.id);
        LOG("Randomized materia on field %d: %s changed to: %s", fieldID, oldMateriaName.c_str(), newMateriaName.c_str());

        randomizedFieldMateria++;
    }

    if (randomizedFieldItems == fieldData.items.size() && randomizedFieldMateria == fieldData.materia.size())
    {
        fieldNeedsRandomize = false;
    }
}