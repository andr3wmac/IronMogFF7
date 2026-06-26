#include "BattleModule.h"
#include "livemod/game/GameManager.h"
#include "livemod/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "rules/Restrictions.h"

void BattleModule::setup(GameManager* game)
{
    this->game = game;
    gameModule = game->getGameModule();

    BIND_EVENT_ONE_ARG(game->onModuleChanged, BattleModule::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onUpdate, BattleModule::onUpdate);
}

void BattleModule::onModuleChanged(uint8_t newGameModule)
{
    // Entered battle
    if (gameModule != GameModule::Battle && newGameModule == GameModule::Battle)
    {
        waitingForBattleData = true;
    }

    // Exited battle
    if (gameModule == GameModule::Battle && newGameModule != GameModule::Battle)
    {
        game->onBattleExit.invoke();
    }

    gameModule = newGameModule;
}

void BattleModule::onUpdate(bool justConnected)
{
    if (gameModule == GameModule::Battle)
    {
        // Ensure the battle data is ready before triggering the event.
        if (waitingForBattleData && isBattleDataLoaded())
        {
            onBattleEnter();
            waitingForBattleData = false;
            lastBattleFormation = game->read<uint16_t>(BattleOffsets::ActiveFormationID);
            LOG("Entered battle formation %d", lastBattleFormation);
        }

        // Detect if the active formation changed due to a battle transition
        uint16_t currentFormation = game->read<uint16_t>(BattleOffsets::ActiveFormationID);
        if (!waitingForBattleData && currentFormation != lastBattleFormation)
        {
            waitingForFormation = true;
            lastBattleFormation = currentFormation;
        }

        // Ensure the formation data is ready before triggering the event.
        if (waitingForFormation && isFormationLoaded(currentFormation))
        {
            onBattleTransition(currentFormation);
            waitingForFormation = false;
            LOG("Battle transitioned to formation %d", lastBattleFormation, currentFormation);
        }
    }
}

void BattleModule::onBattleEnter()
{
    // First trigger anything else that might modify battles
    game->onBattleEnter.invoke();

    // Now delete any banned drops/steals.
    deleteBannedDrops();
}

void BattleModule::onBattleTransition(uint16_t formation)
{
    // First trigger anything else that might modify battles
    game->onBattleTransition.invoke(formation);

    // Now delete any banned drops/steals.
    deleteBannedDrops();
}

void BattleModule::deleteBannedDrops()
{
    const auto& [scene, formation] = game->getBattleFormation();

    std::set<int> activeEnemyIndexes;
    for (int i = 0; i < 6; ++i)
    {
        if (formation->enemyIDs[i] == UINT16_MAX)
        {
            continue;
        }

        for (int j = 0; j < 3; ++j)
        {
            if (formation->enemyIDs[i] == scene->enemyIDs[j])
            {
                activeEnemyIndexes.insert(j);
            }
        }
    }

    for (int idx : activeEnemyIndexes)
    {
        // Maximum of 4 item slots per enemy
        for (int i = 0; i < 4; ++i)
        {
            uint16_t dropID = game->read<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::DropIDs[i]);
            if (dropID == UINT16_MAX)
            {
                continue;
            }

            if (Restrictions::isItemBanned(dropID))
            {
                game->write<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::DropIDs[i], UINT16_MAX);
                LOG("Deleted banned item from enemy drops: %d", dropID);
            }
        }

        uint16_t morphID = game->read<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::MorphItemID);
        if (morphID != UINT16_MAX)
        {
            if (Restrictions::isItemBanned(morphID))
            {
                game->write<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::MorphItemID, UINT16_MAX);
                LOG("Deleted banned item from enemy morph: %d", morphID);
            }
        }
    }
}

bool BattleModule::inBattle()
{
    return gameModule == GameModule::Battle && !waitingForBattleData;
}

// When we switch to the battle module the actual data for the battle isn't fully loaded
// so we can't time events to overwrite it. We introduce some heuristics to try to ensure
// the data is fully loaded before we let anything modify it.
bool BattleModule::isBattleDataLoaded()
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
        std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
        for (int i = 0; i < 3; ++i)
        {
            uint8_t id = partyIDs[i];
            if (id == 0xFF)
            {
                verifiedPlayers++;
                continue;
            }

            uintptr_t characterOffset = getCharacterDataOffset(id);
            uint16_t worldMaxHP = game->read<uint16_t>(characterOffset + CharacterDataOffsets::MaxHP);
            uint16_t battleMaxHP = game->read<uint16_t>(BattleOffsets::Allies[i] + BattleOffsets::MaxHP);

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
                uint16_t dropID = game->read<uint16_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::DropIDs[j]);

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

bool BattleModule::isFormationLoaded(uint16_t formationID)
{
    if (gameModule != GameModule::Battle)
    {
        return false;
    }

    const auto& [scene, formation] = game->getBattleFormation(formationID);
    if (formation == nullptr)
    {
        return false;
    }

    for (int i = 0; i < 6; ++i)
    {
        if (formation->enemyIDs[i] == 0xFFFF)
        {
            continue;
        }

        uint16_t currentID = game->read<uint16_t>(BattleOffsets::Enemies[i] + BattleOffsets::EnemyID);
        if (currentID != formation->enemyIDs[i])
        {
            return false;
        }
    }

    return true;
}