#include "LiveModFF7Core/game/modules/BattleLimitManager.h"
#include "LiveModFF7Core/game/GameData.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"

#include <algorithm>
#include <cstring>

using LimitList = std::array<uint8_t, 12>;
static constexpr int LimitCount = 71;
static constexpr uint8_t NoLimit = 0x7F;     // Player data's empty limit ID.
static constexpr uint8_t NoListLimit = 0xFF; // Kernel list's empty attack ID, which reads as NoLimit.

// Character AI offsets by character ID (Young Cloud, Sephiroth and a spare after the nine), identical
// on US and CSR. Event 1 of a block is the main script, which runs a character's turns when the game
// controls them; only Vincent's (for his transformed forms) exists among the nine.
static constexpr std::array<uint16_t, 12> NativeCharacterAI = {24, 446, 482, 518, 0xFFFF, 1086, 0xFFFF, 1122, 0xFFFF, 0xFFFF, 1202, 0xFFFF};

// Battle overlay instructions that pick Tifa's reels and Cait Sith's slots by character ID, and read
// their own records. Each is patched in its immediate only.
struct CodePatch
{
    uintptr_t address;
    uint32_t original;
};

static constexpr CodePatch TifaMenuCheck = {0xDF188, 0x34040002};   // ori a0,zero,2: reels for character 2
static constexpr CodePatch CaitMenuCheck = {0xDF190, 0x34020006};   // ori v0,zero,6: slots for character 6
static constexpr CodePatch CaitLevelRead = {0xDF1BC, 0x9042CA5E};   // lbu v0,Cait+0x0E: slots only at level 2
static constexpr CodePatch CaitLevelCompare = {0xDF1C0, 0x00000000}; // nop before comparing that level with a0 (2)
static constexpr CodePatch TifaLevelRead = {0xE53EC, 0x9063C84E};   // lbu v1,Tifa+0x0E: reel setup
static constexpr CodePatch TifaLearnedRead = {0xE5480, 0x24A5C862}; // addiu a1,Tifa+0x22: reel setup
static constexpr CodePatch LimitMenuPatches[] = {TifaMenuCheck, CaitMenuCheck, CaitLevelRead, CaitLevelCompare, TifaLevelRead, TifaLearnedRead};

// Replaces the low 16 bits (an immediate relative to 0x800A0000) with a field of a character's record.
uint32_t recordImmediate(uint32_t instruction, uint8_t characterID, uint32_t field)
{
    uint32_t address = 0x80000000 | uint32_t(CharacterDataOffsets::Characters[characterID] + field);
    return (instruction & 0xFFFF0000) | ((address - 0x800A0000) & 0xFFFF);
}

uintptr_t limitListAddress(uint8_t characterID)
{
    return KernelOffsets::CharacterLimits + characterID * KernelOffsets::CharacterBlockStride;
}

void BattleLimitManager::setup(GameManager* gameManager)
{
    game = gameManager;
    BIND_EVENT(game->onExit, BattleLimitManager::onExit);
    BIND_EVENT_ONE_ARG(game->onUpdate, BattleLimitManager::onUpdate);
    BIND_EVENT(game->onBattleEnter, BattleLimitManager::onBattleEnter);
}

void BattleLimitManager::setLimitSet(uint8_t characterID, uint8_t sourceCharacterID)
{
    if (characterID < sets.size() && sourceCharacterID < sets.size())
    {
        sets[characterID] = characterID == sourceCharacterID ? NoSwap : sourceCharacterID;
        status = "Limit sets applied; party limits update before the next battle.";
    }
}

void BattleLimitManager::clearLimitSets()
{
    sets.fill(NoSwap);
    restore();
    status = "No limit break swaps.";
}

uint8_t BattleLimitManager::characterUsing(uint8_t sourceCharacterID) const
{
    for (uint8_t id = 0; id < sets.size(); ++id)
    {
        if (limitSource(id) == sourceCharacterID)
        {
            return id;
        }
    }
    return NoSwap;
}

bool BattleLimitManager::hasLimitData() const
{
    return GameData::limitBreaks.size() == LimitCount && GameData::characterLimits[0][0] != 0;
}

void BattleLimitManager::restore()
{
    updateLimitLists(true);
    updatePartyLimits(true);
    updateCharacterAI(true);
    patchLimitMenus(true);
}

void BattleLimitManager::onExit()
{
    restore();
}

void BattleLimitManager::onUpdate(bool justConnected)
{
    updateLimitLists(false);
    updateCharacterAI(false);

    // During battle the game owns the player data it has already validated.
    if (game->read<uint8_t>(GameOffsets::CurrentModule) != GameModule::Battle)
    {
        updatePartyLimits(false);
    }
}

// Each character's list becomes their source's. The game never writes these lists, so only a table
// that holds native lists (in any arrangement) is touched.
void BattleLimitManager::updateLimitLists(bool restore)
{
    if (!hasLimitData())
    {
        return;
    }

    std::array<LimitList, 9> current;
    for (uint8_t id = 0; id < current.size(); ++id)
    {
        if (!game->read(limitListAddress(id), sizeof(LimitList), current[id].data()) ||
            std::find(GameData::characterLimits.begin(), GameData::characterLimits.end(), current[id]) == GameData::characterLimits.end())
        {
            return;
        }
    }

    for (uint8_t id = 0; id < current.size(); ++id)
    {
        const LimitList& wanted = GameData::characterLimits[restore ? id : limitSource(id)];
        if (current[id] != wanted)
        {
            game->write(limitListAddress(id), (uint8_t*)wanted.data(), sizeof(LimitList));
        }
    }
}

