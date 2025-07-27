#pragma once
#include <cstdint>

#define CONST_U8  static constexpr uint8_t
#define CONST_U16 static constexpr uint16_t
#define CONST_PTR static constexpr uintptr_t

struct GameOffsets
{
    CONST_PTR FrameNumber       = 0x51568;
    CONST_PTR FieldID           = 0x9A05C;
    CONST_PTR MusicID           = 0x9A14E;
    CONST_PTR CurrentModule     = 0x9C560; 
    CONST_PTR PartyIDList       = 0x9CBDC; // 3 Bytes in a row.
    CONST_PTR Inventory         = 0x9CBE0; // 320 item list, 2 byte ids.
    CONST_PTR MateriaInventory  = 0x9C360; // 200 item list, 4 byte ids.
    CONST_PTR InGameTime        = 0x9D264; // 32-bit integer, in seconds
    CONST_PTR GameProgress      = 0x9D288; 
    CONST_PTR PauseMenuOptions  = 0x9D2A6; // Bitmask of options enabled in the menu.
};

struct GameModule
{
    CONST_U8 None   = 0;
    CONST_U8 Field  = 1;
    CONST_U8 Battle = 2;
    CONST_U8 World  = 3;
    CONST_U8 Menu   = 5;
};

struct FieldOffsets
{
    CONST_PTR FieldX = 0x74EB0;
    CONST_PTR FieldY = 0x74EB4;
    CONST_PTR FieldZ = 0x74EB8;
};

struct FieldScriptOffsets
{
    CONST_PTR ScriptStart = 0x115000;

    CONST_PTR ItemID = 0x02;
    CONST_PTR ItemQuantity = 0x04;

    // TODO: get the real number
    CONST_PTR MateriaID = 0x03;
};

// Character data exists for each of the cast of playabale characters and these stats are all saved onto memory card.
// When you enter a battle, relevant fields from this are copied into Battle Allies, etc
// Therefore, changing Current HP on character data while in a battle has no effect.
struct CharacterDataOffsets
{
    CONST_PTR Cloud     = 0x9C738;
    CONST_PTR Barret    = 0x9C7BC;
    CONST_PTR Tifa      = 0x9C840;
    CONST_PTR Aerith    = 0x9C8C4;
    CONST_PTR RedXIII   = 0x9C948;
    CONST_PTR Yuffie    = 0x9C9CC;
    CONST_PTR CaitSith  = 0x9CA50;
    CONST_PTR Vincent   = 0x9CAD4;
    CONST_PTR Cid       = 0x9CB58;

    CONST_PTR Characters[] = { Cloud, Barret, Tifa, Aerith, RedXIII, Yuffie, CaitSith, Vincent, Cid };

    CONST_PTR ID                = 0x00;
    CONST_PTR Level             = 0x01;
    CONST_PTR Strength          = 0x02;
    CONST_PTR Vitality          = 0x03;
    CONST_PTR Magic             = 0x04;
    CONST_PTR Spirit            = 0x05;
    CONST_PTR Dexterity         = 0x06;
    CONST_PTR Luck              = 0x07;
    CONST_PTR StrengthBonus     = 0x08;
    CONST_PTR VitalityBonus     = 0x09;
    CONST_PTR MagicBonus        = 0x0A;
    CONST_PTR SpiritBonus       = 0x0B;
    CONST_PTR DexterityBonus    = 0x0C;
    CONST_PTR LuckBonus         = 0x0D;
    CONST_PTR CurrentLimitLevel = 0x0E;
    CONST_PTR CurrentLimitBar   = 0x0F;
    CONST_PTR Name              = 0x10;
    CONST_PTR CurrentHP         = 0x2C;

    CONST_PTR WeaponMateria[] = { 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C };
    CONST_PTR ArmorMateria[]  = { 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C };
};

inline uintptr_t getCharacterDataOffset(uint8_t characterID)
{
    switch (characterID)
    {
        case 0:     return CharacterDataOffsets::Cloud;
        case 1:     return CharacterDataOffsets::Barret;
        case 2:     return CharacterDataOffsets::Tifa;
        case 3:     return CharacterDataOffsets::Aerith;
        case 4:     return CharacterDataOffsets::RedXIII;
        case 5:     return CharacterDataOffsets::Yuffie;
        case 9:     return CharacterDataOffsets::CaitSith;
        case 10:    return CharacterDataOffsets::Vincent;
    }

    return CharacterDataOffsets::Cloud;
}

