#include "BanItemsAndMateria.h"
#include "AppFrame/AppFrame.h"
#include "LiveModFF7/game/GameData.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"
#include "rules/Restrictions.h"

REGISTER_RULE(BanItemsAndMateria, "Ban Items & Materia", "Restricts the types of items and materia that can be found, purchased, or dropped.")

void BanItemsAndMateria::setup()
{
    if (noItems)
    {
        Restrictions::banItem(0); // Potion
        Restrictions::banItem(1); // Hi-Potion
        Restrictions::banItem(2); // X-Potion
        Restrictions::banItem(3); // Ether
        Restrictions::banItem(4); // Turbo Ether
        Restrictions::banItem(5); // Elixir
        Restrictions::banItem(6); // Megalixir
        Restrictions::banItem(7); // Phoenix Down
        Restrictions::banItem(8); // Antidote
        Restrictions::banItem(9); // Soft
        Restrictions::banItem(10); // Maiden's Kiss
        Restrictions::banItem(11); // Cornucopia
        Restrictions::banItem(12); // Echo Screen
        Restrictions::banItem(13); // Hyper
        Restrictions::banItem(14); // Tranquilizer
        Restrictions::banItem(15); // Remedy
        Restrictions::banItem(16); // Smoke Bomb
        Restrictions::banItem(17); // Speed Drink
        Restrictions::banItem(18); // Hero Drink
        Restrictions::banItem(19); // Vaccine
        Restrictions::banItem(20); // Grenade
        Restrictions::banItem(21); // Shrapnel
        Restrictions::banItem(22); // Right arm
        Restrictions::banItem(23); // Hourglass
        Restrictions::banItem(24); // Kiss of Death
        Restrictions::banItem(25); // Spider Web
        Restrictions::banItem(26); // Dream Powder
        Restrictions::banItem(27); // Mute Mask
        Restrictions::banItem(28); // War Gong
        Restrictions::banItem(29); // Loco weed
        Restrictions::banItem(30); // Fire Fang
        Restrictions::banItem(31); // Fire Veil
        Restrictions::banItem(32); // Antarctic Wind
        Restrictions::banItem(33); // Ice Crystal
        Restrictions::banItem(34); // Bolt Plume
        Restrictions::banItem(35); // Swift Bolt
        Restrictions::banItem(36); // Earth Drum
        Restrictions::banItem(37); // Earth Mallet
        Restrictions::banItem(38); // Deadly Waste
        Restrictions::banItem(39); // M-Tentacles
        Restrictions::banItem(40); // Stardust
        Restrictions::banItem(41); // Vampire Fang
        Restrictions::banItem(42); // Ghost Hand
        Restrictions::banItem(43); // Vagyrisk Claw
        Restrictions::banItem(44); // Light Curtain
        Restrictions::banItem(45); // Lunar Curtain
        Restrictions::banItem(46); // Mirror
        Restrictions::banItem(47); // Holy Torch
        Restrictions::banItem(48); // Bird Wing
        Restrictions::banItem(49); // Dragon Scales
        Restrictions::banItem(50); // Impaler
        Restrictions::banItem(51); // Shrivel
        Restrictions::banItem(52); // Eye drop
        Restrictions::banItem(53); // Molotov
        Restrictions::banItem(54); // S-mine
        Restrictions::banItem(55); // 8inch Cannon
        Restrictions::banItem(56); // Graviball
        Restrictions::banItem(57); // T/S Bomb
        Restrictions::banItem(58); // Ink
        Restrictions::banItem(59); // Dazers
        Restrictions::banItem(60); // Dragon Fang
        Restrictions::banItem(61); // Cauldron
        Restrictions::banItem(62); // Sylkis Greens
        Restrictions::banItem(63); // Reagan Greens
        Restrictions::banItem(64); // Mimett Greens
        Restrictions::banItem(65); // Curiel Greens
        Restrictions::banItem(66); // Pahsana Greens
        Restrictions::banItem(67); // Tantal Greens
        Restrictions::banItem(68); // Krakka Greens
        Restrictions::banItem(69); // Gysahl Greens
        Restrictions::banItem(70); // Tent
        Restrictions::banItem(71); // Power Source
        Restrictions::banItem(72); // Guard Source
        Restrictions::banItem(73); // Magic Source
        Restrictions::banItem(74); // Mind Source
        Restrictions::banItem(75); // Speed Source
        Restrictions::banItem(76); // Luck Source
        Restrictions::banItem(77); // Zeio Nut
        Restrictions::banItem(78); // Carob Nut
        Restrictions::banItem(79); // Porov Nut
        Restrictions::banItem(80); // Pram Nut
        Restrictions::banItem(81); // Lasan Nut
        Restrictions::banItem(82); // Saraha Nut
        Restrictions::banItem(83); // Luchile Nut
        Restrictions::banItem(84); // Pepio Nut
        Restrictions::banItem(85); // Battery
        Restrictions::banItem(86); // Tissue
        Restrictions::banItem(87); // Omnislash
        Restrictions::banItem(88); // Catastrophe
        Restrictions::banItem(89); // Final Heaven
        Restrictions::banItem(90); // Great Gospel
        Restrictions::banItem(91); // Cosmo Memory
        Restrictions::banItem(92); // All Creation
        Restrictions::banItem(93); // Chaos
        Restrictions::banItem(94); // Highwind
        Restrictions::banItem(95); // 1/35 soldier
        Restrictions::banItem(96); // Super Sweeper
        Restrictions::banItem(97); // Masamune Blade
        Restrictions::banItem(98); // Save Crystal
        Restrictions::banItem(99); // Combat Diary
        Restrictions::banItem(100); // Autograph
        Restrictions::banItem(101); // Gambler
        Restrictions::banItem(102); // Desert Rose
        Restrictions::banItem(103); // Earth Harp
        Restrictions::banItem(104); // Guide Book
    }

    if (noWeapons)
    {
        Restrictions::banItem(128); // Buster Sword
        Restrictions::banItem(129); // Mythril Saber
        Restrictions::banItem(130); // Hardedge
        Restrictions::banItem(131); // Butterfly Edge
        Restrictions::banItem(132); // Enhance Sword
        Restrictions::banItem(133); // Organics
        Restrictions::banItem(134); // Crystal Sword
        Restrictions::banItem(135); // Force Stealer
        Restrictions::banItem(136); // Rune Blade
        Restrictions::banItem(137); // Murasame
        Restrictions::banItem(138); // Nail Bat
        Restrictions::banItem(139); // Yoshiyuki
        Restrictions::banItem(140); // Apocalypse
        Restrictions::banItem(141); // Heaven's Cloud
        Restrictions::banItem(142); // Ragnarok
        Restrictions::banItem(143); // Ultima Weapon
        Restrictions::banItem(144); // Leather Glove
        Restrictions::banItem(145); // Metal Knuckle
        Restrictions::banItem(146); // Mythril Claw
        Restrictions::banItem(147); // Grand Glove
        Restrictions::banItem(148); // Tiger Fang
        Restrictions::banItem(149); // Diamond Knuckle
        Restrictions::banItem(150); // Dragon Claw
        Restrictions::banItem(151); // Crystal Glove
        Restrictions::banItem(152); // Motor Drive
        Restrictions::banItem(153); // Platinum Fist
        Restrictions::banItem(154); // Kaiser Knuckle
        Restrictions::banItem(155); // Work Glove
        Restrictions::banItem(156); // Powersoul
        Restrictions::banItem(157); // Master Fist
        Restrictions::banItem(158); // God's Hand
        Restrictions::banItem(159); // Premium Heart
        Restrictions::banItem(160); // Gatling Gun
        Restrictions::banItem(161); // Assault Gun
        Restrictions::banItem(162); // Cannon Ball
        Restrictions::banItem(163); // Atomic Scissors
        Restrictions::banItem(164); // Heavy Vulcan
        Restrictions::banItem(165); // Chainsaw
        Restrictions::banItem(166); // Microlaser
        Restrictions::banItem(167); // AM Cannon
        Restrictions::banItem(168); // W Machine Gun
        Restrictions::banItem(169); // Drill Arm
        Restrictions::banItem(170); // Solid Bazooka
        Restrictions::banItem(171); // Rocket Punch
        Restrictions::banItem(172); // Enemy Launcher
        Restrictions::banItem(173); // Pile Banger
        Restrictions::banItem(174); // Max Ray
        Restrictions::banItem(175); // Missing Score
        Restrictions::banItem(176); // Mythril Clip
        Restrictions::banItem(177); // Diamond Pin
        Restrictions::banItem(178); // Silver Barrette
        Restrictions::banItem(179); // Gold Barrette
        Restrictions::banItem(180); // Adaman Clip
        Restrictions::banItem(181); // Crystal Comb
        Restrictions::banItem(182); // Magic Comb
        Restrictions::banItem(183); // Plus Barrette
        Restrictions::banItem(184); // Centclip
        Restrictions::banItem(185); // Hairpin
        Restrictions::banItem(186); // Seraph Comb
        Restrictions::banItem(187); // Behemoth Horn
        Restrictions::banItem(188); // Spring Gun Clip
        Restrictions::banItem(189); // Limited Moon
        Restrictions::banItem(190); // Guard Stick
        Restrictions::banItem(191); // Mythril Rod
        Restrictions::banItem(192); // Full Metal Staff
        Restrictions::banItem(193); // Striking Staff
        Restrictions::banItem(194); // Prism Staff
        Restrictions::banItem(195); // Aurora Rod
        Restrictions::banItem(196); // Wizard Staff
        Restrictions::banItem(197); // Wizer Staff
        Restrictions::banItem(198); // Fairy Tale
        Restrictions::banItem(199); // Umbrella
        Restrictions::banItem(200); // Princess Guard
        Restrictions::banItem(201); // Spear
        Restrictions::banItem(202); // Slash Lance
        Restrictions::banItem(203); // Trident
        Restrictions::banItem(204); // Mast Ax
        Restrictions::banItem(205); // Partisan
        Restrictions::banItem(206); // Viper Halberd
        Restrictions::banItem(207); // Javelin
        Restrictions::banItem(208); // Grow Lance
        Restrictions::banItem(209); // Mop
        Restrictions::banItem(210); // Dragoon Lance
        Restrictions::banItem(211); // Scimitar
        Restrictions::banItem(212); // Flayer
        Restrictions::banItem(213); // Spirit Lance
        Restrictions::banItem(214); // Venus Gospel
        Restrictions::banItem(215); // 4-point Shuriken
        Restrictions::banItem(216); // Boomerang
        Restrictions::banItem(217); // Pinwheel
        Restrictions::banItem(218); // Razor Ring
        Restrictions::banItem(219); // Hawkeye
        Restrictions::banItem(220); // Crystal Cross
        Restrictions::banItem(221); // Wind Slash
        Restrictions::banItem(222); // Twin Viper
        Restrictions::banItem(223); // Spiral Shuriken
        Restrictions::banItem(224); // Superball
        Restrictions::banItem(225); // Magic Shuriken
        Restrictions::banItem(226); // Rising Sun
        Restrictions::banItem(227); // Oritsuru
        Restrictions::banItem(228); // Conformer
        Restrictions::banItem(229); // Yellow M-phone
        Restrictions::banItem(230); // Green M-phone
        Restrictions::banItem(231); // Blue M-phone
        Restrictions::banItem(232); // Red M-phone
        Restrictions::banItem(233); // Crystal M-phone
        Restrictions::banItem(234); // White M-phone
        Restrictions::banItem(235); // Black M-phone
        Restrictions::banItem(236); // Silver M-phone
        Restrictions::banItem(237); // Trumpet Shell
        Restrictions::banItem(238); // Gold M-phone
        Restrictions::banItem(239); // Battle Trumpet
        Restrictions::banItem(240); // Starlight Phone
        Restrictions::banItem(241); // HP Shout
        Restrictions::banItem(242); // Quicksilver
        Restrictions::banItem(243); // Shotgun
        Restrictions::banItem(244); // Shortbarrel
        Restrictions::banItem(245); // Lariat
        Restrictions::banItem(246); // Winchester
        Restrictions::banItem(247); // Peacemaker
        Restrictions::banItem(248); // Buntline
        Restrictions::banItem(249); // Long Barrel R
        Restrictions::banItem(250); // Silver Rifle
        Restrictions::banItem(251); // Sniper CR
        Restrictions::banItem(252); // Supershot ST
        Restrictions::banItem(253); // Outsider
        Restrictions::banItem(254); // Death Penalty
        Restrictions::banItem(255); // Masamune
    }

    if (noArmor)
    {
        Restrictions::banItem(256); // Bronze Bangle
        Restrictions::banItem(257); // Iron Bangle
        Restrictions::banItem(258); // Titan Bangle
        Restrictions::banItem(259); // Mythril Armlet
        Restrictions::banItem(260); // Carbon Bangle
        Restrictions::banItem(261); // Silver Armlet
        Restrictions::banItem(262); // Gold Armlet
        Restrictions::banItem(263); // Diamond Bangle
        Restrictions::banItem(264); // Crystal Bangle
        Restrictions::banItem(265); // Platinum Bangle
        Restrictions::banItem(266); // Rune Armlet
        Restrictions::banItem(267); // Edincoat
        Restrictions::banItem(268); // Wizard Bracelet
        Restrictions::banItem(269); // Adaman Bangle
        Restrictions::banItem(270); // Gigas Armlet
        Restrictions::banItem(271); // Imperial Guard
        Restrictions::banItem(272); // Aegis Armlet
        Restrictions::banItem(273); // Fourth Bracelet
        Restrictions::banItem(274); // Warrior Bangle
        Restrictions::banItem(275); // Shinra Beta
        Restrictions::banItem(276); // Shinra Alpha
        Restrictions::banItem(277); // Four Slots
        Restrictions::banItem(278); // Fire Armlet
        Restrictions::banItem(279); // Aurora Armlet
        Restrictions::banItem(280); // Bolt Armlet
        Restrictions::banItem(281); // Dragon Armlet
        Restrictions::banItem(282); // Minerva Band
        Restrictions::banItem(283); // Escort Guard
        Restrictions::banItem(284); // Mystile
        Restrictions::banItem(285); // Ziedrich
        Restrictions::banItem(286); // Precious Watch
        Restrictions::banItem(287); // Chocobracelet
    }

    if (noAccessories)
    {
        Restrictions::banItem(288); // Power Wrist
        Restrictions::banItem(289); // Protect Vest
        Restrictions::banItem(290); // Earring
        Restrictions::banItem(291); // Talisman
        Restrictions::banItem(292); // Choco Feather
        Restrictions::banItem(293); // Amulet
        Restrictions::banItem(294); // Champion Belt
        Restrictions::banItem(295); // Poison Ring
        Restrictions::banItem(296); // Touph Ring
        Restrictions::banItem(297); // Circlet
        Restrictions::banItem(298); // Star Pendant
        Restrictions::banItem(299); // Silver Glasses
        Restrictions::banItem(300); // Headband
        Restrictions::banItem(301); // Fairy Ring
        Restrictions::banItem(302); // Jem Ring
        Restrictions::banItem(303); // White Cape
        Restrictions::banItem(304); // Sprint Shoes
        Restrictions::banItem(305); // Peace Ring
        Restrictions::banItem(306); // Ribbon
        Restrictions::banItem(307); // Fire Ring
        Restrictions::banItem(308); // Ice Ring
        Restrictions::banItem(309); // Bolt Ring
        Restrictions::banItem(310); // Tetra Elemental
        Restrictions::banItem(311); // Safety Bit
        Restrictions::banItem(312); // Fury Ring
        Restrictions::banItem(313); // Curse Ring
        Restrictions::banItem(314); // Protect Ring
        Restrictions::banItem(315); // Cat's Bell
        Restrictions::banItem(316); // Reflect Ring
        Restrictions::banItem(317); // Water Ring
        Restrictions::banItem(318); // Sneak Glove
        Restrictions::banItem(319); // HypnoCrown
    }

    if (noSummon)
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

    if (noEnemySkill)
    {
        Restrictions::banMateria(44); // Enemy Skill
    }

    if (noMaster)
    {
        Restrictions::banMateria(48); // Master Command
        Restrictions::banMateria(73); // Master Magic
        Restrictions::banMateria(90); // Master Summon
    }
}

