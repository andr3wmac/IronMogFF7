#include "LiveModFF7Core/game/modules/EquipmentManager.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"

#include <algorithm>
#include <cstring>
#include <vector>

static constexpr int WeaponCount = 128;
static constexpr int ArmorCount = 32;
static constexpr uint8_t NoOwner = 0xFF;
static constexpr uint16_t CharacterBits = 0x1FF; // The nine main characters. Young Cloud (9) and Sephiroth (10) sit above.

// Weapons are grouped by owner, identical on US and CSR. Cloud's also carry Young Cloud's bit and the Masamune (127) only Sephiroth's.
struct WeaponRange
{
    uint8_t first;
    uint8_t owner;
};

static constexpr WeaponRange WeaponRanges[] = {
    {0, CharacterID::Cloud},     {16, CharacterID::Tifa},   {32, CharacterID::Barret},
    {48, CharacterID::RedXIII},  {62, CharacterID::Aerith}, {73, CharacterID::Cid},
    {87, CharacterID::Yuffie},   {101, CharacterID::CaitSith}, {114, CharacterID::Vincent},
    {127, NoOwner},
};

// Each character's weapon at New Game (Cait Sith's and Vincent's are set when they join).
static constexpr std::array<uint8_t, 9> StartingWeapons = {0, 32, 16, 62, 48, 87, 101, 114, 73};

// Armor is equippable by all nine except the Minerva Band (Tifa, Aerith, Yuffie) and the Escort Guard (the rest and Young Cloud).
static constexpr uint16_t MinervaBand = 26;
static constexpr uint16_t EscortGuard = 27;

// The leading bytes of the Buster Sword and Bronze Bangle records, which nothing changes, confirm the tables are resident.
static constexpr uint8_t WeaponSignature[] = {0x23, 0xFF, 0x11, 0xFF, 0x12, 0xFF, 0x01, 0x00, 0x60, 0x00, 0xFF, 0xF8, 0xFF, 0xFF};
static constexpr uint8_t ArmorSignature[] = {0x00, 0xFF, 0x08, 0x00, 0x00, 0x00, 0xFF, 0xFF};

static constexpr uint32_t NoMateria = 0xFFFFFFFF;
static constexpr uint16_t NoItem = 0xFFFF;

uint8_t nativeWeaponOwner(uint8_t weapon)
{
    uint8_t owner = NoOwner;
    for (const WeaponRange& range : WeaponRanges)
    {
        if (weapon >= range.first)
        {
            owner = range.owner;
        }
    }
    return owner;
}

uint16_t nativeWeaponMask(uint8_t weapon)
{
    uint8_t owner = nativeWeaponOwner(weapon);
    if (owner == NoOwner)
    {
        return 0x400;
    }
    return (1 << owner) | (owner == CharacterID::Cloud ? 0x200 : 0);
}

uint16_t nativeArmorMask(uint16_t armor)
{
    switch (armor)
    {
        case MinervaBand: return 0x02C;
        case EscortGuard: return 0x3D3;
    }
    return CharacterBits;
}

uint16_t readU16(const std::vector<uint8_t>& data, size_t offset)
{
    return data[offset] | (data[offset + 1] << 8);
}

bool isStartingWeapon(uint8_t weapon)
{
    return std::find(StartingWeapons.begin(), StartingWeapons.end(), weapon) != StartingWeapons.end();
}

void EquipmentManager::setup(GameManager* gameManager)
{
    game = gameManager;
    BIND_EVENT(game->onExit, EquipmentManager::onExit);
    BIND_EVENT_ONE_ARG(game->onFrame, EquipmentManager::onFrame);
}

void EquipmentManager::setEquipSource(uint8_t characterID, uint8_t sourceCharacterID)
{
    if (characterID < sources.size() && sourceCharacterID < sources.size())
    {
        sources[characterID] = characterID == sourceCharacterID ? NoSwap : sourceCharacterID;
        status = "Equipment swaps registered.";
    }
}