struct PlayerOffsets
{
    CONST_PTR Players[] = { 0x9D84C, 0x9DC8C, 0x9E0CC };

    CONST_PTR CharacterID   = 0x00;
    CONST_PTR CoverChance   = 0x01;
    CONST_PTR Strength      = 0x02;
    CONST_PTR Vitality      = 0x03;
    CONST_PTR Magic         = 0x04;
    CONST_PTR Spirit        = 0x05;
    CONST_PTR Speed         = 0x06;
    CONST_PTR Luck          = 0x07;
    CONST_PTR PhysAttack    = 0x08;
    CONST_PTR PhysDefense   = 0x0A;
    CONST_PTR MagicAttack   = 0x0C;
    CONST_PTR MagicDefense  = 0x0E;
    CONST_PTR CurrentHP     = 0x10;
    CONST_PTR MaxHP         = 0x12;
    CONST_PTR CurrentMP     = 0x14;
    CONST_PTR MaxMP         = 0x16;

    CONST_PTR LimitBreakDisplay = 0x1B;
    CONST_PTR EnemySkillMenu    = 0x5B;
};

struct BattleCharacterOffsets
{
    // Battle Character Data Length = 104 bytes
    CONST_PTR Allies[] = { 0xF83E0, 0xF8448, 0xF84B0 };
    CONST_PTR Enemies[] = { 0xF8580, 0xF85E8, 0xF8650, 0xF86B8, 0xF8720, 0xF8788 };

    CONST_PTR Status    = 0x00;
    CONST_PTR Level     = 0x09;
    CONST_PTR Strength  = 0x0D;
    CONST_PTR Evade     = 0x0F;
    CONST_PTR Speed     = 0x14;
    CONST_PTR Luck      = 0x15;
    CONST_PTR ID        = 0x24;
    CONST_PTR CurrentHP = 0x2C;
    CONST_PTR MaxHP     = 0x30;

    // TODO: what do these do for allies?
    CONST_PTR Gil = 0x58;
    CONST_PTR Exp = 0x5C;
};

// Note: It looks like every formation can have a maximum of 3 enemies, but those enemies can be used multiple times.
// For instance if you have 6 of the same enemy in a fight all 6 of those will reference a single entry in formation data.
struct EnemyFormationOffsets
{
    CONST_PTR FormationID = 0x707BC;

    // Enemy Data Length = 184 bytes
    CONST_PTR Enemies[] = { 0xF5FCC, 0xF6084, 0xF613C };

    CONST_PTR DropRates[]   = { 0x00, 0x01, 0x02, 0x03 };
    CONST_PTR DropIDs[]     = { 0x04, 0x06, 0x08, 0x0A };   // First 4 bits specify the type (Item, Weapon, etc)
};

struct DropType
{
    CONST_U8 Accessory  = 0b1001;
    CONST_U8 Armor      = 0b1000;
    CONST_U8 Item       = 0b0000;
    CONST_U8 Weapon     = 0b0100;
};

inline std::pair<uint8_t, uint16_t> unpackDropID(uint16_t dropID)
{
    uint8_t dropType = (dropID >> 5) & 0x0F;
    uint16_t id = dropID & 0x1F;
    return { dropType, id };
}

inline uint16_t packDropID(uint8_t dropType, uint16_t id)
{
    return ((dropType & 0x0F) << 5) | (id & 0x1F);
}

struct StatusFlags
{
    CONST_U16 None       = 0;
    CONST_U16 Dead       = (1 << 0);
    CONST_U16 NearDeath  = (1 << 1);
    CONST_U16 Sleep      = (1 << 2);
    CONST_U16 Poison     = (1 << 3);
    CONST_U16 Sadness    = (1 << 4);
};

struct ShopOffsets
{
    CONST_PTR ShopStart  = 0x1D4714;
    CONST_PTR ShopStride = 84;

    // All prices are stored in uint32_t and are just an array of them in order of:
    // Items, Weapons, Armor, Accessories.
    CONST_PTR PricesStart = 0x1D6854;

    // All materia prices are stored in uint32_t
    CONST_PTR MateriaPricesStart = 0x1D6E54;
};