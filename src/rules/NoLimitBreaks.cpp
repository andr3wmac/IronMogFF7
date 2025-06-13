#include "NoLimitBreaks.h"
#include "game/MemoryOffsets.h"

REGISTER_RULE("No Limit Breaks", NoLimitBreaks)

void NoLimitBreaks::onStart()
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
    game->write<uint8_t>(CharacterDataOffsets::CaitSith + CharacterDataOffsets::CurrentLimitBar, 0);
    game->write<uint8_t>(CharacterDataOffsets::Vincent  + CharacterDataOffsets::CurrentLimitBar, 0);

    // HACK: Somewhere there is a real variable tracking limit break for each player in battle but I haven't
    // found it yet. The values being written to below are seemingly just holding the bar at 0 so it never
    // fills up and doesn't reveal the limit menu. If we stop writing these every frame the limit bar will
    // do the fill animation to where its actually supposed to be.

    if (game->inBattle())
    {
        // These make the bar display as empty in the battle UI.
        game->write<uint8_t>(PlayerOffsets::Players[0] + PlayerOffsets::LimitBreakDisplay, 0);
        game->write<uint8_t>(PlayerOffsets::Players[1] + PlayerOffsets::LimitBreakDisplay, 0);
        game->write<uint8_t>(PlayerOffsets::Players[2] + PlayerOffsets::LimitBreakDisplay, 0);

        // These were lifted from gameshark codes. Unsure what the actual structure is yet but it's obviously player 1, 2, 3.
        game->write<uint8_t>(0xF5E6A, 0);
        game->write<uint8_t>(0xF5E9C, 0);
        game->write<uint8_t>(0xF5ED2, 0);
    }
}