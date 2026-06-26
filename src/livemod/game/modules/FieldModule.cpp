#include "FieldModule.h"
#include "livemod/game/GameManager.h"
#include "livemod/game/MemoryOffsets.h"
#include "livemod/utilities/Logging.h"

void FieldModule::setup(GameManager* game)
{
    this->game = game;
    gameModule = game->getGameModule();
    fieldID = 0;

    BIND_EVENT_ONE_ARG(game->onModuleChanged, FieldModule::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onUpdate, FieldModule::onUpdate);
    BIND_EVENT_ONE_ARG(game->onFrame, FieldModule::onFrame);
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

void FieldModule::onFrame(uint32_t frameNumber)
{
    // Apply any messages that need to be overwritten
    for (int i = 0; i < overwriteMessages.size(); ++i)
    {
        const MessageOverwrite& overwriteMsg = overwriteMessages[i];
        
        // Check if the current dialog is overwrite message
        uint16_t fieldScriptPtr = game->getScriptExecutionPointer(overwriteMsg.fieldMsg.group);
        if (fieldScriptPtr == overwriteMsg.fieldMsg.offset)
        {
            game->writeString(getWindowTextOffset(overwriteMsg.fieldMsg.window), overwriteMsg.fieldMsg.strLength, overwriteMsg.text);
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
    overwriteMessages.clear();
    messagesToClear.clear();

    // Trigger anything else that might modify field items/materia (e.g. randomizers,
    // ban enforcement). Ban enforcement runs last and may queue message overwrites below.
    game->onFieldChanged.invoke(fieldID);

    // Clear original messages that will be overwritten in real time
    for (FieldScriptMessage& fieldMsg : messagesToClear)
    {
        game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, "");
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
    
    if (justConnected && screenFade == 0)
    {
        isScreenReady = true;
    }

    // HACK: fix for base of tower transition after wedge falls. For whatever reason 
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

        // HACK: fix for Tifa waking up in Dr's Office
        if (fieldID == 400 && game->getGameMoment() == 999)
        {
            isScreenReady = lastFieldScreenFade == 0 && screenFade > 0;
        }
    }

    lastFieldScreenFade = screenFade;
    return isScreenReady;
}

// The goal here is to find the message thats closest in memory (offset) that also contains
// the name of the item. The message is usually: Received "{itemName}"!
int FieldModule::findPickUpMessage(std::string itemName, uint8_t group, uint8_t script, uint32_t offset)
{
    FieldData fieldData = GameData::getField(getFieldID());
    if (!fieldData.isValid())
    {
        return -1;
    }

    int bestIndex = -1;
    uint32_t bestDistance = UINT32_MAX;

    for (int i = 0; i < fieldData.messages.size(); ++i)
    {
        FieldScriptMessage& fieldMsg = fieldData.messages[i];

        // The message is always in the same group+script as the pick up.
        if (fieldMsg.group != group || fieldMsg.script != script)
        {
            continue;
        }

        std::string msg = game->readString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength);
        if (msg.find(itemName) != std::string::npos)
        {
            uint32_t distance = std::abs((int32_t)(fieldMsg.offset - offset));
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }
    }

    // HACK: "Counter Attack" is sometimes shortened to "Counter" in field pick up messages.
    if (bestIndex == -1 && itemName == "Counter Attack")
    {
        return findPickUpMessage("Counter", group, script, offset);
    }

    // HACK: "Luck Plus" is misspelled as Lucky Plus in one text box.
    if (bestIndex == -1 && itemName == "Luck Plus")
    {
        return findPickUpMessage("Lucky Plus", group, script, offset);
    }

    // HACK: "Megalixir" is sometimes "Last Elixir" in field pick up messages.
    if (bestIndex == -1 && itemName == "Megalixir")
    {
        return findPickUpMessage("Last Elixir", group, script, offset);
    }

    return bestIndex;
}

void FieldModule::overwriteMessage(int msgIndex, const std::string& newText)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (msgIndex < 0 || !fieldData.isValid())
    {
        return;
    }

    const FieldScriptMessage& fieldMsg = fieldData.messages[msgIndex];

    // We need to determine how many message boxes are pointing to the same string to determine if
    // we need to do a special override for it or not.
    int strMsgCount = 0;

    for (const FieldScriptItem& compareItem : fieldData.items)
    {
        std::string compareItemName = GameData::getItemName(compareItem.id);
        int compareMsgIndex = findPickUpMessage(compareItemName, compareItem.group, compareItem.script, compareItem.offset);
        if (compareMsgIndex >= 0)
        {
            const FieldScriptMessage& compareFieldMsg = fieldData.messages[compareMsgIndex];

            if (fieldMsg.strOffset == compareFieldMsg.strOffset)
            {
                strMsgCount++;
            }
        }
    }

    for (const FieldScriptItem& compareItem : fieldData.materia)
    {
        std::string compareMateriaName = GameData::getMateriaName((uint8_t)compareItem.id);
        int compareMsgIndex = findPickUpMessage(compareMateriaName, compareItem.group, compareItem.script, compareItem.offset);
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
        overwriteMessages.push_back({ fieldMsg, newText });
        messagesToClear.push_back(fieldMsg);
    }
    else
    {
        game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, newText);
    }
}