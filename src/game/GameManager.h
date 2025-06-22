#pragma once

#include "emulators/Emulator.h"
#include "utilities/Event.h"
#include <string>
#include <array>

class Rule;

class GameManager
{
public:
    GameManager();
    ~GameManager();
    
    bool attachToEmulator(std::string processName);
    bool attachToEmulator(std::string processName, uintptr_t memoryAddress);

    void addRule(Rule* rule);

    void start(uint32_t inputSeed);
    inline uint32_t getSeed() { return seed; }
    void update();

    // Returns a byte representing what module the game is. eg Field, Battle, World, etc
    uint8_t getGameModule() { return gameModule; }
    bool inBattle();
    uint16_t getFieldID() { return fieldID; }

    // Returns a list of the character IDs that are currently in the party. 0xFF is the slot is empty.
    std::array<uint8_t, 3> getPartyIDs();

    // Returns a list of item IDs currently in the party's possession.
    std::array<uint16_t, 320> getPartyInventory();
    void setInventorySlot(uint32_t slotIndex, uint16_t itemID, uint8_t quantity);

    // Returns a list of materia IDs currently in the party's possession.
    std::array<uint32_t, 200> getPartyMateria();

    // Events
    Event<int> onFrame;
    Event<> onBattleEnter;
    Event<> onBattleExit;
    Event<uint16_t> onFieldChanged;

    // Read/Write RAM Functions
    template <typename T>
    T read(uintptr_t offset)
    {
        T value{};
        emulator->read(offset, &value, sizeof(value));
        return value;
    }

    bool read(uintptr_t offset, uintptr_t size, uint8_t* dataOut)
    {
        return emulator->read(offset, dataOut, size);
    }

    template <typename T>
    void write(uintptr_t offset, T value)
    {
        emulator->write(offset, &value, sizeof(value));
    }

private:
    Emulator* emulator;
    std::vector<Rule*> rules;

    uint32_t seed = 0;
    uint32_t frameNumber = 0;
    uint8_t gameModule = 0;

    uint16_t fieldID = 0;

    bool waitingForBattleData = false;
    bool isBattleDataLoaded();
};