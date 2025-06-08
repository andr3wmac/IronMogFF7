#include "GameManager.h"
#include "game/MemoryOffsets.h"
#include "features/Feature.h"

#include <thread>
#include <chrono>

GameManager::GameManager()
    : emulator(nullptr)
{

}

GameManager::~GameManager()
{
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

void GameManager::addFeature(Feature* feature)
{
    features.push_back(feature);
    feature->setManager(this);
}

void GameManager::run()
{
    for (int i = 0; i < features.size(); ++i)
    {
        features[i]->onEnable();
    }

    while (true)
    {
        uint8_t newGameModule = read<uint8_t>(GameOffsets::CurrentModule);
        if (newGameModule != gameModule)
        {
            // Entered battle
            if (gameModule != GameModule::Battle && newGameModule == GameModule::Battle)
            {
                onBattleEnter.Invoke();
            }

            // Exited battle
            if (gameModule == GameModule::Battle && newGameModule != GameModule::Battle)
            {
                onBattleEnter.Invoke();
            }

            // Game module changed.
            gameModule = newGameModule;
        }

        uint32_t newFrameNumber = read<uint32_t>(GameOffsets::FrameNumber);
        if (newFrameNumber != frameNumber)
        {
            frameNumber = newFrameNumber;
            onFrame.Invoke(newFrameNumber);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    return gameModule == GameModule::Battle;
}