// Party members' player data holds their current level's limits: IDs at LimitIDs and attack records at
// LimitData, which battle init checks against the lists. Keep them matching the (remapped) lists. Without
// swaps a character's own list reproduces what the game builds.
void BattleLimitManager::updatePartyLimits(bool restore)
{
    if (!hasLimitData())
    {
        return;
    }

    auto party = game->getPartyIDs();
    for (int slot = 0; slot < 3; ++slot)
    {
        uint8_t id = party[slot];
        if (id >= sets.size() || (!restore && sets[id] == NoSwap))
        {
            continue;
        }
        uint8_t level = game->read<uint8_t>(getCharacterDataOffset(id) + CharacterDataOffsets::CurrentLimitLevel);
        if (level < 1 || level > 4)
        {
            continue;
        }

        const LimitList& list = GameData::characterLimits[restore ? id : limitSource(id)];
        std::array<uint8_t, 3> ids;
        std::array<AttackData, 3> records;
        for (int position = 0; position < 3; ++position)
        {
            uint8_t attack = list[(level - 1) * 3 + position];
            ids[position] = attack == NoListLimit ? NoLimit : uint8_t(attack - 0x80);
            std::fill((uint8_t*)&records[position], (uint8_t*)&records[position] + sizeof(AttackData), 0xFF);
            if (ids[position] < LimitCount)
            {
                records[position] = GameData::limitBreaks[ids[position]].attackData;
            }
        }

        uintptr_t player = PlayerOffsets::Players[slot];
        std::array<uint8_t, 3> currentIDs;
        std::array<AttackData, 3> currentRecords;
        if (!game->read(player + PlayerOffsets::LimitIDs, sizeof(currentIDs), currentIDs.data()) ||
            !game->read(player + PlayerOffsets::LimitData, sizeof(currentRecords), (uint8_t*)currentRecords.data()))
        {
            continue;
        }
        if (currentIDs != ids || std::memcmp(currentRecords.data(), records.data(), sizeof(records)) != 0)
        {
            game->write(player + PlayerOffsets::LimitIDs, ids.data(), sizeof(ids));
            game->write(player + PlayerOffsets::LimitData, (uint8_t*)records.data(), sizeof(records));
            LOG("Limit swaps: party slot %d uses %s's level %d limits.", slot + 1, getCharacterName(restore ? id : limitSource(id)).c_str(), level);
        }
    }
}

// A character whose limit source has a main AI script runs the source's AI, so the source's
// game-controlled turns (Vincent's transformed forms) work. Other characters keep their own AI.
void BattleLimitManager::updateCharacterAI(bool restore)
{
    std::array<uint16_t, 12> current;
    if (!hasLimitData() || !game->read(KernelOffsets::CharacterAI, sizeof(current), (uint8_t*)current.data()) ||
        !std::equal(current.begin() + 9, current.end(), NativeCharacterAI.begin() + 9))
    {
        return;
    }

    std::array<uint16_t, 12> wanted = NativeCharacterAI;
    for (uint8_t id = 0; id < sets.size() && !restore; ++id)
    {
        uint16_t source = NativeCharacterAI[limitSource(id)];
        if (source != 0xFFFF && game->read<uint16_t>(KernelOffsets::CharacterAI + source + 2) != 0xFFFF)
        {
            wanted[id] = source;
        }
    }
    if (current != wanted)
    {
        game->write(KernelOffsets::CharacterAI, (uint8_t*)wanted.data(), sizeof(wanted));
    }
}

// The battle overlay is loaded for each battle and these instructions only run once a limit menu opens,
// so patching at battle start lands before their first use.
void BattleLimitManager::patchLimitMenus(bool restore)
{
    if (game->read<uint8_t>(GameOffsets::CurrentModule) != GameModule::Battle)
    {
        return;
    }

    uint8_t tifa = restore ? CharacterID::Tifa : characterUsing(CharacterID::Tifa);
    uint8_t cait = restore ? CharacterID::CaitSith : characterUsing(CharacterID::CaitSith);
    std::array<uint32_t, 6> wanted = {
        (TifaMenuCheck.original & 0xFFFF0000) | tifa,
        (CaitMenuCheck.original & 0xFFFF0000) | cait,
        cait < 9 ? recordImmediate(CaitLevelRead.original, cait, CharacterDataOffsets::CurrentLimitLevel) : CaitLevelRead.original,
        0x34040002, // ori a0,zero,2: the level compare no longer relies on a0 still holding Tifa's ID.
        tifa < 9 ? recordImmediate(TifaLevelRead.original, tifa, CharacterDataOffsets::CurrentLimitLevel) : TifaLevelRead.original,
        tifa < 9 ? recordImmediate(TifaLearnedRead.original, tifa, CharacterDataOffsets::LearnedLimits) : TifaLearnedRead.original,
    };
    if (restore)
    {
        wanted[3] = CaitLevelCompare.original;
    }

    // Only touch the overlay if every site holds either its original instruction or a version of our patch.
    for (size_t i = 0; i < wanted.size(); ++i)
    {
        uint32_t current = game->read<uint32_t>(LimitMenuPatches[i].address);
        bool known = i == 3 ? current == CaitLevelCompare.original || current == 0x34040002
                            : (current & 0xFFFF0000) == (LimitMenuPatches[i].original & 0xFFFF0000);
        if (!known)
        {
            LOG("Limit swaps: unexpected battle code at 0x%X; limit menus are left unchanged.", 0x80000000 | LimitMenuPatches[i].address);
            return;
        }
    }
    for (size_t i = 0; i < wanted.size(); ++i)
    {
        game->write<uint32_t>(LimitMenuPatches[i].address, wanted[i]);
    }
}

void BattleLimitManager::onBattleEnter()
{
    if (hasLimitData())
    {
        patchLimitMenus(false);
        status = "Limit breaks swapped for this battle.";
    }
}
