#pragma once

#include "core/emulators/Emulator.h"
#include "core/game/CustomItem.h"
#include "core/game/GameData.h"
#include "core/game/modules/BattleModule.h"
#include "core/game/modules/FieldModule.h"
#include "core/game/modules/MenuModule.h"
#include "core/game/modules/WorldModule.h"
#include "core/utilities/Event.h"
#include "core/utilities/Flags.h"
#include <array>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

class Extra;
class Rule;

class GameManager
{
public:
    enum class GameState : uint8_t
    {
        BootScreen   = 0,
        MainMenuCold = 1,
        MainMenuWarm = 2,
        InGame       = 3
    };

    GameManager();
    ~GameManager();
    
    bool connectToEmulator(std::string processName);
    bool connectToEmulator(std::string processName, uintptr_t memoryAddress);
    bool isPaused() { return emulatorPaused; }

    // True once the emulator connection has succeeded. Memory access is a no-op before then.
    bool isConnected() const { return connected.load(std::memory_order_acquire); }

    // Queues an action to run on the game manager thread at the start of the next update. Use this from the
    // GUI thread for anything that mutates game, rule, or extra state. Safe to call from any thread.
    void queueAction(std::function<void()> action);

    bool isRuleEnabled(std::string ruleName);
    Rule* getRule(std::string ruleName);
    bool isExtraEnabled(std::string extraName);
    Extra* getExtra(std::string extraName);
    std::string getSettingsSummary();

    void setup(GameVersion version, uint32_t inputSeed);
    void loadSaveData();
    void clearSaveData();
    inline uint32_t getSeed() { return seed; }
    GameState getState();
    bool update();

    float getDifficultyScale() { return difficultyScale.load(); }
    void setDifficultyScale(float newScale);

    // Returns how long the last update() took in ms.
    double getLastUpdateDuration() { return lastUpdateDuration; }

    GameVersion getGameVersion() { return gameVersion; }
    uint8_t getGameModule() { return gameModule; }
    uint16_t getGameMoment();
    bool inBattle() { return battle.inBattle(); }
    bool inMenu();
    uint16_t getFieldID() { return field.getFieldID(); }

    // Returns a list of the character IDs that are currently in the party. 0xFF is the slot is empty.
    std::array<uint8_t, 3> getPartyIDs();

    // Returns true if character is in party.
    bool inParty(uint8_t characterID);

    // Returns true if character is currently available on PHS.
    bool isPHSVisible(uint8_t characterID);

    // Returns a list of item IDs currently in the party's possession.
    std::array<uint16_t, 320> getPartyInventory();
    void setInventorySlot(uint32_t slotIndex, uint16_t itemID, uint8_t quantity);

    // Custom items: registers an item in one of FF7's unused slots and returns its assigned id.
    // Register during a rule's setup; the registry is rebuilt each time the game is connected.
    uint16_t registerCustomItem(const CustomItem& item);

    // True if any custom items are registered for the current game.
    bool hasCustomItems() { return !customItems.empty(); }

    // Returns the registered custom item with the given id, or nullptr if it isn't a custom item.
    const CustomItem* findCustomItem(uint16_t itemID);

    // Returns all custom items registered for the current game.
    const std::vector<CustomItem>& getCustomItems() { return customItems; }

    // Returns a list of materia IDs currently in the party's possession.
    std::array<uint32_t, 200> getPartyMateria();

    // Returns the last text displayed in a window
    std::string getWindowText(uint8_t index);

    // Returns requested formation data and battle scene
    std::pair<BattleScene*, BattleFormation*> getBattleFormation(uint16_t formationID);

    // Returns the current battle scene and formation.
    std::pair<BattleScene*, BattleFormation*> getBattleFormation();

    // Given an offset to a battle character this function will apply a multiplier to each of the chosen stats.
    void applyBattleStatMultiplier(uintptr_t battleCharOffset, StatMultiplierSet& multiplierSet, bool defenseSoftCap = false);
    void applyBattleStatMultiplier(uintptr_t battleCharOffset, float multiplier, bool applyToHP = true, bool applyToMP = true, bool applyToStats = true, bool defenseSoftCap = false);

