#include "GameManager.h"
#include "core/audio/AudioManager.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "extras/Extra.h"
#include "rules/Rule.h"

#include <thread>
#include <chrono>

GameManager::GameManager()
    : emulator(nullptr)
{
    memset(fieldScriptExecutionTable, 0, 128);
}

GameManager::~GameManager()
{
    if (emulator != nullptr)
    {
        delete emulator;
    }
}

bool GameManager::connectToEmulator(std::string processName)
{
    emulator = Emulator::getEmulatorFromProcessName(processName);
    if (emulator == nullptr)
    {
        return false;
    }

    return emulator->connect(processName);
}

bool GameManager::connectToEmulator(std::string processName, uintptr_t memoryAddress)
{
    emulator = Emulator::getEmulatorCustom(processName, memoryAddress);
    if (emulator == nullptr)
    {
        return false;
    }

    return emulator->connect(processName);
}

std::string GameManager::readString(uintptr_t offset, uint32_t length)
{
    std::vector<uint8_t> strData;
    strData.resize(length);
    emulator->read(offset, strData.data(), length);
    return GameData::decodeString(strData);
}

void GameManager::writeString(uintptr_t offset, uint32_t length, const std::string& string, bool centerAlign)
{
    std::vector<uint8_t> strData = GameData::encodeString(string);

    std::vector<uint8_t> finalStrData;
    finalStrData.resize(length, 0x00); // Fill with 0x00 by default

    size_t strLen = std::min<size_t>(strData.size(), length);
    size_t padding = centerAlign ? (length - strLen) / 2 : 0;

    for (size_t i = 0; i < strLen; ++i)
    {
        finalStrData[padding + i] = strData[i];
    }

    emulator->write(offset, finalStrData.data(), length);
}

bool GameManager::isRuleEnabled(std::string ruleName)
{
    for (Rule* rule : Rule::getList())
    {
        if (rule->enabled && rule->name == ruleName)
        {
            return true;
        }
    }

    return false;
}

Rule* GameManager::getRule(std::string ruleName)
{
    for (Rule* rule : Rule::getList())
    {
        if (rule->enabled && rule->name == ruleName)
        {
            return rule;
        }
    }

    return nullptr;
}

bool GameManager::isExtraEnabled(std::string extraName)
{
    for (Extra* extra : Extra::getList())
    {
        if (extra->enabled && extra->name == extraName)
        {
            return true;
        }
    }

    return false;
}

Extra* GameManager::getExtra(std::string extraName)
{
    for (Extra* extra : Extra::getList())
    {
        if (extra->enabled && extra->name == extraName)
        {
            return extra;
        }
    }

    return nullptr;
}

void GameManager::setup(uint32_t inputSeed)
{
    // Note: seed may change after loading a save file, so its important to not utilize it in rule setup.
    seed = inputSeed;

    for (Rule* rule : Rule::getList())
    {
        if (!rule->enabled)
        {
            continue;
        }
        rule->setManager(this);
        rule->setup();
    }

    for (Extra* extra : Extra::getList())
    {
        if (!extra->enabled)
        {
            continue;
        }
        extra->setManager(this);
        extra->setup();
    }
}

void GameManager::loadSaveData()
{
    const uint8_t saveDataVersion = 0;

    uint16_t ironMogID = read<uint16_t>(SavemapOffsets::IronMogSave);

    // 49 and 4D are the letters IM for Iron Mog
    if (ironMogID == 0x4D49)
    {
        // Load existing save data.
        seed = read<uint32_t>(SavemapOffsets::IronMogSeed);

        std::string seedString = Utilities::seedToHexString(seed);
        LOG("Loaded seed from save file: %s", seedString.c_str());
    }
    else 
    {
        clearSaveData();

        // Write header and seed into save data area.
        write<uint16_t>(SavemapOffsets::IronMogSave, 0x4D49);
        write<uint8_t>(SavemapOffsets::IronMogVersion, saveDataVersion);

        // Write seed
        write<uint32_t>(SavemapOffsets::IronMogSeed, seed);
    }
}

