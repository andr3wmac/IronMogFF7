#pragma once
#include <array>
#include <cstdint>

class GameManager;

// Gives characters another character's set of limit breaks. Limit levels, gauges and learned limits stay
// the character's own. A learned limit's position carries over to the source's limit in that position.
//
// - Each character's limit list in the kernel (resident battle and growth data) becomes the source's.
//   Battle init validates party members' limits against these lists, limits execute through them, and
//   every menu names limits by their IDs, so all of these follow.
// - Party members' player data (their current level's limit IDs and attack records) is kept consistent
//   with the lists between battles, before battle init checks it.
// - A source whose AI has a main (turn) script lends it: Vincent's transformed forms act through it.
// - Tifa's reels and Cait Sith's slots are chosen by hardcoded character checks in the battle overlay,
//   which are patched after it loads to point at whoever uses those limit sets.
//
// A limit's animation is looked up in whichever battle model is loaded, so this pairs with
// BattleModelManager loading the source character's model.
class BattleLimitManager
{
public:
    void setup(GameManager* game);

    // Uses sourceCharacterID's limit breaks for characterID (both 0-8). Party members' current limits change before the next battle.
    void setLimitSet(uint8_t characterID, uint8_t sourceCharacterID);
    void clearLimitSets();

    // Playtest telemetry.
    const char* getStatus() const { return status; }

private:
    static constexpr uint8_t NoSwap = 0xFF;

    void onExit();
    void onUpdate(bool justConnected);
    void onBattleEnter();
    bool hasLimitData() const;
    uint8_t limitSource(uint8_t characterID) const { return sets[characterID] == NoSwap ? characterID : sets[characterID]; }
    uint8_t characterUsing(uint8_t sourceCharacterID) const;

    void updateLimitLists(bool restore);
    void updatePartyLimits(bool restore);
    void updateCharacterAI(bool restore);
    void patchLimitMenus(bool restore);
    void restore();

    GameManager* game = nullptr;
    std::array<uint8_t, 9> sets = {NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap};

    const char* status = "No limit break swaps.";
};
