#include "LiveModFF7Core/game/modules/FieldModelManager.h"
#include "LiveModFF7Core/game/FieldModelFormats.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "LiveModFF7Core/utilities/Utilities.h"

#include <algorithm>
#include <array>

static constexpr uint32_t RamMask = 0x1FFFFF;
static constexpr uint32_t BsxStagingStart = 0x80000000 | FieldModelOffsets::BsxStaging;

struct GlobalModel
{
    uint64_t archive; // Native disc LBA/length of the BCX.
    uint8_t bones, parts;
};

// By global model ID - 1: Cloud, Aerith, Barret, Tifa, Red XIII, Cid, Yuffie, Cait Sith, Vincent.
// A global model's face ID is its global model ID - 1.
static constexpr std::array<GlobalModel, 9> GlobalModels = {{
    {UINT64_C(0x00001BA00001EF79), 22, 15},
    {UINT64_C(0x00001B8B0001EF7D), 24, 16},
    {UINT64_C(0x00001F250001EF81), 22, 15},
    {UINT64_C(0x000020330001EF85), 25, 17},
    {UINT64_C(0x000021370001EF8A), 30, 22},
    {UINT64_C(0x00001CCC0001EF8F), 22, 15},
    {UINT64_C(0x00001DBB0001EF93), 25, 16},
    {UINT64_C(0x00001CC60001EF97), 29, 16},
    {UINT64_C(0x00001B8A0001EF9B), 26, 17},
}};

// The archive entry after the nine models is FIELD.TDB (every character's eyes and mouths), which a
// swap never changes, so it identifies the resident table.
static constexpr uint64_t FaceTexturesEntry = UINT64_C(0x0000353F0001EF9F);

using ArchiveTable = std::array<uint64_t, 9>;

const GlobalModel& globalModel(uint8_t globalID)
{
    return GlobalModels[globalID - 1];
}

uint8_t faceID(uint8_t globalID)
{
    return globalID - 1;
}

// Cloud, Barret and Cid have identical bone hierarchies and bone-to-part assignments, so scene clips
// authored for one play correctly on another.
bool sharesSkeleton(uint8_t globalA, uint8_t globalB)
{
    auto humanoid = [](uint8_t id) { return id == 1 || id == 3 || id == 6; };
    return globalA == globalB || (humanoid(globalA) && humanoid(globalB));
}

const GlobalModelFootprint& footprint(uint8_t globalID)
{
    return GameData::globalModelFootprints[globalID - 1];
}

struct RuntimeModelTable
{
    uint32_t address = 0;
    uint8_t count = 0;
    std::array<FieldRuntimeModel, 64> models;
};

uint32_t fnv1a(const uint8_t* data, uint32_t size)
{
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < size; ++i)
    {
        hash = (hash ^ data[i]) * 16777619u;
    }
    return hash;
}

// The DAT model section the field loader is working from.
bool readDat(GameManager* game, FieldDatSection& dat)
{
    uint32_t section = game->read<uint32_t>(FieldModelOffsets::DatModelSection);
    if (!Utilities::isRAMPointer(section, 4))
    {
        return false;
    }

    uint16_t count = game->read<uint16_t>((section & RamMask) + 2);
    uint32_t size = 4 + count * 8;
    return count <= 64 && Utilities::isRAMPointer(section, size) && game->read(section & RamMask, size, (uint8_t*)&dat) && dat.size == size;
}

bool readRuntimeModels(GameManager* game, RuntimeModelTable& table)
{
    table.count = game->read<uint8_t>(FieldModelOffsets::RuntimeModelCount);
    table.address = game->read<uint32_t>(FieldModelOffsets::RuntimeModels);

    return table.count != 0 && table.count <= 64 && 
        Utilities::isRAMPointer(table.address, table.count * 36) && 
        game->read(table.address & RamMask, table.count * 36, (uint8_t*)table.models.data());
}

bool readArchive(GameManager* game, ArchiveTable& table)
{
    return game->read(FieldModelOffsets::GlobalArchive, sizeof(table), (uint8_t*)table.data());
}

uint32_t metadataBytes(const FieldRuntimeModel& model)
{
    return model.bones * 4 + model.parts * 32 + model.clips * 16;
}

// The allocator places each record's bone/part/animation metadata back to back after the records,
// in slot order. Returns the end of that layout, or 0 if the table is not the allocator's output.
uint32_t allocatorLayoutEnd(const RuntimeModelTable& table)
{
    uint32_t cursor = table.address + table.count * 36;
    for (uint8_t i = 0; i < table.count; ++i)
    {
        if (table.models[i].skeleton != cursor)
        {
            return 0;
        }
        cursor += metadataBytes(table.models[i]);
    }

    return Utilities::isRAMPointer(cursor, 0) ? cursor : 0;
}