void GameManager::clearSaveData()
{
    // Zero out the area.
    for (int i = 0; i < 8; ++i)
    {
        write<uint32_t>(SavemapOffsets::IronMogSave + (i * 4), 0);
    }
}

GameManager::GameState GameManager::getState()
{
    uint8_t gameModule = read<uint8_t>(GameOffsets::CurrentModule);

    // Not really sure what this is actually meant for but its consistently
    // 27 when on the main menu, and 26 when on game over screen.
    uint8_t screenState = read<uint8_t>(0xEFBB1);

    if (gameModule == 0 && screenState == 0)
    {
        return GameState::BootScreen;
    }

    if (gameModule == 0 && screenState == 27)
    {
        // Fresh boot, main menu.
        return GameState::MainMenuCold;
    }

    if (gameModule != 0 && screenState == 27)
    {
        // Main menu after a soft reset or game over.
        return GameState::MainMenuWarm;
    }

    return GameState::InGame;
}

bool GameManager::update()
{
    double currentTime = Utilities::getTimeMS();
    
    // If read/write errors have occured then connection has been broken.
    if (emulator->pollErrors())
    {
        return false;
    }

    GameState state = getState();
    {
        if (lastGameState == GameState::InGame && state != GameState::InGame)
        {
            // Clearing save data prevents stale state getting stuck from a game over.
            clearSaveData();
            AudioManager::pauseMusic();
        }

        if (lastGameState != GameState::InGame && state == GameState::InGame)
        {
            loadSaveData();
            onStart.invoke();
            lastFrameUpdateTime = Utilities::getTimeMS();
        }

        lastGameState = state;
    }

    // Only perform updates when we're actually in the game.
    if (state != GameState::InGame)
    {
        return true;
    }

    // We assume if 200ms has passed without the frame number advancing that the emulator is paused
    
    if (currentTime - lastFrameUpdateTime > 200 && !emulatorPaused)
    {
        emulatorPaused = true;
        onEmulatorPaused.invoke();
        LOG("Emulator paused.");
    }

    // Update the field script execution table.
    read(FieldScriptOffsets::ExecutionTable, 128, (uint8_t*)(&fieldScriptExecutionTable[0]));

    onUpdate.invoke();
    
    uint8_t newGameModule = read<uint8_t>(GameOffsets::CurrentModule);
    if (newGameModule != gameModule)
    {
        // Entered battle
        if (gameModule != GameModule::Battle && newGameModule == GameModule::Battle)
        {
            waitingForBattleData = true;
        }

        // Exited battle
        if (gameModule == GameModule::Battle && newGameModule != GameModule::Battle)
        {
            onBattleExit.invoke();
        }

        if (gameModule == GameModule::Battle && newGameModule == GameModule::Field)
        {
            waitingForFieldData = true;
        }

        if (gameModule != GameModule::World && newGameModule == GameModule::World)
        {
            waitingForWorldData = true;
            lastWorldScreenFade = read<uint8_t>(GameOffsets::WorldScreenFade);
        }

        if (gameModule != GameModule::Menu && newGameModule == GameModule::Menu)
        {
            uint8_t menuType = read<uint8_t>(GameOffsets::MenuType);
            if (menuType == MenuType::Shop)
            {
                waitingForShopData = true;
                wasInShopMenu = true;
            }
        }

        if (newGameModule != GameModule::Menu && wasInShopMenu)
        {
            // HACK: when we exit a shop sometimes the field doesn't overwrite the shop data
            // so it stays stale in memory, then the next time we open the shop isShopDataLoaded()
            // gets false positive from old memory. So, we corrupt one of the materia prices on exit.
            uint32_t lastMateriaPrice = read<uint32_t>(ShopOffsets::MateriaPricesStart + (68 * 4));
            if (lastMateriaPrice == 9000) 
            {
                write<uint32_t>(ShopOffsets::MateriaPricesStart + (68 * 4), 0);
            }
            wasInShopMenu = false;
        }

        // Game module changed.
        gameModule = newGameModule;
        onModuleChanged.invoke(gameModule);
    }

    if (gameModule == GameModule::Battle)
    {
        if (waitingForBattleData && isBattleDataLoaded())
        {
            onBattleEnter.invoke();
            waitingForBattleData = false;
        }
    }

    if (gameModule == GameModule::Field)
    {
        // Detect if we're warping into the same field we're already in, this 
        // is a reload and otherwise wouldn't trigger onFieldChanged.
        uint8_t fieldWarpTrigger = read<uint8_t>(GameOffsets::FieldWarpTrigger);
        if (fieldWarpTrigger == 1 && !waitingForFieldData)
        {
            uint16_t fieldWarpID = read<uint16_t>(GameOffsets::FieldWarpID);
            if (fieldID == fieldWarpID)
            {
                waitingForFieldData = true;
                framesInField = 0;
                lastFieldScreenFade = read<uint16_t>(GameOffsets::FieldScreenFade);
            }
        }

        uint16_t newFieldID = read<uint16_t>(GameOffsets::FieldID);
        if (newFieldID != fieldID)
        {
            waitingForFieldData = true;
            fieldID = newFieldID;
            framesInField = 0;
            lastFieldScreenFade = read<uint16_t>(GameOffsets::FieldScreenFade);
        }

        if (waitingForFieldData && isFieldDataLoaded())
        {
            LOG("Loaded Field: %d", fieldID);
            onFieldChanged.invoke(fieldID);
            waitingForFieldData = false;
        }
    }

    if (gameModule == GameModule::World)
    {
        if (waitingForWorldData && isWorldDataLoaded())
        {
            // When exiting onto the world map field ID is updated to your exit location
            // We don't trigger the onFieldChanged event for this but its important we update
            // this value in case we re-enter the field we just left.
            uint16_t newFieldID = read<uint16_t>(GameOffsets::FieldID);
            fieldID = newFieldID;
            waitingForFieldData = false;

            LOG("Entered world map.");
            onWorldMapEnter.invoke();
            waitingForWorldData = false;
        }
    }

    if (gameModule == GameModule::Menu)
    {
        if (waitingForShopData && isShopDataLoaded())
        {
            onShopOpened.invoke();
            waitingForShopData = false;
        }
    }

    uint32_t newFrameNumber = read<uint32_t>(GameOffsets::FrameNumber);

    // A jump in frame number likely indicates a load game or load save state.
    int frameDifference = std::abs((int)newFrameNumber - (int)frameNumber);
    if (frameDifference > 30 && framesSinceReload > 30)
    {
        double timeGap = currentTime - lastFrameUpdateTime;
        LOG("Load detected, reloading rules %lf", timeGap);
        loadSaveData();
        onStart.invoke();
        framesSinceReload = 0;
    }
    framesSinceReload++;

    // Detect change in frame number and trigger event
    if (newFrameNumber != frameNumber)
    {
        frameNumber = newFrameNumber;
        lastFrameUpdateTime = currentTime;
        framesInField++;

        if (emulatorPaused)
        {
            LOG("Emulator resumed.");
            onEmulatorResumed.invoke();
            emulatorPaused = false;
        }
        
        onFrame.invoke(newFrameNumber);
    }

    lastUpdateDuration = Utilities::getTimeMS() - currentTime;
    return true;
}

