#include "NoSaving.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "utilities/Flags.h"

REGISTER_MOD(NoSaving, "No Saving", "Restrictions")

std::string NoSaving::getDescription() const
{
    return "Prevents saving the game during a run.\n\nThis restriction is useful for attempts where progress must be made without creating new saves.";
}

void NoSaving::setup()
{
    BIND_EVENT_ONE_ARG(game->onFrame, NoSaving::onFrame);
}

std::vector<std::string> NoSaving::describe(ModDescriptionType descType)
{
    if (descType == ModDescriptionType::Negation)
    {
        return { "Saving" };
    }

    return {};
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