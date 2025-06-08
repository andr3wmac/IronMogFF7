#include "RandomizeEnemyDrops.h"
#include "game/MemoryOffsets.h"

#include <iostream>

void RandomizeEnemyDrops::onEnable()
{
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeEnemyDrops::onFrame);
    BIND_EVENT(game->onBattleEnter, RandomizeEnemyDrops::onBattleEnter);
    BIND_EVENT(game->onBattleExit, RandomizeEnemyDrops::onBattleExit);
}

void RandomizeEnemyDrops::onDisable()
{

}

void RandomizeEnemyDrops::onFrame(uint32_t frameNumber)
{
    if (game->inBattle())
    {
        if (framesInBattle > 120)
        {

        }

        // Max 3 unique enemies per fight
        for (int i = 0; i < 3; ++i)
        {
            // Maximum of 4 item slots per enemy
            for (int j = 0; j < 4; ++j)
            {
                uint16_t dropID = game->read<uint16_t>(EnemyFormationOffsets::Enemies[i] + EnemyFormationOffsets::DropIDs[j]);

                // Empty slot
                if (dropID == 65535)
                {
                    continue;
                }

                std::pair<uint8_t, uint16_t> data = unpackDropID(dropID);
                if (data.first == DropType::Item)
                {
                    uint16_t newDropID = packDropID(DropType::Item, 0x16);
                    game->write<uint8_t>(EnemyFormationOffsets::Enemies[i] + EnemyFormationOffsets::DropRates[j], 0x3F);
                    game->write<uint16_t>(EnemyFormationOffsets::Enemies[i] + EnemyFormationOffsets::DropIDs[j], newDropID);
                }
            }
        }

        //uint16_t enemy1ItemID = game->read<uint16_t>(0xF5FD0);
        //std::cout << "Enemy 1 Item: " << enemy1ItemID << std::endl;

        /*for (int i = 0; i < 6; ++i)
        {
            uint16_t enemyID = game->read<uint16_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::ID);
            uint32_t enemyHP = game->read<uint32_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::CurrentHP);
            uint32_t enemyMaxHP = game->read<uint32_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::MaxHP);

            uint8_t enemySceneIdx = game->read<uint8_t>(BattleEnemyOffsets::EnemyObjBase + (i * 16));

            std::cout << i << ") ID: " << enemyID << " HP: " << enemyHP << "/" << enemyMaxHP << " Scene Idx: " << (int)enemySceneIdx << std::endl;

            uint8_t Level = game->read<uint8_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Level);
            uint8_t Strength = game->read<uint8_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Strength);
            uint8_t Evade = game->read<uint8_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Evade);
            uint8_t Speed = game->read<uint8_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Speed);
            uint8_t Luck = game->read<uint8_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Luck);

            //game->write<uint8_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Strength, 255);

            uint32_t Gil = game->read<uint32_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Gil);
            uint32_t Exp = game->read<uint32_t>(BattleEnemyOffsets::Enemies[i] + BattleEnemyOffsets::Exp);

            std::cout << "   Level: " << (int)Level << " Gil: " << Gil << " Exp: " << Exp << std::endl;
        }*/
    }
}

void RandomizeEnemyDrops::onBattleEnter()
{
    // Select a random item to replace each existing item with.
    std::cout << "Entered battle!" << std::endl;
    

}

void RandomizeEnemyDrops::onBattleExit()
{
    std::cout << "Exited battle!" << std::endl;
}