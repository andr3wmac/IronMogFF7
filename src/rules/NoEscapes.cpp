#include "NoEscapes.h"
#include "core/game/MemoryOffsets.h"
#include "rules/Restrictions.h"

REGISTER_RULE(NoEscapes, "No Escapes", "Escaping from battles is prohibited, including the use of Exit materia.")

void NoEscapes::setup()
{
    BIND_EVENT(game->onBattleEnter, NoEscapes::onBattleEnter);

    Restrictions::banItem(16);      // Smoke bomb
    Restrictions::banMateria(59);   // Exit
}

void NoEscapes::onBattleEnter()
{
    // TODO: this is apparently "not runnable due to pincer" there is probably a more proper
    // not runnable that's used for bosses we could find. Not sure if it matters.
    game->write<uint8_t>(0x163780, 2);
}