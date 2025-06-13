#include "RandomizeFieldItems.h"
#include "game/MemoryOffsets.h"

REGISTER_RULE("Randomize Field Items", RandomizeFieldItems)

void RandomizeFieldItems::onStart()
{
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeFieldItems::onFieldChanged);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeFieldItems::onFrame);
    BIND_EVENT(game->onBattleExit, RandomizeFieldItems::onBattleExit);
}

void RandomizeFieldItems::onFieldChanged(uint16_t fieldID)
{
    framesUntilApply = 60;
}

void RandomizeFieldItems::onBattleExit()
{
    framesUntilApply = 60;
}

void RandomizeFieldItems::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Field)
    {
        return;
    }

    // We delay the application of the randomization until X amount of frames have passsed
    // since the event to trigger it, this gives the game enough time to finish loading the field.
    if (framesUntilApply > 0)
    {
        framesUntilApply--;

        if (framesUntilApply == 0)
        {
            randomizeFieldItems(game->getFieldID());
        }
    }
}

void RandomizeFieldItems::randomizeFieldItems(uint16_t fieldID)
{
    if (fieldID == 123)
    {
        uintptr_t itemIDOffset = 0x1158BE;
        uintptr_t itemQuantityOffset = 0x1158C0;

        // Testing for now
        game->write<uint16_t>(itemIDOffset, 5);
        game->write<uint8_t>(itemQuantityOffset, 2);
    }
}