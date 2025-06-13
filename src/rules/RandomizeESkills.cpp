#include "RandomizeESkills.h"

REGISTER_RULE("Randomize E.Skills", RandomizeESkills)

void RandomizeESkills::onStart()
{
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeESkills::onFrame);
    BIND_EVENT(game->onBattleEnter, RandomizeESkills::onBattleEnter);
    BIND_EVENT(game->onBattleExit, RandomizeESkills::onBattleExit);
}

void RandomizeESkills::onFrame(uint32_t frameNumber)
{
    // The plan:
    // When the rule is enabled use the seed to generate a swap table that rearranges each enemy skill bit to a different one
    // When we enter a battle store the current state of any equipped eskill materia
    // Monitor the bit mask for changes, in the event it changes it means something attacked us and we learned a new eskill
    // Determine which bit flipped, and flip it back, then get the mapped bit and flip that one instead.
}

void RandomizeESkills::onBattleEnter()
{
    // Select a random item to replace each existing item with.
}

void RandomizeESkills::onBattleExit()
{

}