bool bsxHeaderAt(GameManager* game, uintptr_t address, const FieldModelLighting& layout)
{
    return game->read<uint32_t>(address) == layout.bsxBytes && game->read<uint32_t>(address + 4) == layout.modelHeaderOffset;
}

// The destination's BSX is fully in the staging buffer and has not been edited.
bool stagedBsxMatches(GameManager* game, const FieldModelLighting& layout)
{
    uint32_t size = 16 + layout.modelCount * 48;
    std::array<uint8_t, 16 + 64 * 48> directory;
    return layout.modelCount <= 64 && bsxHeaderAt(game, FieldModelOffsets::BsxStaging, layout) &&
           game->read(FieldModelOffsets::BsxStaging + layout.modelHeaderOffset, size, directory.data()) &&
           fnv1a(directory.data(), size) == layout.directoryHash;
}

// Native BSX initialization has copied the destination's BSX to its retained location.
bool retainedBsxMatches(GameManager* game, const FieldModelLighting& layout)
{
    uint32_t base = game->read<uint32_t>(FieldModelOffsets::RetainedBsx);
    return Utilities::isRAMPointer(base, 8) && bsxHeaderAt(game, base & RamMask, layout);
}

// Native BSX initialization ends by storing the lighting table as the free pointer. During a load the
// free pointer is the allocation end instead.
bool modelsInitialized(GameManager* game)
{
    return game->read<uint32_t>(FieldModelOffsets::FreePointer) == game->read<uint32_t>(FieldModelOffsets::LightingTable);
}

// Retail loader code confirms the field overlay, and with it the archive table, is resident.
bool fieldLoaderPresent(GameManager* game)
{
    return game->read<uint32_t>(0xADAA4) == 0x27BDFFF0 && game->read<uint32_t>(0xADDBC) == 0x80910007 &&
           game->read<uint32_t>(0xADE00) == 0x8C640000 && game->read<uint32_t>(0xADE8C) == 0x0C00CF9D &&
           game->read<uint64_t>(FieldModelOffsets::GlobalArchive + 9 * 8) == FaceTexturesEntry;
}

void FieldModelManager::setup(GameManager* gameManager)
{
    game = gameManager;
    BIND_EVENT(game->onStart, FieldModelManager::onStart);
    BIND_EVENT(game->onExit, FieldModelManager::onExit);
    BIND_EVENT(game->onLoadUpdate, FieldModelManager::onLoadUpdate);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, FieldModelManager::onFieldChanged);
}

void FieldModelManager::setSwap(uint8_t globalID, uint8_t replacementGlobalID)
{
    if (globalID < 1 || globalID > 9 || replacementGlobalID < 1 || replacementGlobalID > 9)
    {
        return;
    }
    uint8_t value = replacementGlobalID == globalID ? 0 : replacementGlobalID;
    if (swaps[globalID - 1] == value)
    {
        return;
    }

    // The archive must be restored with the swaps it was installed for.
    if (phase != Phase::Idle)
    {
        finishField("The model swaps changed during a field load.", false);
    }
    releaseField();
    swaps[globalID - 1] = value;
    status = hasSwaps() ? "Waiting for a field load." : "No field model swaps.";
}

void FieldModelManager::clearSwaps()
{
    if (phase != Phase::Idle)
    {
        finishField("The model swaps were cleared during a field load.", false);
    }
    releaseField();
    swaps.fill(0);
    status = "No field model swaps.";
}

bool FieldModelManager::hasSwaps() const
{
    return std::any_of(swaps.begin(), swaps.end(), [](uint8_t id) { return id != 0; });
}

void FieldModelManager::onStart()
{
    // A load can replace all of RAM; never restore a previous attempt's edits into it.
    phase = Phase::Idle;
    fieldID = 0;
    fieldObserved = false;
    archiveInstalled = false;
    modelsWereInitialized = false;
    modelsLoading = true; // Report models already loaded when connecting mid-field.
    lastModule = game->read<uint8_t>(GameOffsets::CurrentModule);

    if (hasSwaps())
    {
        status = "Waiting for a field load; already loaded bodies are kept.";
    }
}

void FieldModelManager::onExit()
{
    releaseField();
}

