#include "LiveModFF7Core/game/modules/BattleModelManager.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "LiveModFF7Core/utilities/Utilities.h"

#include <algorithm>
#include <array>

static constexpr uint32_t RamMask = 0x1FFFFF;
static constexpr uint64_t HiCloudEntry = UINT64_C(0x00018800000077B5);

struct PlayerModel
{
    uint64_t archive;           // Native disc LBA/length of the model.
    uint32_t sections;          // Section count at the start of the loaded body.
    uint32_t animationOffset;   // First animation relative to the body once relocated.
};

// By character ID.
static constexpr std::array<PlayerModel, 9> PlayerModels = {{
    {UINT64_C(0x0001080000007573), 113, 0x5B70},
    {UINT64_C(0x0001180000007594), 112, 0x65F4},
    {UINT64_C(0x00011800000075B7), 107, 0x73DC},
    {UINT64_C(0x00010800000075DA), 112, 0x656C},
    {UINT64_C(0x0000D800000075FB), 105, 0x67AC},
    {UINT64_C(0x0001200000007616), 106, 0x72B8},
    {UINT64_C(0x0000E8000000763A), 107, 0x746C},
    {UINT64_C(0x0001100000007657), 112, 0x75B8},
    {UINT64_C(0x0001100000007679), 105, 0x6A10},
}};

// Native player archive entries 0-15 and the character each belongs to. Barret's and Vincent's models
// change with their equipped weapon (BARRETT2-4, VINSENT2-3), and 9-10 are the Kalm flashback's
// Young Cloud (Cloud's model) and Sephiroth.
struct ArchiveEntry
{
    uint64_t archive;
    uint8_t owner; // Character ID.
};

static constexpr std::array<ArchiveEntry, 16> NativeArchive = {{
    {UINT64_C(0x0001080000007573), 0}, {UINT64_C(0x0001180000007594), 1}, {UINT64_C(0x00011800000075B7), 2},
    {UINT64_C(0x00010800000075DA), 3}, {UINT64_C(0x0000D800000075FB), 4}, {UINT64_C(0x0001200000007616), 5},
    {UINT64_C(0x0000E8000000763A), 6}, {UINT64_C(0x0001100000007657), 7}, {UINT64_C(0x0001100000007679), 8},
    {UINT64_C(0x0001080000007573), 9}, {UINT64_C(0x0000B0000000769B), 10}, {UINT64_C(0x00011000000076B1), 1},
    {UINT64_C(0x00011000000076D3), 1}, {UINT64_C(0x00011000000076F5), 1}, {UINT64_C(0x0001100000007717), 7},
    {UINT64_C(0x0001100000007739), 7},
}};

uintptr_t archiveEntry(uint16_t index)
{
    return BattleModelOffsets::PlayerArchive + index * 8;
}

// Retail loader code confirms the battle overlay, and with it the archive table, is resident. The
// HiCloud entry is never swapped.
bool battleLoaderPresent(GameManager* game)
{
    return game->read<uint32_t>(0xB3CEC) == 0x3C028010 && game->read<uint32_t>(0xB3D08) == 0x8C840000 &&
           game->read<uint32_t>(0xB3D0C) == 0x8C650004 && game->read<uint32_t>(0xB3D18) == 0x0C00CF9D &&
           game->read<uint64_t>(archiveEntry(16)) == HiCloudEntry;
}

void BattleModelManager::setup(GameManager* gameManager)
{
    game = gameManager;
    BIND_EVENT(game->onStart, BattleModelManager::onStart);
    BIND_EVENT(game->onExit, BattleModelManager::onExit);
    BIND_EVENT(game->onLoadUpdate, BattleModelManager::onLoadUpdate);
}

void BattleModelManager::setSwap(uint8_t characterID, uint8_t replacementCharacterID)
{
    if (characterID >= swaps.size() || replacementCharacterID >= swaps.size())
    {
        return;
    }
    uint8_t value = replacementCharacterID == characterID ? NoSwap : replacementCharacterID;
    if (swaps[characterID] == value)
    {
        return;
    }

    // Redirected entries must be restored with the swaps they were installed for.
    if (pending)
    {
        finishBattle("The model swaps changed during a battle load.", false);
    }
    swaps[characterID] = value;
    status = hasSwaps() ? "Waiting for a battle load." : "No battle model swaps.";
}

void BattleModelManager::clearSwaps()
{
    if (pending)
    {
        finishBattle("The model swaps were cleared during a battle load.", false);
    }
    swaps.fill(NoSwap);
    status = "No battle model swaps.";
}

bool BattleModelManager::hasSwaps() const
{
    return std::any_of(swaps.begin(), swaps.end(), [](uint8_t id) { return id != NoSwap; });
}

void BattleModelManager::onStart()
{
    // A load can replace all of RAM; never restore a previous attempt's edits into it. A battle
    // already in progress is left alone.
    pending = false;
    slots = {};
    lastModule = game->read<uint8_t>(GameOffsets::CurrentModule);
    if (hasSwaps())
    {
        status = "Waiting for a battle load; already loaded bodies are kept.";
    }
}

