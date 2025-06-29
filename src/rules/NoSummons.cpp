#include "NoSummons.h"
#include "game/MemoryOffsets.h"
#include "rules/Restrictions.h"

REGISTER_RULE("No Summons", NoSummons)

void NoSummons::setup()
{
    BIND_EVENT_ONE_ARG(game->onFrame, NoSummons::onFrame);

    // Ban all summon materia, this will prevent them from being chosen by any randomizers.
    Restrictions::banMateria(74); // Choco/Mog
    Restrictions::banMateria(75); // Shiva
    Restrictions::banMateria(76); // Ifrit
    Restrictions::banMateria(77); // Ramuh
    Restrictions::banMateria(78); // Titan
    Restrictions::banMateria(79); // Odin
    Restrictions::banMateria(80); // Leviathan
    Restrictions::banMateria(81); // Bahamut
    Restrictions::banMateria(82); // Kjata
    Restrictions::banMateria(83); // Alexander
    Restrictions::banMateria(84); // Phoenix
    Restrictions::banMateria(85); // Neo Bahamut
    Restrictions::banMateria(86); // Hades
    Restrictions::banMateria(87); // Typoon
    Restrictions::banMateria(88); // Bahamut ZERO
    Restrictions::banMateria(89); // Knights of Round
    Restrictions::banMateria(90); // Master Summon

    if (!game->isRuleEnabled("Randomize Field Items"))
    {
        needsFieldChecks = true;
    }

    if (!game->isRuleEnabled("Randomize Shops"))
    {
        needsShopChecks = true;
    }
}

void NoSummons::onFrame(uint32_t frameNumber)
{

}