#include "FieldModule.h"
#include "core/game/GameManager.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "rules/Restrictions.h"

void FieldModule::setup(GameManager* game)
{
    this->game = game;
    gameModule = game->getGameModule();
    fieldID = 0;

    BIND_EVENT_ONE_ARG(game->onModuleChanged, FieldModule::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onUpdate, FieldModule::onUpdate);
    BIND_EVENT(game->onWorldMapEnter, FieldModule::onWorldMapEnter);
}

void FieldModule::onModuleChanged(uint8_t newGameModule)
{
    if (gameModule == GameModule::Battle && newGameModule == GameModule::Field)
    {
        waitingForFieldData = true;
    }

    gameModule = newGameModule;
}

void FieldModule::onUpdate(bool justConnected)
{
    if (gameModule == GameModule::Field)
    {
        // Detect if we're warping into the same field we're already in, this 
        // is a reload and otherwise wouldn't trigger onFieldChanged.
        uint8_t fieldWarpTrigger = game->read<uint8_t>(GameOffsets::FieldWarpTrigger);
        if (fieldWarpTrigger == 1 && !waitingForFieldData)
        {
            uint16_t fieldWarpID = game->read<uint16_t>(GameOffsets::FieldWarpID);
            if (fieldID == fieldWarpID)
            {
                lastFieldScreenFade = game->read<uint16_t>(GameOffsets::FieldScreenFade);
                waitingForFieldData = true;
            }
        }

        uint16_t newFieldID = game->read<uint16_t>(GameOffsets::FieldID);
        if (newFieldID != fieldID)
        {
            lastFieldScreenFade = game->read<uint16_t>(GameOffsets::FieldScreenFade);
            waitingForFieldData = true;
            fieldID = newFieldID;
        }

        if (waitingForFieldData && isFieldDataLoaded(justConnected))
        {
            LOG("Loaded Field: %d", fieldID);
            onFieldChanged(fieldID);
            waitingForFieldData = false;
        }
    }
}

void FieldModule::onWorldMapEnter()
{
    // When exiting onto the world map field ID is updated to your exit location
    // We don't trigger the onFieldChanged event for this but its important we update
    // this value in case we re-enter the field we just left.
    uint16_t newFieldID = game->read<uint16_t>(GameOffsets::FieldID);
    fieldID = newFieldID;
}

void FieldModule::onFieldChanged(uint16_t fieldID)
{
    // First trigger anything else that might modify field items/materia.
    game->onFieldChanged.invoke(fieldID);

    // Now check each field item for something thats banned. This way if it wasn't
    // replaced by a randomizer we delete it to enforce the ban.

    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldScriptItem& item = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
        uint16_t itemID = game->read<uint16_t>(itemIDOffset);

        if (Restrictions::isItemBanned(itemID))
        {
            // Delete item.
            for (int b = 0; b < 5; ++b)
            {
                game->write<uint8_t>(FieldScriptOffsets::ScriptStart + item.offset + b, 0x5F);
            }

            // Delete the popup message
            std::string itemName = GameData::getItemName(itemID);
            int msgIndex = game->findPickUpMessage(itemName, item.group, item.script, item.offset);
            if (msgIndex >= 0)
            {
                const FieldScriptMessage& fieldMsg = fieldData.messages[msgIndex];
                for (int b = 0; b < 3; ++b)
                {
                    game->write<uint8_t>(FieldScriptOffsets::ScriptStart + fieldMsg.offset + b, 0x5F);
                }
            }

            LOG("Deleted banned item: %s %d", itemName.c_str(), itemID);
        }
    }

    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldScriptItem& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;
        uint8_t materiaID = game->read<uint8_t>(idOffset);

        if (Restrictions::isMateriaBanned(materiaID))
        {
            // Delete materia.
            for (int b = 0; b < 7; ++b)
            {
                game->write<uint8_t>(FieldScriptOffsets::ScriptStart + materia.offset + b, 0x5F);
            }

            // Delete the popup message
            std::string materiaName = GameData::getItemName(materiaID);
            int msgIndex = game->findPickUpMessage(materiaName, materia.group, materia.script, materia.offset);
            if (msgIndex >= 0)
            {
                const FieldScriptMessage& fieldMsg = fieldData.messages[msgIndex];
                for (int b = 0; b < 3; ++b)
                {
                    game->write<uint8_t>(FieldScriptOffsets::ScriptStart + fieldMsg.offset + b, 0x5F);
                }
            }

            LOG("Deleted banned materia: %d", materiaID);
        }
    }
}

