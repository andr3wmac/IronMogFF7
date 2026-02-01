#include "NoLimitBreaks.h"
#include "core/game/MemoryOffsets.h"

REGISTER_RULE(NoLimitBreaks, "No Limit Breaks", "Limit breaks are disabled.")

void NoLimitBreaks::setup()
{
    BIND_EVENT_ONE_ARG(game->onFrame, NoLimitBreaks::onFrame);
}

void NoLimitBreaks::onFrame(uint32_t frameNumber)
{
    // These only apply outside of battles.
    game->write<uint8_t>(CharacterDataOffsets::Cloud    + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Barret   + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Tifa     + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Aerith   + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::RedXIII  + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Yuffie   + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Cid      + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::CaitSith + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Vincent  + CharacterDataOffsets::CurrentLimitBar, 0);

    if (game->inBattle())
    {
        // These were found from memory scanning. Unsure what the actual structure is yet but it's obviously player 1, 2, 3.
        game->write<uint8_t>(0xF5E68, 0);
        game->write<uint8_t>(0xF5E9C, 0);
        game->write<uint8_t>(0xF5ED0, 0); 

        // The values above are the real limit break values, however the limit break display will only ever rise to match
        // that value it won't go back down so we set it to 0 here.
        game->write<uint8_t>(PlayerOffsets::Players[0] + PlayerOffsets::LimitBreakDisplay, 0);
        game->write<uint8_t>(PlayerOffsets::Players[1] + PlayerOffsets::LimitBreakDisplay, 0);
        game->write<uint8_t>(PlayerOffsets::Players[2] + PlayerOffsets::LimitBreakDisplay, 0);
    }
}