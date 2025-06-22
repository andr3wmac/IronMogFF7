#include "GameManager.h"
#include "game/MemoryOffsets.h"
#include "rules/Rule.h"

#include <thread>
#include <chrono>

GameManager::GameManager()
    : emulator(nullptr)
{

}

GameManager::~GameManager()
{
    for (int i = 0; i < rules.size(); ++i)
    {
        delete rules[i];
    }
    rules.clear();

    if (emulator != nullptr)
    {
        delete emulator;
    }
}

bool GameManager::attachToEmulator(std::string processName)
{
    emulator = Emulator::getEmulatorFromProcessName(processName);
    if (emulator == nullptr)
    {
        return false;
    }

    return emulator->attach(processName);
}

bool GameManager::attachToEmulator(std::string processName, uintptr_t memoryAddress)
{
    emulator = Emulator::getEmulatorCustom(processName, memoryAddress);
    if (emulator == nullptr)
    {
        return false;
    }

    return emulator->attach(processName);
}

void GameManager::addRule(Rule* rule)
{
    rules.push_back(rule);
    rule->setManager(this);
}

void GameManager::start(uint32_t inputSeed)
{
    // Try to load seed from the game data
    // if its not found then use input seed and write it.

    seed = inputSeed;

    for (int i = 0; i < rules.size(); ++i)
    {
        rules[i]->onStart();
    }
}

void GameManager::update()
{
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
            onBattleExit.Invoke();
        }

        // Game module changed.
        gameModule = newGameModule;
    }

    if (gameModule == GameModule::Battle)
    {
        if (waitingForBattleData && isBattleDataLoaded())
        {
            onBattleEnter.Invoke();
            waitingForBattleData = false;
        }
    }

    if (gameModule == GameModule::Field)
    {
        uint16_t newFieldID = read<uint16_t>(GameOffsets::FieldID);
        if (newFieldID != fieldID)
        {
            onFieldChanged.Invoke(newFieldID);
            fieldID = newFieldID;
        }
    }

    uint32_t newFrameNumber = read<uint32_t>(GameOffsets::FrameNumber);
    if (newFrameNumber != frameNumber)
    {
        frameNumber = newFrameNumber;
        onFrame.Invoke(newFrameNumber);
    }
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

bool GameManager::inBattle()
{
    return gameModule == GameModule::Battle && !waitingForBattleData;
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
            uint16_t worldHP = read<uint16_t>(characterOffset + CharacterDataOffsets::CurrentHP);
            uint16_t battleHP = read<uint16_t>(BattleCharacterOffsets::Allies[i] + BattleCharacterOffsets::CurrentHP);

            if (worldHP == battleHP)
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
                uint16_t dropID = read<uint16_t>(EnemyFormationOffsets::Enemies[i] + EnemyFormationOffsets::DropIDs[j]);

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