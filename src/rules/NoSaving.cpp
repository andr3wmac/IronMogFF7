#include "NoSaving.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"

REGISTER_RULE("No Saving", NoSaving)

void NoSaving::setup()
{
    BIND_EVENT_ONE_ARG(game->onFrame, NoSaving::onFrame);
}

void NoSaving::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Menu && game->getGameModule() != GameModule::World)
    {
        return;
    }

    Flags<uint16_t> disabledOptions = game->read<uint16_t>(GameOffsets::MenuLockingMask);
    if (!disabledOptions.isBitSet(9))
    {
        disabledOptions.setBit(9, true);
        game->write<uint16_t>(GameOffsets::MenuLockingMask, disabledOptions.value());
    }
}