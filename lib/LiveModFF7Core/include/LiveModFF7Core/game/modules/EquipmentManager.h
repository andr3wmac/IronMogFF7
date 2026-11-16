#pragma once
#include <array>
#include <cstdint>

class GameManager;

// Gives characters another character's equipment: the weapons (and gender-restricted armor) the source
// can equip, and the source's starting weapon.
//
// - Who can equip an item is a character-ID bit mask in the resident kernel weapon and armor data. Each
//   character's bit is set wherever the source's native bit is, so the equip menu, shops and battle all follow.
//   Bits above the nine characters (Young Cloud, Sephiroth) and the Masamune are left alone.
// - Outside battles and menus, a character holding a weapon they can no longer equip is given the source's
//   starting weapon. This covers New Game, Cait Sith's and Vincent's joins (their records are rewritten
//   then) and enabling the swap mid-run. Materia that no longer fits moves to the materia list, and a
//   replaced weapon other than a starting weapon goes to the inventory.
class EquipmentManager
{
public:
    void setup(GameManager* game);

    // characterID (0-8) equips what sourceCharacterID (0-8) natively equips and starts with their weapon.
    void setEquipSource(uint8_t characterID, uint8_t sourceCharacterID);
    void clearEquipSources();

    // The weapon (0-127, item ID - 128) characterID starts with, which they can always equip.
    uint8_t getStartingWeapon(uint8_t characterID) const;

    // Playtest telemetry.
    const char* getStatus() const { return status; }

private:
    static constexpr uint8_t NoSwap = 0xFF;

    void onExit();
    void onFrame(int frameNumber);
    uint8_t equipSource(uint8_t characterID) const { return sources[characterID] == NoSwap ? characterID : sources[characterID]; }
    bool hasSwaps() const;

    bool updateMasks(bool restore);
    void updateStartingWeapons();
    void replaceWeapon(uint8_t characterID, uint8_t weapon);

    GameManager* game = nullptr;
    std::array<uint8_t, 9> sources = {NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap, NoSwap};

    const char* status = "No equipment swaps.";
};