std::array<uint8_t, 3> GameManager::getPartyIDs()
{
    std::array<uint8_t, 3> results = { 0xFF, 0xFF, 0xFF };

    results[0] = read<uint8_t>(GameOffsets::PartyIDList + 0);
    results[1] = read<uint8_t>(GameOffsets::PartyIDList + 1);
    results[2] = read<uint8_t>(GameOffsets::PartyIDList + 2);

    return results;
}

std::array<uint16_t, 320> GameManager::getPartyInventory()
{
    std::array<uint16_t, 320> results;
    emulator->read(GameOffsets::Inventory, results.data(), sizeof(uint16_t) * 320);
    return results;
}

void GameManager::setInventorySlot(uint32_t slotIndex, uint16_t itemID, uint8_t quantity)
{
    if (slotIndex >= 320)
    {
        return;
    }

    uint16_t data = (quantity << 9) | (itemID & 0x1FF);
    emulator->write(GameOffsets::Inventory + (sizeof(uint16_t) * slotIndex), &data, sizeof(uint16_t));
}

std::array<uint32_t, 200> GameManager::getPartyMateria()
{
    std::array<uint32_t, 200> results;
    emulator->read(GameOffsets::MateriaInventory, results.data(), sizeof(uint32_t) * 200);
    return results;
}

