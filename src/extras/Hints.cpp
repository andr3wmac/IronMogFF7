#include "Hints.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

REGISTER_EXTRA("Hints", Hints)

void Hints::setup()
{
    BIND_EVENT(game->onStart, Hints::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, Hints::onFieldChanged);
}

void Hints::onStart()
{

}

void Hints::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    for (int i = 0; i < fieldData.messages.size(); ++i)
    {
        FieldMessage& fieldMsg = fieldData.messages[i];
        std::string msg = game->readString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength);

        if (msg.find("Turtle") != std::string::npos)
        {
            std::string newMsg = "HELLO WORLD FROM TURTLE PARADISE ONE TWO THREE FOUR FIX SIX WHAT WILL HAPPEN!";
            game->writeString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength, newMsg);
            LOG("FOUND TURTLE PARADISE MESSEAGE: %s", msg.c_str());
        }
    }
}