bool BanItemsAndMateria::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Items", &noItems);
    changed |= ImGui::Checkbox("Weapons", &noWeapons);
    changed |= ImGui::Checkbox("Armor", &noArmor);
    changed |= ImGui::Checkbox("Accessories", &noAccessories);

    ImGui::SeparatorText("Materia");

    changed |= ImGui::Checkbox("Summon", &noSummon);
    changed |= ImGui::Checkbox("Magic", &noMagic);
    changed |= ImGui::Checkbox("Command", &noCommand);
    changed |= ImGui::Checkbox("Support", &noSupport);
    changed |= ImGui::Checkbox("Independent", &noIndependent);
    changed |= ImGui::Checkbox("Enemy Skill", &noEnemySkill);
    changed |= ImGui::Checkbox("Master", &noMaster);

    return changed;
}

void BanItemsAndMateria::loadSettings(const ConfigFile& cfg)
{
    noItems       = cfg.get<bool>("noItems", noItems);
    noWeapons     = cfg.get<bool>("noWeapons", noWeapons);
    noArmor       = cfg.get<bool>("noArmor", noArmor);
    noAccessories = cfg.get<bool>("noAccessories", noAccessories);

    noSummon      = cfg.get<bool>("noSummon", noSummon);
    noMagic       = cfg.get<bool>("noMagic", noMagic);
    noCommand     = cfg.get<bool>("noCommand", noCommand);
    noSupport     = cfg.get<bool>("noSupport", noSupport);
    noIndependent = cfg.get<bool>("noIndependent", noIndependent);
    noEnemySkill  = cfg.get<bool>("noEnemySkill", noEnemySkill);
    noMaster      = cfg.get<bool>("noMaster", noMaster);
}