uint16_t GameManager::getGameMoment()
{
    return read<uint16_t>(GameOffsets::GameMoment);
}

bool GameManager::inBattle()
{
    return gameModule == GameModule::Battle && !waitingForBattleData;
}

bool GameManager::inMenu()
{
    return gameModule == GameModule::Menu;
}

// When we switch to the battle module the actual data for the battle isn't fully loaded
// so we can't time events to overwrite it. We introduce some heuristics to try to ensure
// the data is fully loaded before we let anything modify it.
bool GameManager::isBattleDataLoaded()
{
    if (gameModule != GameModule::Battle)
    {
        return false;
    }

    int verifiedPlayers = 0;
    int verifiedEnemyDrops = 0;

    // Verify that player data has been loaded
    // We do this by watching for the players HP to be copied into the battle allies area
    {
        std::array<uint8_t, 3> partyIDs = getPartyIDs();
        for (int i = 0; i < 3; ++i)
        {
            uint8_t id = partyIDs[i];
            if (id == 0xFF)
            {
                verifiedPlayers++;
                continue;
            }

            uintptr_t characterOffset = getCharacterDataOffset(id);
            uint16_t worldMaxHP = read<uint16_t>(characterOffset + CharacterDataOffsets::MaxHP);
            uint16_t battleMaxHP = read<uint16_t>(BattleOffsets::Allies[i] + BattleOffsets::MaxHP);

            if (worldMaxHP == battleMaxHP)
            {
                verifiedPlayers++;
            }
        }
    }

    // Verify that enemy data has been loaded
    // The concept here is that at least one drop slot of one of the enemies in all battle formations is going to be
    // 65535, however before the enemy data is loaded none of them are equal to that.
    {
        for (int i = 0; i < 3; ++i)
        {
            // Maximum of 4 item slots per enemy
            for (int j = 0; j < 4; ++j)
            {
                uint16_t dropID = read<uint16_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::DropIDs[j]);

                // Empty slot
                if (dropID == 65535)
                {
                    verifiedEnemyDrops++;
                }
            }
        }
    }

    if (verifiedPlayers == 3 && verifiedEnemyDrops > 0)
    {
        return true;
    }

    return false;
}

// Detect if field data is fully loaded by verifying the set of information
// we know about the field is confirmed in memory.
bool GameManager::isFieldDataLoaded()
{
    if (gameModule != GameModule::Field)
    {
        return false;
    }

    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return false;
    }

    int randomizedFieldItems = 0;
    int randomizedFieldMateria = 0;

    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldScriptItem& item = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
        uintptr_t itemQuantityOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemQuantity;

        uint16_t itemID = read<uint16_t>(itemIDOffset);
        uint8_t itemQuantity = read<uint8_t>(itemQuantityOffset);

        if (itemID != item.id || itemQuantity != item.quantity)
        {
            return false;
        }
    }

    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldScriptItem& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;

        uint8_t materiaID = read<uint8_t>(idOffset);

        if (materiaID != materia.id)
        {
            return false;
        }
    }

    for (int i = 0; i < fieldData.messages.size(); ++i)
    {
        FieldScriptMessage& message = fieldData.messages[i];
        uint8_t opCode = read<uint8_t>(FieldScriptOffsets::ScriptStart + message.offset);
        uint8_t endChar = read<uint8_t>(FieldScriptOffsets::ScriptStart + message.strOffset + message.strLength);
        if (opCode != 0x40 || endChar != 0xFF)
        {
            return false;
        }
    }

    for (int i = 0; i < fieldData.worldExits.size(); ++i)
    {
        FieldWorldExit& exit = fieldData.worldExits[i];
        uintptr_t exitOffset = FieldScriptOffsets::TriggersStart + exit.offset;

        uint16_t fieldID = read<uint8_t>(exitOffset);
        if (fieldID != exit.fieldID)
        {
            return false;
        }
    }

    // Encounter data
    {
        for (int t = 0; t < 2; ++t)
        {
            uintptr_t tableOffset = FieldScriptOffsets::EncounterStart + fieldData.encounterOffset + (t * FieldScriptOffsets::EncounterTableStride);

            Encounter encTable[10];
            read(tableOffset + 2, sizeof(uint16_t) * 10, (uint8_t*)encTable);

            for (int i = 0; i < 10; ++i)
            {
                Encounter& origEncounter = fieldData.getEncounter(t, i);
                if (origEncounter.prob == 0 && origEncounter.id == 0)
                {
                    continue;
                }

                if (origEncounter.prob != encTable[i].prob || origEncounter.id != encTable[i].id)
                {
                    return false;
                }
            }
        }
    }

    // Field is ready if we've hit peak fade out and started coming back down.
    uint16_t screenFade = read<uint16_t>(GameOffsets::FieldScreenFade);
    bool isScreenReady = (lastFieldScreenFade == 0x100 && screenFade < lastFieldScreenFade);
    lastFieldScreenFade = screenFade;

    return isScreenReady;
}

