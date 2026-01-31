#pragma once
#include <cstdint>

#define CONST_U8  static constexpr uint8_t
#define CONST_U16 static constexpr uint16_t
#define CONST_PTR static constexpr uintptr_t

struct GameOffsets
{
    CONST_PTR FrameNumber       = 0x51568;  // uint32_t
    CONST_PTR MusicVolume       = 0x62F5E;  // uint16_t controlling global music volume
    CONST_PTR NextFormationID   = 0x707BC;  // uint16_t formation id of the next random encounter
    CONST_PTR MusicLock         = 0x716D4;  // uint8_t 1 is locked, 0 is unlocked
    CONST_PTR FieldID           = 0x9A05C;  // uint16_t
    CONST_PTR MusicID           = 0x9A14E;  // uint16_t id of currently playing music
    CONST_PTR MenuType          = 0x9ABF5;  // uint8_t 8 = shop, 9 = main menu
    CONST_PTR FieldWarpTrigger  = 0x9ABF5;  // uint8_t, set to 1 to warp to field warp ID below
    CONST_PTR FieldWarpID       = 0x9ABF6;  // uint16_t
    CONST_PTR FieldScreenFade   = 0x9AC42;  // uint16_t, 0 - 256 how much screen is faded for loading transitions in fields
    CONST_PTR CurrentModule     = 0x9C560;  // uint8_t
    CONST_PTR PartyIDList       = 0x9CBDC;  // 3 uint8_t in a row.
    CONST_PTR Inventory         = 0x9CBE0;  // 320 item list, uint16_t ids.
    CONST_PTR MateriaInventory  = 0x9C360;  // 200 item list, uint32_t ids.
    CONST_PTR Gil               = 0x9D260;  // uint32_t party gil
    CONST_PTR InGameTime        = 0x9D264;  // uint32_t in seconds
    CONST_PTR GameMoment        = 0x9D288;  // uint16_t
    CONST_PTR MenuLockingMask   = 0x9D2A6;  // uint16_t bitmask of options disabled in the menu. 
    CONST_PTR PHSVisibilityMask = 0x9D78A;  // uint16_t bitmask of which characters are on PHS
    CONST_PTR WindowText        = 0xE4944;  // Array of window text entries, each window gets 256 characters, terminated by 0xFF.
    CONST_PTR WorldScreenFade   = 0x10B488; // uint8_t, 0 - 255 how much screen is faded for loading world map
};

inline uintptr_t getWindowTextOffset(uint8_t index)
{
    return GameOffsets::WindowText + (index * 256);
}

struct GameModule
{
    CONST_U8 None           = 0;
    CONST_U8 Field          = 1;
    CONST_U8 Battle         = 2;
    CONST_U8 World          = 3;
    CONST_U8 Menu           = 5;
    CONST_U8 Snowboarding   = 8;
};

struct MenuType
{
    CONST_U8 Credits        = 0x05;
    CONST_U8 NameEntry      = 0x06;
    CONST_U8 PartySelect    = 0x07;
    CONST_U8 Shop           = 0x08;
    CONST_U8 MainMenu       = 0x09;
    CONST_U8 Save           = 0x0E;
};

struct FieldOffsets
{
    CONST_PTR FieldX = 0x74EB0; // int32_t
    CONST_PTR FieldY = 0x74EB4; // int32_t
    CONST_PTR FieldZ = 0x74EB8; // int32_t
};

struct FieldScriptOffsets
{
    // List of uint16_t each is a pointer to current line of field script being executed for that group.
    CONST_PTR ExecutionTable = 0x831FC; 

    CONST_PTR TriggersStart = 0x114FF6;
    CONST_PTR ScriptStart   = 0x115000;

    CONST_PTR EncounterStart       = 0x114FE4;
    CONST_PTR EncounterTableStride = 24;

    CONST_PTR ItemID        = 0x02; // uint16_t
    CONST_PTR ItemQuantity  = 0x04; // uint8_t
    CONST_PTR MateriaID     = 0x03; // uint8_t
};

struct WorldOffsets
{
    CONST_PTR WorldX = 0xE56FC; // int32_t
    CONST_PTR WorldY = 0xE5700; // int32_t
    CONST_PTR WorldZ = 0xE5704; // int32_t

    // 16 encounter tables, 4 sets each, 14 in each of those. Each table has 2 byte header. 2048 total.
    CONST_PTR EncounterStart = 0xBD9E8;

    // Add offset from GameData to get to memory address.
    CONST_PTR ScriptStart = 0xD0B6A;