// Detect if field data is fully loaded by verifying the set of information
// we know about the field is confirmed in memory.
bool FieldModule::isFieldDataLoaded(bool justConnected)
{
    if (gameModule != GameModule::Field)
    {
        return false;
    }

    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return false;
    }

    int randomizedFieldItems = 0;
    int randomizedFieldMateria = 0;

    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldScriptItem& item = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
        uintptr_t itemQuantityOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemQuantity;

        uint16_t itemID = game->read<uint16_t>(itemIDOffset);
        uint8_t itemQuantity = game->read<uint8_t>(itemQuantityOffset);

        if (itemID != item.id || itemQuantity != item.quantity)
        {
            return false;
        }
    }

    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldScriptItem& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;

        uint8_t materiaID = game->read<uint8_t>(idOffset);

        if (materiaID != materia.id)
        {
            return false;
        }
    }

    for (int i = 0; i < fieldData.messages.size(); ++i)
    {
        FieldScriptMessage& message = fieldData.messages[i];
        uint8_t opCode = game->read<uint8_t>(FieldScriptOffsets::ScriptStart + message.offset);
        uint8_t endChar = game->read<uint8_t>(FieldScriptOffsets::ScriptStart + message.strOffset + message.strLength);
        if (opCode != 0x40 || endChar != 0xFF)
        {
            return false;
        }
    }

    for (int i = 0; i < fieldData.worldExits.size(); ++i)
    {
        FieldWorldExit& exit = fieldData.worldExits[i];
        uintptr_t exitOffset = FieldScriptOffsets::TriggersStart + exit.offset;

        uint16_t fieldID = game->read<uint8_t>(exitOffset);
        if (fieldID != exit.fieldID)
        {
            return false;
        }
    }

    // Encounter data
    {
        for (int t = 0; t < 2; ++t)
        {
            uintptr_t tableOffset = FieldScriptOffsets::EncounterStart + fieldData.encounterOffset + (t * FieldScriptOffsets::EncounterTableStride);

            Encounter encTable[10];
            game->read(tableOffset + 2, sizeof(uint16_t) * 10, (uint8_t*)encTable);

            for (int i = 0; i < 10; ++i)
            {
                Encounter& origEncounter = fieldData.getEncounter(t, i);
                if (origEncounter.prob == 0 && origEncounter.id == 0)
                {
                    continue;
                }

                if (origEncounter.prob != encTable[i].prob || origEncounter.id != encTable[i].id)
                {
                    return false;
                }
            }
        }
    }

    // Field is ready if we've hit peak fade out and started coming back down.
    uint16_t screenFade = game->read<uint16_t>(GameOffsets::FieldScreenFade);
    bool isScreenReady = (lastFieldScreenFade == 0x100 && screenFade < lastFieldScreenFade);
    lastFieldScreenFade = screenFade;

    if (justConnected && screenFade == 0)
    {
        isScreenReady = true;
    }

    // Hack fix for base of tower transition after wedge falls. For whatever reason 
    // FieldScreenFade stays at 256 the whole time.
    {
        if (fieldID == 156 && game->getGameMoment() == 218)
        {
            uint16_t fieldWarpID = game->read<uint16_t>(GameOffsets::FieldWarpID);
            uint8_t screenBlack = game->read<uint8_t>(0x9AC40);
            if (fieldID != fieldWarpID && screenBlack == 0)
            {
                isScreenReady = true;
            }
        }

        // Due to the previous problem of FieldScreenFade being stuck at 256 we need a special
        // case for the transition to the next scene going up the tower.
        if (fieldID == 158 && game->getGameMoment() == 221)
        {
            uint8_t screenBlack = game->read<uint8_t>(0x9AC40);
            isScreenReady = (screenFade == 0 && screenBlack == 0);
        }
    }

    return isScreenReady;
}