// Detect if shop data is fully loaded by verifying the set of information
// we know about the shop is confirmed in memory.
bool GameManager::isShopDataLoaded()
{
    if (gameModule != GameModule::Menu)
    {
        return false;
    }

    uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * 2);

    uint16_t shopType = read<uint16_t>(shopOffset + 0);
    if (shopType > 8) { return false; }

    uint8_t invCount = read<uint8_t>(shopOffset + 2);
    if (invCount == 0 || invCount > SHOP_ITEM_MAX) { return false; }

    uint8_t padding = read<uint8_t>(shopOffset + 3);
    if (padding != 0) { return false; }

    for (int i = invCount; i < SHOP_ITEM_MAX; ++i)
    {
        uintptr_t itemOffset = shopOffset + 4 + (i * 8);

        uint32_t itemType = read<uint32_t>(itemOffset + 0);
        uint16_t itemID = read<uint32_t>(itemOffset + 4);
        uint16_t itemPadding = read<uint16_t>(itemOffset + 6);

        // If we're outside the specified item count we should see all zeroes.
        if (itemType != 0 || itemID != 0 || itemPadding != 0)
        {
            return false;
        }
    }

    // Check some hardcoded prices to ensure the price table is loaded
    {
        uint32_t lastItemPrice = read<uint32_t>(ShopOffsets::PricesStart + (101 * 4));
        if (lastItemPrice != 50) { return false; }

        uint32_t lastWeaponPrice = read<uint32_t>(ShopOffsets::PricesStart + (255 * 4));
        if (lastWeaponPrice != 999999) { return false; }

        uint32_t lastArmorPrice = read<uint32_t>(ShopOffsets::PricesStart + (287 * 4));
        if (lastArmorPrice != 2) { return false; }

        uint32_t lastAccessoryPrice = read<uint32_t>(ShopOffsets::PricesStart + (317 * 4));
        if (lastAccessoryPrice != 10000) { return false; }

        uint32_t lastMateriaPrice = read<uint32_t>(ShopOffsets::MateriaPricesStart + (68 * 4));
        if (lastMateriaPrice != 9000) { return false; }
    }

    return true;
}

bool GameManager::isWorldDataLoaded()
{
    read(WorldOffsets::EncounterStart, 2048, (uint8_t*)worldMapEncounterTable);

    for (int r = 0; r < 16; ++r)
    {
        WorldMapEncounters& origEncounters = GameData::worldMapEncounters[r];

        for (int s = 0; s < 4; ++s)
        {
            std::vector<Encounter>& origEncSet = origEncounters.sets[s];
            if (origEncSet.size() == 0)
            {
                continue;
            }

            uintptr_t dataOffset = (r * 64) + (s * 16) + 1;

            for (int i = 0; i < 14; ++i)
            {
                Encounter& origEnc = origEncSet[i];
                Encounter& encData = worldMapEncounterTable[dataOffset + i];

                if (origEnc.raw != encData.raw)
                {
                    return false;
                }
            }
        }
    }

    // World is ready if we've hit peak fade out and started coming back down.
    uint8_t screenFade = read<uint8_t>(GameOffsets::WorldScreenFade);
    bool isScreenReady = (lastWorldScreenFade == 0xFF && screenFade < lastWorldScreenFade);
    lastWorldScreenFade = screenFade;

    return isScreenReady;
}