    // Subtract this from a desired memory address, then divide by two to get the value to use in jump instruction.
    CONST_PTR JumpStart = 0xD09EC;
};

// Converts a memory address into a jump address that world scripts can use
inline uint16_t getJumpAddress(uintptr_t memAddress)
{
    uintptr_t jumpStart = memAddress - WorldOffsets::JumpStart;
    return (uint16_t)(jumpStart / 2);
}

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
    CONST_U8 CharacterIDs[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

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
    CONST_PTR MaxHP             = 0x38;

    CONST_PTR WeaponMateria[] = { 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C };
    CONST_PTR ArmorMateria[]  = { 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C };
};

inline uintptr_t getCharacterDataOffset(uint8_t characterID)
{
    switch (characterID)
    {
        case 0: return CharacterDataOffsets::Cloud;
        case 1: return CharacterDataOffsets::Barret;
        case 2: return CharacterDataOffsets::Tifa;
        case 3: return CharacterDataOffsets::Aerith;
        case 4: return CharacterDataOffsets::RedXIII;
        case 5: return CharacterDataOffsets::Yuffie;
        case 6: return CharacterDataOffsets::CaitSith;
        case 7: return CharacterDataOffsets::Vincent;
        case 8: return CharacterDataOffsets::Cid;
    }

    return CharacterDataOffsets::Cloud;
}

inline std::string getCharacterName(uint8_t characterID)
{
    switch (characterID)
    {
        case 0: return "CLOUD";
        case 1: return "BARRET";
        case 2: return "TIFA";
        case 3: return "AERITH";
        case 4: return "REDXIII";
        case 5: return "YUFFIE";
        case 6: return "CAITSITH";
        case 7: return "VINCENT";
        case 8: return "CID";
    }

    return "CLOUD";
}

struct PlayerOffsets
{
    CONST_PTR Players[] = { 0x9D84C, 0x9DC8C, 0x9E0CC };

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

    CONST_PTR LimitBreakDisplay = 0x1B;     // uint8_t
    CONST_PTR EnemySkillMenu    = 0x348;    // List of 24 8-byte entries.
    CONST_PTR CharacterID       = 0x420;
};

struct BattleOffsets
{
    CONST_PTR FormationID = 0x707BC;

    // Battle Character Data Length = 104 bytes
    CONST_PTR Allies[]  = { 0xF83E0, 0xF8448, 0xF84B0 };
    CONST_PTR Enemies[] = { 0xF8580, 0xF85E8, 0xF8650, 0xF86B8, 0xF8720, 0xF8788 };

    CONST_PTR Status    = 0x00; // uint16_t
    CONST_PTR Level     = 0x09; // uint8_t
    CONST_PTR Strength  = 0x0D; // uint8_t
    CONST_PTR Magic     = 0x0E; // uint8_t
    CONST_PTR Evade     = 0x0F; // uint8_t
    CONST_PTR Speed     = 0x14; // uint8_t
    CONST_PTR Luck      = 0x15; // uint8_t
    CONST_PTR Defense   = 0x20; // uint16_t
    CONST_PTR MDefense  = 0x22; // uint16_t
    CONST_PTR CurrentMP = 0x28; // uint16_t
    CONST_PTR MaxMP     = 0x2A; // uint16_t
    CONST_PTR CurrentHP = 0x2C; // uint32_t
    CONST_PTR MaxHP     = 0x30; // uint32_t
    CONST_PTR Gil       = 0x58; // uint32_t
    CONST_PTR Exp       = 0x5C; // uint32_t

    // Graphics Data
    CONST_PTR AllyModels[] = { 0x103200, 0x112200, 0x121200 };

    CONST_PTR Inventory = 0x1671B8; // 6 bytes per inventory. uint16_t item id, uint8_t quantity, other 3 bytes unknown.
};

struct BattleStateOffsets
{
    CONST_PTR Allies[] = { 0xF5BBC, 0xF5C00, 0xF5C44 };

    CONST_PTR ATB       = 0x00;
    CONST_PTR HPDisplay = 0x38;
};

struct BattleSceneOffsets
{
    // Enemy Data Length = 184 bytes
    // This data stays static and each formation has instances made up of these enemies.
    CONST_PTR Enemies[] = { 0xF5F44, 0xF5FFC, 0xF60B4 };

