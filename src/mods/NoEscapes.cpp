#include "NoEscapes.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "mods/Restrictions.h"

REGISTER_MOD(NoEscapes, "No Escapes", "Restrictions")

std::string NoEscapes::getDescription() const
{
    return "Prevents escaping from battles, including escapes attempted with Exit materia.\n\nEvery encounter must be resolved without fleeing.";
}

void NoEscapes::setup()
{
    BIND_EVENT(game->onBattleEnter, NoEscapes::onBattleEnter);

    Restrictions::banItem(16);      // Smoke bomb
    Restrictions::banMateria(59);   // Exit
}

std::vector<std::string> NoEscapes::describe(ModDescriptionType descType)
{
    if (descType == ModDescriptionType::Negation)
    {
        return { "Escapes" };
    }

    return {};
}

void NoEscapes::onBattleEnter()
{
    // TODO: this is apparently "not runnable due to pincer" there is probably a more proper
    // not runnable that's used for bosses we could find. Not sure if it matters.
    game->write<uint8_t>(0x163780, 2);
}