void FieldModelManager::onLoadUpdate()
{
    // This runs before the manager's module event, so transitions are handled here.
    uint8_t module = game->read<uint8_t>(GameOffsets::CurrentModule);
    if (module != lastModule)
    {
        onModuleChanged(module);
    }

    if (hasSwaps())
    {
        // Loader windows are short and frames stall while loading, so keep polling quickly.
        game->requestFastUpdates();
    }

    if (module != GameModule::Field)
    {
        return;
    }

    // Every field load, including a same-field reload, moves the free pointer off the lighting table
    // when the allocator runs, until BSX initialization finishes. The BSX and BCX disc reads in between
    // span many polls, and the allocation is exactly what a swap resizes.
    bool initialized = fieldLoaderPresent(game) && modelsInitialized(game);
    bool loadStarted = modelsWereInitialized && !initialized;
    modelsWereInitialized = initialized;

    if (hasSwaps())
    {
        uint16_t currentField = game->read<uint16_t>(GameOffsets::FieldID);
        if (currentField != fieldID || (loadStarted && phase == Phase::Idle))
        {
            beginField(currentField);
        }
        if (phase != Phase::Idle)
        {
            updateField();
        }
    }

    // After the swap steps, so consumers see the final models.
    if (!initialized)
    {
        modelsLoading = true;
        game->requestFastUpdates();
    }
    else if (modelsLoading)
    {
        notifyModelsReady();
    }
}

void FieldModelManager::notifyModelsReady()
{
    modelsLoading = false;
    game->onFieldModelsReady.invoke();
}

void FieldModelManager::onModuleChanged(uint8_t newModule)
{
    uint8_t previousModule = lastModule;
    lastModule = newModule;
    modelsWereInitialized = false;
    if (newModule == GameModule::Field)
    {
        // Returning from the menu keeps the loaded field; anything else loads it.
        if (previousModule != GameModule::Menu)
        {
            modelsLoading = true;
        }
        return;
    }

    if (phase != Phase::Idle)
    {
        finishField("Field loading was interrupted by a module change.", true);
    }

    // Returning from the menu keeps the loaded field; anything else reloads it.
    if (newModule != GameModule::Menu)
    {
        fieldID = 0;
    }
}

void FieldModelManager::onFieldChanged(uint16_t newFieldID)
{
    // The field has finished loading; no attempt may outlive it with edits in place.
    if (phase != Phase::Idle && newFieldID == fieldID)
    {
        updateField();
        if (phase != Phase::Idle)
        {
            finishField("Field finished loading before the replacements were verified.", fieldExpected);
        }
    }

    // The field is shown, so its models are loaded even if initialization was never seen finishing.
    if (modelsLoading)
    {
        LOG("Field models: field %d was shown before its models were seen finishing initialization.", newFieldID);
        notifyModelsReady();
    }
}

void FieldModelManager::beginField(uint16_t newFieldID)
{
    if (phase != Phase::Idle)
    {
        finishField("Another field load interrupted the replacements.", true);
    }

    fieldExpected = fieldObserved;
    fieldObserved = true;
    fieldID = newFieldID;
    aliasedClips = 0;

    lighting = GameData::getField(fieldID).modelLighting;
    if (lighting.bsxBytes == 0 || footprint(1).drawBytes == 0)
    {
        finishField("No party model layout for this field in the game data.", false);
        return;
    }

    phase = Phase::AwaitAllocation;
    status = "Waiting for the native allocator.";
}

void FieldModelManager::updateField()
{
    if (!fieldLoaderPresent(game))
    {
        return;
    }

    // Every entry is native until the swaps are installed together, then every swapped entry is redirected.
    ArchiveTable archive;
    if (!readArchive(game, archive))
    {
        return;
    }

    for (uint8_t id = 1; id <= 9; ++id)
    {
        uint64_t expected = globalModel(archiveInstalled ? replacementFor(id) : id).archive;
        if (archive[id - 1] != expected)
        {
            finishField("Field archive entries do not match the audited assets.", true);
            return;
        }
    }

    // Each step either advances the phase, finishes the attempt, or waits for the loader.
    if (phase == Phase::AwaitAllocation)
    {
        resizeAllocation();
    }

    if (phase == Phase::AwaitBsx)
    {
        prepareLightingReserve();
    }

    if (phase == Phase::AwaitInit)
    {
        finishFieldLoad();
    }
}