    CONST_PTR Name          = 0x00; // 32 byte string
    CONST_PTR Level         = 0x20;
    CONST_PTR Speed         = 0x21;
    CONST_PTR Luck          = 0x22;
    CONST_PTR Evade         = 0x23;
    CONST_PTR Strength      = 0x24;
    CONST_PTR Defense       = 0x25;
    CONST_PTR Magic         = 0x26;
    CONST_PTR MagicDefense  = 0x27;
    CONST_PTR ElementTypes  = 0x28; // List of uint8_t with 8 entries.
    CONST_PTR ElementRates  = 0x30; // List of uint8_t with 8 entries.

    // Drop/Steals
    CONST_PTR DropRates[] = { 0x88, 0x89, 0x8A, 0x8B };
    CONST_PTR DropIDs[]   = { 0x8C, 0x8E, 0x90, 0x92 };
};

struct DropType
{
    CONST_U8 Accessory  = 0b1001;
    CONST_U8 Armor      = 0b1000;
    CONST_U8 Item       = 0b0000;
    CONST_U8 Weapon     = 0b0100;
};

struct ElementType
{
    CONST_U8 Types[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E };

    CONST_U8 Fire       = 0x00;
    CONST_U8 Ice        = 0x01;
    CONST_U8 Bolt       = 0x02;
    CONST_U8 Earth      = 0x03;
    CONST_U8 Bio        = 0x04;
    CONST_U8 Gravity    = 0x05;
    CONST_U8 Water      = 0x06;
    CONST_U8 Wind       = 0x07;
    CONST_U8 Holy       = 0x08;
    CONST_U8 Health     = 0x09;
    CONST_U8 Cut        = 0x0A;
    CONST_U8 Hit        = 0x0B;
    CONST_U8 Punch      = 0x0C;
    CONST_U8 Shoot      = 0x0D;
    CONST_U8 Scream     = 0x0E;
    CONST_U8 None       = 0xFF;
};

inline std::string getElementTypeName(uint8_t id)
{
    switch (id)
    {
        case 0:  return "Fire";
        case 1:  return "Ice";
        case 2:  return "Bolt";
        case 3:  return "Earth";
        case 4:  return "Bio";
        case 5:  return "Gravity";
        case 6:  return "Water";
        case 7:  return "Wind";
        case 8:  return "Holy";
        case 9:  return "Health";
        case 10: return "Cut";
        case 11: return "Hit";
        case 12: return "Punch";
        case 13: return "Shoot";
        case 14: return "Scream";
    }

    return "None";
}

struct ElementRate
{
    CONST_U8 Rates[] = { 0x00, 0x02, 0x04, 0x05, 0x06, 0x07 };

    CONST_U8 Death          = 0x00;
    CONST_U8 DoubleDamage   = 0x02;
    CONST_U8 HalfDamage     = 0x04;
    CONST_U8 NullifyDamage  = 0x05;
    CONST_U8 Absorb         = 0x06;
    CONST_U8 FullCure       = 0x07;
    CONST_U8 None           = 0xFF;
};

inline std::string getElementRateName(uint8_t id)
{
    switch (id)
    {
        case 0x00: return "Death";
        case 0x02: return "Double Damage";
        case 0x04: return "Half Damage";
        case 0x05: return "Nullify Damage";
        case 0x06: return "Absorb";
        case 0x07: return "Full Cure";
    }

    return "None";
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

    // uint8_t thats set to 0 for Buy, 1 for Sell, and 2 for Exit.
    CONST_PTR MenuIndex = 0x1D95F2;

    // All prices are stored in uint32_t and are just an array of them in order of:
    // Items, Weapons, Armor, Accessories.
    CONST_PTR PricesStart = 0x1D6854;

    // All materia prices are stored in uint32_t
    CONST_PTR MateriaPricesStart = 0x1D6E54;
};

struct SavemapOffsets
{
    CONST_PTR Start = 0x9C6E4;

    // These are IronMog specific values we store and fetch from an unused spot in the save map.
    // This area from 0x0B5C to 0x0B7C is 32 bytes of unused data.
    CONST_PTR IronMogSave       = Start + 0x0B5C;  // 2 Bytes for the ASCII letters IM to know we've been here.
    CONST_PTR IronMogVersion    = Start + 0x0B5E;  // A save data format version number, uint8_t
    CONST_PTR IronMogSeed       = Start + 0x0B5F;  // uint32_t seed used in current playthrough
    CONST_PTR IronMogPermadeath = Start + 0x0B63;  // uint16_t used by permadeath to track dead characters

    CONST_PTR BuggyHighwindPosition = Start + 0x0F74;
};