void EquipmentManager::clearEquipSources()
{
    sources.fill(NoSwap);
    updateMasks(true);
    status = "No equipment swaps.";
}

uint8_t EquipmentManager::getStartingWeapon(uint8_t characterID) const
{
    return characterID < sources.size() ? StartingWeapons[equipSource(characterID)] : StartingWeapons[CharacterID::Cloud];
}

bool EquipmentManager::hasSwaps() const
{
    return std::any_of(sources.begin(), sources.end(), [](uint8_t source) { return source != NoSwap; });
}

void EquipmentManager::onExit()
{
    updateMasks(true);
}

void EquipmentManager::onFrame(int frameNumber)
{
    if (!hasSwaps() || !updateMasks(false))
    {
        return;
    }

    // Character records are only rewritten where nothing is reading them: not in battle (which copies them at its start) 
    // and not in a menu (the equip menu works on them).
    uint8_t module = game->getGameModule();
    if ((module == GameModule::Field || module == GameModule::World) && !game->inMenu())
    {
        updateStartingWeapons();
    }
}

// The game never writes the equip masks, so only resident tables whose other bits are native are touched.
// Returns whether the masks now hold the wanted values.
bool EquipmentManager::updateMasks(bool restore)
{
    std::vector<uint8_t> weapons(WeaponCount * KernelOffsets::WeaponDataStride);
    std::vector<uint8_t> armor(ArmorCount * KernelOffsets::ArmorDataStride);
    if (!game->read(KernelOffsets::WeaponDataStart, weapons.size(), weapons.data()) ||
        !game->read(KernelOffsets::ArmorDataStart, armor.size(), armor.data()) ||
        std::memcmp(weapons.data(), WeaponSignature, sizeof(WeaponSignature)) != 0 ||
        std::memcmp(armor.data(), ArmorSignature, sizeof(ArmorSignature)) != 0)
    {
        status = "Waiting for the kernel equipment tables.";
        return false;
    }

    // A character's bit is set wherever their source's native bit is.
    auto remap = [this, restore](uint16_t native) {
        uint16_t mask = native & ~CharacterBits;
        for (uint8_t id = 0; id < sources.size(); ++id)
        {
            if (native & (1 << (restore ? id : equipSource(id))))
            {
                mask |= 1 << id;
            }
        }
        return mask;
    };

    // The Masamune is left to whoever else sets it (Gameplay Mods).
    for (uint8_t weapon = 0; weapon < WeaponCount - 1; ++weapon)
    {
        size_t offset = weapon * KernelOffsets::WeaponDataStride + KernelOffsets::WeaponEquipMask;
        if ((readU16(weapons, offset) & ~CharacterBits) != (nativeWeaponMask(weapon) & ~CharacterBits))
        {
            status = "Unexpected weapon data; equip masks were left unchanged.";
            return false;
        }
    }
    for (uint16_t piece = 0; piece < ArmorCount; ++piece)
    {
        size_t offset = piece * KernelOffsets::ArmorDataStride + KernelOffsets::ArmorEquipMask;
        if ((readU16(armor, offset) & ~CharacterBits) != (nativeArmorMask(piece) & ~CharacterBits))
        {
            status = "Unexpected armor data; equip masks were left unchanged.";
            return false;
        }
    }

    for (uint8_t weapon = 0; weapon < WeaponCount - 1; ++weapon)
    {
        size_t offset = weapon * KernelOffsets::WeaponDataStride + KernelOffsets::WeaponEquipMask;
        uint16_t wanted = remap(nativeWeaponMask(weapon));
        if (readU16(weapons, offset) != wanted)
        {
            game->write<uint16_t>(KernelOffsets::WeaponDataStart + offset, wanted);
        }
    }
    for (uint16_t piece = 0; piece < ArmorCount; ++piece)
    {
        size_t offset = piece * KernelOffsets::ArmorDataStride + KernelOffsets::ArmorEquipMask;
        uint16_t wanted = remap(nativeArmorMask(piece));
        if (readU16(armor, offset) != wanted)
        {
            game->write<uint16_t>(KernelOffsets::ArmorDataStart + offset, wanted);
        }
    }

    if (!restore)
    {
        status = "Equip masks swapped.";
    }
    return true;
}