void BanItemsAndMateria::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("noItems", noItems);
    cfg.set<bool>("noWeapons", noWeapons);
    cfg.set<bool>("noArmor", noArmor);
    cfg.set<bool>("noAccessories", noAccessories);

    cfg.set<bool>("noSummon", noSummon);
    cfg.set<bool>("noMagic", noMagic);
    cfg.set<bool>("noCommand", noCommand);
    cfg.set<bool>("noSupport", noSupport);
    cfg.set<bool>("noIndependent", noIndependent);
    cfg.set<bool>("noEnemySkill", noEnemySkill);
    cfg.set<bool>("noMaster", noMaster);
}

std::vector<std::string> BanItemsAndMateria::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::BanItems)
    {
        std::vector<std::string> results;
        if (noItems) results.push_back("Items");
        if (noWeapons) results.push_back("Weapons");
        if (noArmor) results.push_back("Armor");
        if (noAccessories) results.push_back("Accessories");
        return results;
    }

    if (descType == RuleDescripionType::BanMateria)
    {
        std::vector<std::string> results;
        if (noSummon) results.push_back("Summon");
        if (noMagic) results.push_back("Magic");
        if (noCommand) results.push_back("Command");
        if (noSupport) results.push_back("Support");
        if (noIndependent) results.push_back("Independent");
        if (noEnemySkill) results.push_back("Enemy Skill");
        if (noMaster) results.push_back("Master");
        return results;
    }

    return {};
}
