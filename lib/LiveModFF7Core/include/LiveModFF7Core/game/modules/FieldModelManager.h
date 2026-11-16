#pragma once
#include "LiveModFF7Core/game/GameData.h"
#include <array>
#include <cstdint>

class GameManager;

// Loads other global party models' bodies in place of the originals during native field loads. Each
// replaced record keeps its identity, scripts and animation slots; only the body, face and base clips
// come from the replacement. Scene clips authored for a different skeleton play the replacement's
// standing pose, keeping their frame counts.
//
// Every edit is made during a disc read that precedes the step consuming it, polled through
// GameManager::onLoadUpdate. Nothing runs until a swap is registered.
//
// Independently of swaps, triggers GameManager::onFieldModelsReady once each field load has
// initialized its models (after any swaps are in place).
class FieldModelManager
{
public:
    void setup(GameManager* game);

    // Loads replacementGlobalID's body for global model globalID (both 1-9) on every field load. Swaps
    // can form any mapping, including a full permutation. Bodies already loaded change on the next load.
    void setSwap(uint8_t globalID, uint8_t replacementGlobalID);
    void clearSwaps();
    bool hasSwaps() const;

    // Playtest telemetry.
    uint16_t getFieldID() const { return fieldID; }
    const char* getStatus() const { return status; }
    uint32_t getMatches() const { return matches; }
    uint32_t getFailures() const { return failures; }
    uint32_t getAliasedClips() const { return aliasedClips; }

private:
    // Native field loading happens in phases. Each edit is made during a disc read that precedes
    // the step consuming it, so polling has tens of milliseconds rather than a race against the CPU.
    enum class Phase : uint8_t
    {
        Idle,
        AwaitAllocation, // During the BSX read: resize the swapped records and redirect the archive.
        AwaitBsx,        // During the BCX reads: enlarge the lighting reserve in the staged BSX.
        AwaitInit,       // After initialization: alias scene clips to the idle pose.
    };

    void onStart();
    void onExit();
    void onLoadUpdate();
    void onModuleChanged(uint8_t newModule);
    void onFieldChanged(uint16_t fieldID);
    void notifyModelsReady();

    void beginField(uint16_t fieldID);
    void updateField();
    void resizeAllocation();
    void prepareLightingReserve();
    void finishFieldLoad();
    bool installIdleFallback();
    void releaseField();
    void finishField(const char* status, bool failed);
    bool isSwapped(uint8_t globalID) const { return globalID >= 1 && globalID <= 9 && swaps[globalID - 1] != 0; }
    uint8_t replacementFor(uint8_t globalID) const { return isSwapped(globalID) ? swaps[globalID - 1] : globalID; }

    GameManager* game = nullptr;
    std::array<uint8_t, 9> swaps{};     // Replacement global model ID by global model ID - 1, 0 when unchanged.

    uint8_t lastModule = 0;
    bool modelsLoading = false;         // A field load was seen that has not finished initializing its models.
    bool modelsWereInitialized = false; // Models were initialized last update, so losing that starts a load.
    Phase phase = Phase::Idle;
    uint16_t fieldID = 0;
    bool fieldObserved = false;         // A field was seen since start, so a missed window is a real miss.
    bool fieldExpected = false;
    bool archiveInstalled = false;
    FieldModelLighting lighting;        // Generated BSX layout of the field being loaded.
    uint32_t extraLightingBytes = 0;

    const char* status = "No field model swaps.";
    uint32_t matches = 0, failures = 0, aliasedClips = 0;
};