// A character holding a weapon they can't equip gets their source's starting weapon. 
// Records still holding Young Cloud or Sephiroth (Cait Sith's and Vincent's before they join) are skipped.
void EquipmentManager::updateStartingWeapons()
{
    for (uint8_t id = 0; id < sources.size(); ++id)
    {
        uintptr_t record = getCharacterDataOffset(id);
        if (game->read<uint8_t>(record + CharacterDataOffsets::ID) != id)
        {
            continue;
        }

        uint8_t weapon = game->read<uint8_t>(record + CharacterDataOffsets::EquippedWeapon);
        uint8_t owner = nativeWeaponOwner(weapon);
        if (weapon < WeaponCount && owner != NoOwner && owner != equipSource(id))
        {
            replaceWeapon(id, weapon);
        }
    }
}

void EquipmentManager::replaceWeapon(uint8_t characterID, uint8_t weapon)
{
    uintptr_t record = getCharacterDataOffset(characterID);
    uint8_t replacement = getStartingWeapon(characterID);

    uint8_t slots[8];
    std::array<uint32_t, 8> materia;
    if (!game->read(KernelOffsets::WeaponDataStart + replacement * KernelOffsets::WeaponDataStride + KernelOffsets::WeaponMateriaSlots, sizeof(slots), slots) ||
        !game->read(record + CharacterDataOffsets::WeaponMateria[0], sizeof(materia), (uint8_t*)materia.data()))
    {
        return;
    }

    // Materia keeps its order in the new weapon's slots. Whatever doesn't fit goes to the materia list.
    std::array<uint32_t, 8> placed;
    placed.fill(NoMateria);
    size_t next = 0;
    for (uint32_t orb : materia)
    {
        if (orb == NoMateria)
        {
            continue;
        }
        while (next < placed.size() && (slots[next] == 0 || slots[next] == 4))
        {
            ++next;
        }
        if (next < placed.size())
        {
            placed[next++] = orb;
            continue;
        }

        bool stored = false;
        for (int i = 0; i < 200 && !stored; ++i)
        {
            uintptr_t entry = GameOffsets::MateriaInventory + i * 4;
            if (game->read<uint32_t>(entry) == NoMateria)
            {
                game->write<uint32_t>(entry, orb);
                stored = true;
            }
        }
        if (!stored)
        {
            LOG("Equipment swaps: the materia list is full; materia %d was lost.", orb & 0xFF);
        }
    }

    game->write(record + CharacterDataOffsets::WeaponMateria[0], (uint8_t*)placed.data(), sizeof(placed));
    game->write<uint8_t>(record + CharacterDataOffsets::EquippedWeapon, replacement);

    // Starting weapons are interchangeable and anything better is kept in the inventory.
    if (!isStartingWeapon(weapon))
    {
        uint16_t item = 128 + weapon;
        int freeSlot = -1;
        bool stacked = false;
        for (int i = 0; i < 320 && !stacked; ++i)
        {
            uintptr_t entry = GameOffsets::Inventory + i * 2;
            uint16_t data = game->read<uint16_t>(entry);
            if (data == NoItem)
            {
                freeSlot = freeSlot < 0 ? i : freeSlot;
            }
            else if ((data & 0x1FF) == item && (data >> 9) < 99)
            {
                game->write<uint16_t>(entry, data + (1 << 9));
                stacked = true;
            }
        }
        if (!stacked && freeSlot >= 0)
        {
            game->write<uint16_t>(GameOffsets::Inventory + freeSlot * 2, (1 << 9) | item);
        }
        else if (!stacked)
        {
            LOG("Equipment swaps: the inventory is full; weapon %d was lost.", weapon);
        }
    }

    LOG("Equipment swaps: %s's weapon %d became starting weapon %d.", getCharacterName(characterID).c_str(), weapon, replacement);
}