void BattleModelManager::onExit()
{
    releaseBattle();
}

void BattleModelManager::onLoadUpdate()
{
    // This runs before the manager's module event, so transitions are handled here.
    uint8_t module = game->read<uint8_t>(GameOffsets::CurrentModule);
    if (module != lastModule)
    {
        onModuleChanged(module);
    }

    if (!hasSwaps())
    {
        return;
    }

    game->requestFastUpdates();
    if (module == GameModule::Battle && pending)
    {
        updateBattle();
    }
}

void BattleModelManager::onModuleChanged(uint8_t newModule)
{
    lastModule = newModule;
    if (newModule == GameModule::Battle)
    {
        pending = hasSwaps();
        slots = {};
        if (pending)
        {
            status = "Waiting for the battle loader.";
        }
    }
    else if (pending)
    {
        finishBattle("Battle ended before the replacements were verified.", true);
    }
}

void BattleModelManager::updateBattle()
{
    if (!battleLoaderPresent(game))
    {
        return;
    }

    auto party = game->getPartyIDs();
    if (std::none_of(party.begin(), party.end(), [this](uint8_t member) { return isSwapped(member); }))
    {
        finishBattle("No swapped characters are in the party for this battle.", false);
        return;
    }

    // Each load names the archive entry and physical party slot it fills. The entry depends on the
    // member's weapon for Barret and Vincent, so the member is found through the slot.
    for (int i = 0; i < 3; ++i)
    {
        uint16_t index = game->read<uint16_t>(BattleModelOffsets::LoadSlots + i * 4);
        uint16_t physical = game->read<uint16_t>(BattleModelOffsets::LoadSlots + i * 4 + 2);
        if (physical >= 3 || !isSwapped(party[physical]) || slots[physical].character != NoSwap)
        {
            continue;
        }

        uint8_t member = party[physical];
        SlotState& slot = slots[physical];
        if (index >= NativeArchive.size() || NativeArchive[index].owner != member)
        {
            continue; // Not this battle's load yet.
        }

        // Redirect as soon as the member's load is known, before its body is read. The body pointer
        // can't tell whether that has happened: it lies beyond the overlay's file image, so until
        // battle init clears it, it still holds the previous battle's (identical) address.
        slot.character = member;
        slot.archiveIndex = index;
        if (game->read<uint64_t>(archiveEntry(index)) != NativeArchive[index].archive)
        {
            finishBattle("Battle archive entries do not match the audited assets.", true);
            return;
        }
        game->write<uint64_t>(archiveEntry(index), PlayerModels[swaps[member]].archive);
        slot.redirected = true;
        status = "Battle archive redirected; waiting for the loaded bodies.";
        LOG("Battle models: character %d -> %d in party slot %d (archive entry %d).", member, swaps[member], physical + 1, index);
    }

    bool waiting = false;
    for (int physical = 0; physical < 3; ++physical)
    {
        SlotState& slot = slots[physical];
        if (!isSwapped(party[physical]) || slot.resolved)
        {
            continue;
        }
        if (slot.character == NoSwap)
        {
            waiting = true; // Load not assigned yet.
            continue;
        }

        // The load callback assigns the body pointer, then relocates its animation pointer. Only a
        // pointer assigned after battle init cleared it belongs to this battle.
        uint32_t body = game->read<uint32_t>(BattleModelOffsets::Bodies + physical * 4);
        if (body == 0)
        {
            slot.bodyCleared = true;
        }
        if (!slot.bodyCleared)
        {
            waiting = true;
            continue;
        }
        uint32_t animations = Utilities::isRAMPointer(body, 12) ? game->read<uint32_t>((body & RamMask) + 8) : 0;
        if (!Utilities::isRAMPointer(animations, 4) || animations < body)
        {
            waiting = true;
            continue;
        }

        const PlayerModel& model = PlayerModels[swaps[slot.character]];
        slot.resolved = true;
        if (game->read<uint32_t>(body & RamMask) == model.sections && animations == body + model.animationOffset)
        {
            ++matches;
        }
        else
        {
            ++failures;
            LOG("Battle models: a different body loaded for character %d; the archive redirect was too late.", slot.character);
        }
    }

    if (!waiting)
    {
        finishBattle("Battle bodies loaded.", false);
    }
}

void BattleModelManager::releaseBattle()
{
    bool resident = game->read<uint8_t>(GameOffsets::CurrentModule) == GameModule::Battle && battleLoaderPresent(game);
    for (SlotState& slot : slots)
    {
        if (slot.redirected && resident && isSwapped(slot.character) &&
            game->read<uint64_t>(archiveEntry(slot.archiveIndex)) == PlayerModels[swaps[slot.character]].archive)
        {
            game->write<uint64_t>(archiveEntry(slot.archiveIndex), NativeArchive[slot.archiveIndex].archive);
        }
        slot.redirected = false;
    }
}

void BattleModelManager::finishBattle(const char* newStatus, bool failed)
{
    releaseBattle();
    pending = false;
    slots = {};
    status = newStatus;
    if (failed)
    {
        ++failures;
    }
    LOG("Battle models: %s", newStatus);
}