// The goal here is to find the message thats closest in memory (offset) that also contains
// the name of the item. The message is usually: Received "{itemName}"!
int GameManager::findPickUpMessage(std::string itemName, uint8_t group, uint8_t script, uint32_t offset)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return -1;
    }

    int bestIndex = -1;
    uint32_t bestDistance = UINT32_MAX;

    for (int i = 0; i < fieldData.messages.size(); ++i)
    {
        FieldScriptMessage& fieldMsg = fieldData.messages[i];

        // The message is always in the same group+script as the pick up.
        if (fieldMsg.group != group || fieldMsg.script != script)
        {
            continue;
        }

        std::string msg = readString(FieldScriptOffsets::ScriptStart + fieldMsg.strOffset, fieldMsg.strLength);
        if (msg.find(itemName) != std::string::npos)
        {
            uint32_t distance = std::abs((int32_t)(fieldMsg.offset - offset));
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }
    }

    return bestIndex;
}

std::string GameManager::getWindowText(uint8_t index)
{
    if (getGameModule() != GameModule::Field)
    {
        return "";
    }

    return readString(getWindowTextOffset(index), 256);
}

std::pair<BattleScene*, BattleFormation*> GameManager::getBattleFormation()
{
    if (getGameModule() != GameModule::Battle)
    {
        return { nullptr, nullptr };
    }

    uint16_t formationID = read<uint16_t>(BattleOffsets::FormationID);

    for (auto& [id, scene] : GameData::battleScenes) 
    {
        for (BattleFormation& formation : scene.formations)
        {
            if (formation.id == formationID)
            {
                return { &scene, &formation };
            }
        }
    }

    return { nullptr, nullptr };
}

template <typename T>
void multiplyStat(GameManager* game, uintptr_t offset, float multiplier)
{
    if (multiplier == 1.0f)
    {
        return;
    }

    T stat = game->read<T>(offset);
    stat = Utilities::clampTo<T>(stat * multiplier);
    game->write<T>(offset, stat);
}

void GameManager::applyBattleStatMultiplier(uintptr_t battleCharOffset, StatMultiplierSet& multiplierSet)
{
    if (getGameModule() != GameModule::Battle)
    {
        LOG("Could not apply battle stat multiplier outside of battle.");
        return;
    }

    multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::CurrentHP, multiplierSet.currentHP);
    multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::MaxHP, multiplierSet.maxHP);
    multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::CurrentMP, multiplierSet.currentMP);
    multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::MaxMP, multiplierSet.maxMP);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Strength, multiplierSet.strength);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Magic, multiplierSet.magic);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Evade, multiplierSet.evade);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Speed, multiplierSet.speed);
    multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Luck, multiplierSet.luck);
    multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::Defense, multiplierSet.defense);
    multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::MDefense, multiplierSet.mDefense);
}

void GameManager::applyBattleStatMultiplier(uintptr_t battleCharOffset, float multiplier, bool applyToHP, bool applyToMP, bool applyToStats)
{
    if (getGameModule() != GameModule::Battle)
    {
        LOG("Could not apply battle stat multiplier outside of battle.");
        return;
    }

    if (applyToHP)
    {
        multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::CurrentHP, multiplier);
        multiplyStat<uint32_t>(this, battleCharOffset + BattleOffsets::MaxHP, multiplier);
    }

    if (applyToMP)
    {
        multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::CurrentMP, multiplier);
        multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::MaxMP, multiplier);
    }

    if (applyToStats)
    {
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Strength, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Magic, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Evade, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Speed, multiplier);
        multiplyStat<uint8_t>(this, battleCharOffset + BattleOffsets::Luck, multiplier);
        multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::Defense, multiplier);
        multiplyStat<uint16_t>(this, battleCharOffset + BattleOffsets::MDefense, multiplier);
    }
}