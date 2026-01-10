#pragma once

#include "core/emulators/Emulator.h"
#include "core/utilities/Event.h"
#include <string>
#include <array>

struct BattleScene;
struct BattleFormation;
class Extra;
class Rule;

class GameManager
{
public:
    GameManager();
    ~GameManager();
    
    bool connectToEmulator(std::string processName);
    bool connectToEmulator(std::string processName, uintptr_t memoryAddress);

    bool isRuleEnabled(std::string ruleName);
    Rule* getRule(std::string ruleName);
    bool isExtraEnabled(std::string extraName);
    Extra* getExtra(std::string extraName);

    void setup(uint32_t inputSeed);
    void loadSaveData();
    inline uint32_t getSeed() { return seed; }
    void update();

    // Returns how long the last update() took in ms.
    double getLastUpdateDuration() { return lastUpdateDuration; }

    // Returns a byte representing what module the game is. eg Field, Battle, World, etc
    uint8_t getGameModule() { return gameModule; }
    uint16_t getGameMoment();
    bool inBattle();
    bool inMenu();
    uint16_t getFieldID() { return fieldID; }
    int getFramesInField() { return framesInField; }

    // Returns a list of the character IDs that are currently in the party. 0xFF is the slot is empty.
    std::array<uint8_t, 3> getPartyIDs();

    // Returns a list of item IDs currently in the party's possession.
    std::array<uint16_t, 320> getPartyInventory();
    void setInventorySlot(uint32_t slotIndex, uint16_t itemID, uint8_t quantity);

    // Returns a list of materia IDs currently in the party's possession.
    std::array<uint32_t, 200> getPartyMateria();

    // Finds the nearest message that contains the item name
    int findPickUpMessage(std::string itemName, uint8_t group, uint8_t script, uint32_t offset);

    // Returns the last dialog text that was displayed.
    std::string getLastDialogText();

    // Returns the current battle scene and formation.
    std::pair<BattleScene*, BattleFormation*> getBattleFormation();

    // Returns the pointer to the line of field script last executed for a given group index.
    uint16_t getScriptExecutionPointer(uint8_t groupIndex) { return fieldScriptExecutionTable[groupIndex]; }

    // Events
    Event<> onStart;
    Event<> onUpdate;
    Event<> onEmulatorPaused;
    Event<> onEmulatorResumed;
    Event<int> onFrame;
    Event<uint8_t> onModuleChanged;
    Event<> onBattleEnter;
    Event<> onBattleExit;
    Event<uint16_t> onFieldChanged;
    Event<> onShopOpened;

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

    void write(uintptr_t offset, uint8_t* dataIn, uintptr_t size)
    {
        emulator->write(offset, dataIn, size);
    }

    std::string readString(uintptr_t offset, uint32_t length);
    void writeString(uintptr_t offset, uint32_t length, const std::string& string, bool centerAlign = false);

private:
    Emulator* emulator;

    bool hasStarted = false;
    bool emulatorPaused = false;
    double lastUpdateDuration = 0.0;
    uint32_t seed = 0;
    uint8_t gameModule = 0;
    uint32_t frameNumber = 0;
    double lastFrameUpdateTime = 0.0;
    int framesSinceReload = 0;

    uint16_t fieldID = 0;
    int framesInField = 0;

    // A set of pointers to the last line of field script executed within each group. 
    uint16_t fieldScriptExecutionTable[64];

    bool waitingForBattleData = false;
    bool isBattleDataLoaded();

    bool waitingForFieldData = false;
    bool isFieldDataLoaded();

    bool waitingForShopData = false;
    bool wasInShopMenu = false;
    bool isShopDataLoaded();
};