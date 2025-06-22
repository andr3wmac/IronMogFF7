#pragma once
#include <cstdint>

#define OFF_CONST static constexpr uintptr_t
#define STATUS_FLAG static constexpr uint16_t
#define MODULE_CONST static constexpr uint8_t

#define CONST_U8  static constexpr uint8_t
#define CONST_U16 static constexpr uint16_t
#define CONST_PTR static constexpr uintptr_t

struct GameOffsets
{
    OFF_CONST FrameNumber       = 0x51568;
    OFF_CONST FieldID           = 0x9A05C;
    OFF_CONST CurrentModule     = 0x9C560; 
    OFF_CONST PartyIDList       = 0x9CBDC; // 3 Bytes in a row.
    OFF_CONST Inventory         = 0x9CBE0; // 320 item list, 2 byte ids.
    OFF_CONST MateriaInventory  = 0x9C360; // 200 item list, 4 byte ids.
    OFF_CONST PauseMenuOptions  = 0x9D2A6; // Bitmask of options enabled in the menu.
};

struct GameModule
{
    MODULE_CONST None   = 0;
    MODULE_CONST Field  = 1;
    MODULE_CONST Battle = 2;
    MODULE_CONST World  = 3;
    MODULE_CONST Menu   = 5;
};

struct FieldOffsets
{
    OFF_CONST FieldX = 0x74EB0;
    OFF_CONST FieldY = 0x74EB4;
    OFF_CONST FieldZ = 0x74EB8;
};

struct FieldScriptOffsets
{
    OFF_CONST ScriptStart = 0x115000;

    OFF_CONST ItemID = 0x02;
    OFF_CONST ItemQuantity = 0x04;
};

// Character data exists for each of the cast of playabale characters and these stats are all saved onto memory card.
// When you enter a battle, relevant fields from this are copied into Battle Allies, etc
// Therefore, changing Current HP on character data while in a battle has no effect.
struct CharacterDataOffsets
{
    OFF_CONST Cloud     = 0x9C738;
    OFF_CONST Barret    = 0x9C7BC;
    OFF_CONST Tifa      = 0x9C840;
    OFF_CONST Aerith    = 0x9C8C4;
    OFF_CONST RedXIII   = 0x9C948;
    OFF_CONST Yuffie    = 0x9C9CC;
    OFF_CONST CaitSith  = 0x9CA50;
    OFF_CONST Vincent   = 0x9CAD4;

    OFF_CONST ID                = 0x00;
    OFF_CONST Level             = 0x01;
    OFF_CONST Strength          = 0x02;
    OFF_CONST Vitality          = 0x03;
    OFF_CONST Magic             = 0x04;
    OFF_CONST Spirit            = 0x05;
    OFF_CONST Dexterity         = 0x06;
    OFF_CONST Luck              = 0x07;
    OFF_CONST StrengthBonus     = 0x08;
    OFF_CONST VitalityBonus     = 0x09;
    OFF_CONST MagicBonus        = 0x0A;
    OFF_CONST SpiritBonus       = 0x0B;
    OFF_CONST DexterityBonus    = 0x0C;
    OFF_CONST LuckBonus         = 0x0D;
    OFF_CONST CurrentLimitLevel = 0x0E;
    OFF_CONST CurrentLimitBar   = 0x0F;
    OFF_CONST Name              = 0x10;

    OFF_CONST CurrentHP         = 0x2C;
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
    OFF_CONST Players[] = { 0x9D84C, 0x9DC8C, 0x9E0CC };

    OFF_CONST CharacterID   = 0x00;
    OFF_CONST CoverChance   = 0x01;
    OFF_CONST Strength      = 0x02;
    OFF_CONST Vitality      = 0x03;
    OFF_CONST Magic         = 0x04;
    OFF_CONST Spirit        = 0x05;
    OFF_CONST Speed         = 0x06;
    OFF_CONST Luck          = 0x07;
    OFF_CONST PhysAttack    = 0x08;
    OFF_CONST PhysDefense   = 0x0A;
    OFF_CONST MagicAttack   = 0x0C;
    OFF_CONST MagicDefense  = 0x0E;
    OFF_CONST CurrentHP     = 0x10;
    OFF_CONST MaxHP         = 0x12;
    OFF_CONST CurrentMP     = 0x14;
    OFF_CONST MaxMP         = 0x16;

    OFF_CONST LimitBreakDisplay = 0x1B;
};

struct BattleCharacterOffsets
{
    // Battle Character Data Length = 104 bytes
    OFF_CONST Allies[] = { 0xF83E0, 0xF8448, 0xF84B0 };
    OFF_CONST Enemies[] = { 0xF8580, 0xF85E8, 0xF8650, 0xF86B8, 0xF8720, 0xF8788 };

    OFF_CONST Status    = 0x00;
    OFF_CONST Level     = 0x09;
    OFF_CONST Strength  = 0x0D;
    OFF_CONST Evade     = 0x0F;
    OFF_CONST Speed     = 0x14;
    OFF_CONST Luck      = 0x15;
    OFF_CONST ID        = 0x24;
    OFF_CONST CurrentHP = 0x2C;
    OFF_CONST MaxHP     = 0x30;

    // TODO: what do these do for allies?
    OFF_CONST Gil = 0x58;
    OFF_CONST Exp = 0x5C;
};

// Note: It looks like every formation can have a maximum of 3 enemies, but those enemies can be used multiple times.
// For instance if you have 6 of the same enemy in a fight all 6 of those will reference a single entry in formation data.
struct EnemyFormationOffsets
{
    OFF_CONST FormationID = 0x707BC;

    // Enemy Data Length = 184 bytes
    OFF_CONST Enemies[] = { 0xF5FCC, 0xF6084, 0xF613C };

    OFF_CONST DropRates[]   = { 0x00, 0x01, 0x02, 0x03 };
    OFF_CONST DropIDs[]     = { 0x04, 0x06, 0x08, 0x0A };   // First 4 bits specify the type (Item, Weapon, etc) remaining 
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
    STATUS_FLAG None       = 0;
    STATUS_FLAG Dead       = (1 << 0);
    STATUS_FLAG NearDeath  = (1 << 1);
    STATUS_FLAG Sleep      = (1 << 2);
    STATUS_FLAG Poison     = (1 << 3);
    STATUS_FLAG Sadness    = (1 << 4);
};