#include "NoSummons.h"
#include "game/MemoryOffsets.h"

REGISTER_RULE("No Summons", NoSummons)

void NoSummons::onStart()
{
    BIND_EVENT_ONE_ARG(game->onFrame, NoSummons::onFrame);
}

void NoSummons::onFrame(uint32_t frameNumber)
{

}