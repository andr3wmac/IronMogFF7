#pragma once
#include <array>
#include <cstdint>

class GameManager;

// Loads other characters' battle models (body, textures and animations) for party members during
// native battle loads, keeping each member's identity. The archive entry a member's load reads is
// redirected once the battle overlay is resident and before that member's disc read, then restored once
// every swapped body has loaded. The replacement's base model is used. Polled through GameManager::onLoadUpdate.
class BattleModelManager
{
public:
    void setup(GameManager* game);

    // Loads replacementCharacterID's battle model for characterID (both 0-8) in every battle. Swaps can
    // form any mapping, including a full permutation. Bodies already loaded change in the next battle.
    void setSwap(uint8_t characterID, uint8_t replacementCharacterID);
    void clearSwaps();
    bool hasSwaps() const;

    // The character whose base battle model characterID loads; characterID itself when unswapped.
    uint8_t getReplacement(uint8_t characterID) const { return isSwapped(characterID) ? swaps[characterID] : characterID; }

    // Playtest telemetry.
    const char* getStatus() const { return status; }
    uint32_t getMatches() const { return matches; }
    uint32_t getFailures() const { return failures; }

private:
    static constexpr uint8_t NoSwap = 0xFF;

    // A swapped party member in one physical party slot.
    struct SlotState
    {
        uint8_t character = NoSwap;
        uint16_t archiveIndex = 0; // Archive entry the member's load reads, which varies with weapon.
        bool redirected = false;
        bool bodyCleared = false;  // Seen without a body this battle; until then the pointer is the last battle's.
        bool resolved = false;
    };

    void onStart();
    void onExit();
    void onLoadUpdate();
    void onModuleChanged(uint8_t newModule);

    void updateBattle();
    void releaseBattle();
    void finishBattle(const char* status, bool failed);
    bool isSwapped(uint8_t characterID) const { return characterID < swaps.size() && swaps[characterID] != NoSwap; }

    GameManager* game = nullptr;
    std::array<uint8_t, 9> swaps = {NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap};

    uint8_t lastModule = 0;
    bool pending = false;
    std::array<SlotState, 3> slots;

    const char* status = "No battle model swaps.";
    uint32_t matches = 0, failures = 0;
};