// The allocator sizes each record from the DAT's counts, but the BCX copy uses the asset's own.
// The allocator runs as soon as the DAT is parsed, with no disc wait to catch it in. Instead, while
// the BSX read that follows is pending, lay the records out again exactly as the allocator would
// have with each replacement's counts, then redirect the swapped archive entries ahead of the BCX
// reads. The loader re-reads the free pointer after the BSX read, and nothing after the allocator
// reads the DAT counts.
void FieldModelManager::resizeAllocation()
{
    // The loader stores the allocator's end to both pointers just before starting the BSX read. 
    // With the records laid out back to back, this state only exists for a fresh allocation.
    RuntimeModelTable models;
    uint32_t end = readRuntimeModels(game, models) ? allocatorLayoutEnd(models) : 0;
    if (end == 0 || game->read<uint32_t>(FieldModelOffsets::FreePointer) != end ||
        game->read<uint32_t>(FieldModelOffsets::RetainedBsx) != end)
    {
        return;
    }

    // Once the BSX has landed the BCX reads can start at any moment.
    FieldDatSection dat;
    if (!readDat(game, dat))
    {
        finishField("The DAT model section could not be read; the original bodies were kept.", true);
        return;
    }

    std::array<uint32_t, 9> datRecords{};
    bool loaded = stagedBsxMatches(game, lighting);
    for (uint16_t i = 0; i < dat.count; ++i)
    {
        const FieldDatRecord& record = dat.records[i];
        if (record.globalID >= 1 && record.globalID <= 9)
        {
            ++datRecords[record.globalID - 1];
            loaded = loaded || (isSwapped(record.globalID) && record.loaded != 0);
        }
    }

    if (loaded)
    {
        finishField("BSX read finished before the resize; the original bodies were kept.", fieldExpected);
        return;
    }

    if (game->read<uint8_t>(FieldModelOffsets::AlternateLoadPath) != 0)
    {
        finishField("Field uses the alternate BCX-only load path; the original bodies were kept.", true);
        return;
    }

    bool swappedLoaded = false;
    std::array<bool, 9> enabledGlobals{};
    for (uint8_t i = 0; i < models.count; ++i)
    {
        FieldRuntimeModel& model = models.models[i];
        if (model.globalID < 1 || model.globalID > 9)
        {
            continue;
        }

        enabledGlobals[model.globalID - 1] = true;

        if (!isSwapped(model.globalID))
        {
            continue;
        }

        const GlobalModel& original = globalModel(model.globalID);
        if (model.bones != original.bones || model.parts != original.parts || model.face != faceID(model.globalID))
        {
            finishField("A model record has non-standard counts; the original bodies were kept.", true);
            return;
        }

        uint8_t replacement = replacementFor(model.globalID);
        const GlobalModel& body = globalModel(replacement);
        model.bones = body.bones;
        model.parts = body.parts;
        model.face = faceID(replacement);
        model.partsOffset = body.bones * 4;
        model.animationsOffset = body.bones * 4 + body.parts * 32;
        swappedLoaded = true;
    }

    if (!swappedLoaded)
    {
        finishField("No swapped models are loaded on this field.", false);
        return;
    }

    uint32_t resizedEnd = models.address + models.count * 36;
    for (uint8_t i = 0; i < models.count; ++i)
    {
        models.models[i].skeleton = resizedEnd;
        resizedEnd += metadataBytes(models.models[i]);
    }

    // Retained BCX geometry and the BSX follow the metadata, and the draw buffers sit inside the BSX's
    // lighting reserve. A smaller replacement leaves room unused rather than moving the reserve back.
    uint32_t retained = 0;
    int64_t extraDraw = 0;
    for (uint8_t id = 1; id <= 9; ++id)
    {
        if (enabledGlobals[id - 1])
        {
            retained += footprint(replacementFor(id)).retainedBytes;
        }

        if (isSwapped(id))
        {
            extraDraw += (int64_t(footprint(replacementFor(id)).drawBytes) - footprint(id).drawBytes) * datRecords[id - 1];
        }
    }
    extraLightingBytes = uint32_t(std::max<int64_t>(extraDraw, 0));

    // Refuse before anything is written if the enlarged lighting reserve could reach the live BSX staging buffer.
    uint32_t bsx = resizedEnd + retained;
    if (bsx + lighting.bsxBytes >= BsxStagingStart ||
        bsx + lighting.modelHeaderOffset + lighting.lightingOffset + extraLightingBytes + lighting.modelCount * 48 >= BsxStagingStart)
    {
        finishField("The enlarged lighting reserve would reach the BSX staging buffer; the original bodies were kept.", true);
        return;
    }

    ArchiveTable archive;
    for (uint8_t id = 1; id <= 9; ++id)
    {
        archive[id - 1] = globalModel(replacementFor(id)).archive;
    }

    // Records first and the archive last keeps the unavoidable write window as small as possible.
    game->write(models.address & RamMask, (uint8_t*)models.models.data(), models.count * 36);
    game->write<uint32_t>(FieldModelOffsets::FreePointer, resizedEnd);
    game->write<uint32_t>(FieldModelOffsets::RetainedBsx, resizedEnd);
    game->write(FieldModelOffsets::GlobalArchive, (uint8_t*)archive.data(), sizeof(archive));
    archiveInstalled = true;
    phase = Phase::AwaitBsx;
    status = "Records resized and archive redirected; waiting for the staged BSX.";
    LOG("Field models: field %d resized for the model swaps, %u extra lighting bytes.", fieldID, extraLightingBytes);
}

