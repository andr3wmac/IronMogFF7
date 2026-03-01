#include "BanMateria.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "rules/Restrictions.h"

#include <imgui.h>

REGISTER_RULE(BanMateria, "Ban Materia", "Restricts the types of materia that can be found or purchased.")

void BanMateria::setup()
{
    if (noSummons)
    {
        Restrictions::banMateria(20); // W-Summon
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
    }

    if (noMagic)
    {
        Restrictions::banMateria(49); // Fire
        Restrictions::banMateria(50); // Ice
        Restrictions::banMateria(51); // Earth
        Restrictions::banMateria(52); // Lightning
        Restrictions::banMateria(53); // Restore
        Restrictions::banMateria(54); // Heal
        Restrictions::banMateria(55); // Revive
        Restrictions::banMateria(56); // Seal
        Restrictions::banMateria(57); // Mystify
        Restrictions::banMateria(58); // Transform
        Restrictions::banMateria(59); // Exit
        Restrictions::banMateria(60); // Poison
        Restrictions::banMateria(61); // Gravity
        Restrictions::banMateria(62); // Barrier
        Restrictions::banMateria(64); // Comet
        Restrictions::banMateria(65); // Time
        Restrictions::banMateria(68); // Destruct
        Restrictions::banMateria(69); // Contain
        Restrictions::banMateria(70); // Full Cure
        Restrictions::banMateria(71); // Shield
        Restrictions::banMateria(72); // Ultima
        Restrictions::banMateria(73); // Master Magic
    }

    if (noCommand)
    {
        Restrictions::banMateria(14); // Slash-All
        Restrictions::banMateria(15); // Double Cut
        Restrictions::banMateria(19); // W-Magic
        Restrictions::banMateria(20); // W-Summon
        Restrictions::banMateria(21); // W-Item
        Restrictions::banMateria(36); // Steal
        Restrictions::banMateria(37); // Sense
        Restrictions::banMateria(39); // Throw
        Restrictions::banMateria(40); // Morph
        Restrictions::banMateria(41); // Deathblow
        Restrictions::banMateria(42); // Manipulate
        Restrictions::banMateria(43); // Mime
        Restrictions::banMateria(44); // Enemy Skill
        Restrictions::banMateria(48); // Master Command
    }

    if (noSupport)
    {
        Restrictions::banMateria(23); // All
        Restrictions::banMateria(24); // Counter
        Restrictions::banMateria(25); // Magic Counter
        Restrictions::banMateria(26); // MP Turbo
        Restrictions::banMateria(27); // MP Absorb
        Restrictions::banMateria(28); // HP Absorb
        Restrictions::banMateria(29); // Elemental
        Restrictions::banMateria(30); // Added Effect
        Restrictions::banMateria(31); // Sneak Attack
        Restrictions::banMateria(32); // Final Attack
        Restrictions::banMateria(33); // Added Cut
        Restrictions::banMateria(34); // Steal as well
        Restrictions::banMateria(35); // Quadra Magic
    }

    if (noIndependent)
    {
        Restrictions::banMateria(0); // MP Plus
        Restrictions::banMateria(1); // HP Plus
        Restrictions::banMateria(2); // Speed Plus
        Restrictions::banMateria(3); // Magic Plus
        Restrictions::banMateria(4); // Luck Plus
        Restrictions::banMateria(5); // EXP Plus
        Restrictions::banMateria(6); // Gil Plus
        Restrictions::banMateria(7); // Enemy Away
        Restrictions::banMateria(8); // Enemy Lure
        Restrictions::banMateria(9); // Chocobo Lure
        Restrictions::banMateria(10); // Pre-Emptive
        Restrictions::banMateria(11); // Long Range
        Restrictions::banMateria(12); // Mega All
        Restrictions::banMateria(13); // Counter Attack
        Restrictions::banMateria(14); // Slash-All
        Restrictions::banMateria(15); // Double Cut
        Restrictions::banMateria(16); // Cover
        Restrictions::banMateria(17); // Underwater
        Restrictions::banMateria(18); // HP<->MP
    }

    if (noESkill)
    {
        Restrictions::banMateria(44); // Enemy Skill
    }
}

bool BanMateria::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Summon", &noSummons);
    changed |= ImGui::Checkbox("Magic", &noMagic);
    changed |= ImGui::Checkbox("Command", &noCommand);
    changed |= ImGui::Checkbox("Support", &noSupport);
    changed |= ImGui::Checkbox("Independent", &noIndependent);
    changed |= ImGui::Checkbox("E.Skill", &noESkill);

    return changed;
}

void BanMateria::loadSettings(const ConfigFile& cfg)
{
    noSummons     = cfg.get<bool>("noSummons", noSummons);
    noMagic       = cfg.get<bool>("noMagic", noMagic);
    noCommand     = cfg.get<bool>("noCommand", noCommand);
    noSupport     = cfg.get<bool>("noSupport", noSupport);
    noIndependent = cfg.get<bool>("noIndependent", noIndependent);
    noESkill      = cfg.get<bool>("noESkill", noESkill);
}

void BanMateria::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("noSummons", noSummons);
    cfg.set<bool>("noMagic", noMagic);
    cfg.set<bool>("noCommand", noCommand);
    cfg.set<bool>("noSupport", noSupport);
    cfg.set<bool>("noIndependent", noIndependent);
    cfg.set<bool>("noESkill", noESkill);
}