    // Returns the pointer to the line of field script last executed for a given group index.
    uint16_t getScriptExecutionPointer(uint8_t groupIndex) { return fieldScriptExecutionTable[groupIndex]; }

    // Modules
    BattleModule battle;
    FieldModule field;
    MenuModule menu;
    WorldModule world;

    // Events
    Event<> onStart;
    Event<> onNewGame;
    Event<> onGameOver;
    Event<bool> onUpdate;                   // Triggers when IronMog updates which is more frequent than the game framerate. 
    Event<> onEmulatorPaused;
    Event<> onEmulatorResumed;
    Event<int> onFrame;                     // Triggers when the game's frame number advances.
    Event<uint8_t> onModuleChanged;
    Event<uint16_t> onGameMomentChanged;
    Event<> onBattleEnter;
    Event<> onBattleResumed;                // Triggers instead of onBattleEnter when connecting mid-battle. Only for per-battle bookkeeping, never modify the battle here.
    Event<uint16_t> onBattleTransition;     // Triggers when a battle transitions from one formation to another. Like a multi-phase boss.
    Event<> onBattleExit;
    Event<uint16_t> onFieldChanged;
    Event<> onShopOpened;
    Event<uint8_t> onShopMenuChanged;       // Triggers when player moves the cursor between Buy and Sell in shop menu.
    Event<std::string> onNameEntryOpened;
    Event<> onWorldMapEnter;
    Event<float> onDifficultyScaleChanged;  // Triggers when the difficulty scaling changes, intended to trigger rules to update.
    Event<CustomItemUse> onCustomItemUsed;   // Triggers when a registered custom item is used from the menu.

    // Read/Write RAM Functions
    template <typename T>
    T read(uintptr_t offset)
    {
        T value{};
        if (isConnected())
        {
            emulator->read(offset, &value, sizeof(value));
        }
        return value;
    }

    bool read(uintptr_t offset, uintptr_t size, uint8_t* dataOut)
    {
        return isConnected() && emulator->read(offset, dataOut, size);
    }

    template <typename T>
    void write(uintptr_t offset, T value)
    {
        if (isConnected())
        {
            emulator->write(offset, &value, sizeof(value));
        }
    }

    void write(uintptr_t offset, uint8_t* dataIn, uintptr_t size)
    {
        if (isConnected())
        {
            emulator->write(offset, dataIn, size);
        }
    }

    std::string readString(uintptr_t offset, uint32_t length);
    size_t writeString(uintptr_t offset, uint32_t length, const std::string& string, bool centerAlign = false);

private:
    // Rebuilds the custom item registry, fires onStart, then injects registered items into the kernel.
    void onGameStart();
    void injectCustomItems();
    void runQueuedActions();

    Emulator* emulator;
    std::atomic<bool> connected = false;
    GameVersion gameVersion = GameVersion::PlayStationUS;
    uint8_t gameDisc = 1;

    GameState lastGameState = GameState::BootScreen;
    uint16_t lastGameMoment = 0;
    bool emulatorPaused = false;
    double lastUpdateDuration = 0.0;
    uint32_t seed = 0;
    uint8_t gameModule = 0;
    uint32_t frameNumber = 0;
    int updatesSinceFrame = 0;
    int framesSinceReload = 0;
    bool justEnteredGame = false;
    bool waitingForGameOver = false;
    std::atomic<float> difficultyScale = 1.0f;

    // A set of pointers to the last line of field script executed within each group.
    uint16_t fieldScriptExecutionTable[64];

    // Custom item registry, rebuilt each game start. Menu-use detection lives in MenuModule.
    std::vector<CustomItem> customItems;

    // Actions queued from other threads, drained by update().
    std::mutex queuedActionsMutex;
    std::vector<std::function<void()>> queuedActions;
};