// The BSX stores a fixed lighting-table offset sized for the original draw buffers. Push it back by
// the replacements' extra packet bytes before BSX initialization copies it.
void FieldModelManager::prepareLightingReserve()
{
    if (retainedBsxMatches(game, lighting))
    {
        finishField("Lighting window missed; reload a clean state.", true);
        return;
    }

    if (!stagedBsxMatches(game, lighting))
    {
        return;
    }

    game->write<uint32_t>(FieldModelOffsets::BsxStaging + lighting.modelHeaderOffset + 12, lighting.lightingOffset + extraLightingBytes);
    phase = Phase::AwaitInit;
    status = "Lighting reserve enlarged; waiting for native model initialization.";
}

void FieldModelManager::finishFieldLoad()
{
    if (!modelsInitialized(game) || !retainedBsxMatches(game, lighting))
    {
        return;
    }
    uint32_t lightingTable = game->read<uint32_t>(FieldModelOffsets::LightingTable);

    uint32_t bsx = game->read<uint32_t>(FieldModelOffsets::RetainedBsx);
    if (lightingTable != bsx + lighting.modelHeaderOffset + lighting.lightingOffset + extraLightingBytes)
    {
        finishField("Lighting reserve edit landed too late; reload a clean state.", true);
        return;
    }

    if (!installIdleFallback())
    {
        finishField("Scene clip idle fallback failed validation.", true);
        return;
    }

    ++matches;
    finishField("Replacements loaded.", false);
}

// Clips from slot 3 up are scene gestures authored for the original skeleton. Unless the replacement
// shares that skeleton, alias each to the replacement's static, already scaled idle pose, keeping its
// frame count so script ranges and waits still work. Must run after native initialization or the
// shared idle data could be scaled repeatedly.
bool FieldModelManager::installIdleFallback()
{
    RuntimeModelTable models;
    if (!readRuntimeModels(game, models))
    {
        return false;
    }

    aliasedClips = 0;
    for (uint8_t i = 0; i < models.count; ++i)
    {
        const FieldRuntimeModel& model = models.models[i];
        if (!isSwapped(model.globalID) || sharesSkeleton(model.globalID, replacementFor(model.globalID)))
        {
            continue;
        }

        uint32_t table = model.skeleton + model.animationsOffset;
        if (model.bones != globalModel(replacementFor(model.globalID)).bones || model.clips < 3 || model.clips > 64 || !Utilities::isRAMPointer(table, model.clips * 16))
        {
            return false;
        }

        std::array<FieldAnimationDescriptor, 64> clips;
        if (!game->read(table & RamMask, model.clips * 16, (uint8_t*)clips.data()))
        {
            return false;
        }

        const FieldAnimationDescriptor idle = clips[0];
        if (idle.frames == 0 || idle.bones != model.bones)
        {
            return false;
        }

        for (uint8_t c = 3; c < model.clips; ++c)
        {
            uint16_t frames = clips[c].frames;
            clips[c] = idle;
            clips[c].frames = frames;
        }

        if (model.clips > 3)
        {
            game->write((table & RamMask) + 3 * 16, (uint8_t*)&clips[3], (model.clips - 3) * 16);
            aliasedClips += model.clips - 3;
        }
    }
    return true;
}

// The loaded bodies, VRAM and retained BSX stay as they are; only future loads use the original bodies.
void FieldModelManager::releaseField()
{
    ArchiveTable archive;
    if (archiveInstalled && fieldLoaderPresent(game) && readArchive(game, archive))
    {
        bool restored = false;
        for (uint8_t id = 1; id <= 9; ++id)
        {
            if (isSwapped(id) && archive[id - 1] == globalModel(replacementFor(id)).archive)
            {
                archive[id - 1] = globalModel(id).archive;
                restored = true;
            }
        }

        if (restored)
        {
            game->write(FieldModelOffsets::GlobalArchive, (uint8_t*)archive.data(), sizeof(archive));
        }
    }
    archiveInstalled = false;
}

void FieldModelManager::finishField(const char* newStatus, bool failed)
{
    releaseField();
    phase = Phase::Idle;
    status = newStatus;

    if (failed)
    {
        ++failures;
    }

    LOG("Field models: field %d: %s", fieldID, newStatus);
}
