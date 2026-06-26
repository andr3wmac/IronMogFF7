#include "Restrictions.h"
#include "livemod/game/GameData.h"
#include "livemod/game/MemoryOffsets.h"
#include "livemod/utilities/Logging.h"
#include <set>

std::set<uint16_t> bannedItems;
std::set<uint16_t> bannedMateria;

void Restrictions::reset()
{
    bannedItems.clear(); 
    bannedMateria.clear();
}

void Restrictions::banItem(uint16_t id)
{
    bannedItems.insert(id);
}

bool Restrictions::isItemBanned(uint16_t itemID)
{
    return (bannedItems.count(itemID) > 0);
}

void Restrictions::banMateria(uint16_t materiaID)
{
    bannedMateria.insert(materiaID);
}

bool Restrictions::isMateriaBanned(uint16_t materiaID)
{
    return (bannedMateria.count(materiaID) > 0);
}

void Restrictions::enforceBattleBans(GameManager* game)
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

            if (isItemBanned(dropID))
            {
                game->write<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::DropIDs[i], UINT16_MAX);
                LOG("Deleted banned item from enemy drops: %d", dropID);
            }
        }

        uint16_t morphID = game->read<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::MorphItemID);
        if (morphID != UINT16_MAX)
        {
            if (isItemBanned(morphID))
            {
                game->write<uint16_t>(BattleSceneOffsets::Enemies[idx] + BattleSceneOffsets::MorphItemID, UINT16_MAX);
                LOG("Deleted banned item from enemy morph: %d", morphID);
            }
        }
    }
}

void Restrictions::enforceFieldBans(GameManager* game, uint16_t fieldID)
{
    // Check each field item for something thats banned. This way if it wasn't
    // replaced by a randomizer we delete it to enforce the ban.

    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    for (int i = 0; i < fieldData.items.size(); ++i)
    {
        FieldScriptItem& item = fieldData.items[i];
        uintptr_t itemIDOffset = FieldScriptOffsets::ScriptStart + item.offset + FieldScriptOffsets::ItemID;
        uint16_t itemID = game->read<uint16_t>(itemIDOffset);

        // Do not delete Battery in Wall Market.
        if (fieldData.id == 196 && itemID == 85)
        {
            continue;
        }

        if (isItemBanned(itemID))
        {
            // Delete item.
            for (int b = 0; b < 5; ++b)
            {
                game->write<uint8_t>(FieldScriptOffsets::ScriptStart + item.offset + b, 0x5F);
            }

            // Replace the popup message with "Nothing"
            std::string itemName = GameData::getItemName(itemID);
            int msgIndex = game->field.findPickUpMessage(itemName, item.group, item.script, item.offset);
            if (msgIndex >= 0)
            {
                game->field.overwriteMessage(msgIndex, "Nothing");
            }

            LOG("Deleted banned item: %s %d", itemName.c_str(), itemID);
        }
    }

    for (int i = 0; i < fieldData.materia.size(); ++i)
    {
        FieldScriptItem& materia = fieldData.materia[i];
        uintptr_t idOffset = FieldScriptOffsets::ScriptStart + materia.offset + FieldScriptOffsets::MateriaID;
        uint8_t materiaID = game->read<uint8_t>(idOffset);

        if (isMateriaBanned(materiaID))
        {
            // Delete materia.
            for (int b = 0; b < 7; ++b)
            {
                game->write<uint8_t>(FieldScriptOffsets::ScriptStart + materia.offset + b, 0x5F);
            }

            // Replace the popup message with "Nothing"
            std::string materiaName = GameData::getItemName(materiaID);
            int msgIndex = game->field.findPickUpMessage(materiaName, materia.group, materia.script, materia.offset);
            if (msgIndex >= 0)
            {
                game->field.overwriteMessage(msgIndex, "Nothing");
            }

            LOG("Deleted banned materia: %d", materiaID);
        }
    }
}

void Restrictions::enforceShopBans(GameManager* game, uint8_t menuIndex)
{
    // Buy menu
    if (menuIndex != 0)
    {
        return;
    }

    // Check for banned items.
    FieldData fieldData = GameData::getField(game->getFieldID());
    if (!fieldData.isValid())
    {
        return;
    }

    std::set<uint8_t> checkedShopIDs;
    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        uint8_t shopID = fieldData.shops[i].shopID;
        if (checkedShopIDs.count(shopID) > 0)
        {
            continue;
        }

        uintptr_t shopOffset = ShopOffsets::ShopStart + (ShopOffsets::ShopStride * shopID);
        uint8_t invCount = game->read<uint8_t>(shopOffset + 2);

        if (invCount > SHOP_ITEM_MAX)
        {
            continue;
        }

        for (int j = 0; j < invCount; ++j)
        {
            uintptr_t itemOffset = shopOffset + 4 + (j * 8);
            uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
            uint16_t itemID = game->read<uint16_t>(itemOffset + 4);

            bool shouldDelete = false;

            // Item
            if (itemType == 0)
            {
                if (isItemBanned(itemID))
                {
                    shouldDelete = true;
                }
            }
            // Materia
            else if (itemType == 1)
            {
                if (isMateriaBanned(itemID))
                {
                    shouldDelete = true;
                }
            }

            if (shouldDelete)
            {
                // By writing an item type of 2 in the game makes the slot invalid.
                game->write<uint32_t>(itemOffset + 0, 2);
            }
        }

        checkedShopIDs.insert(shopID);
    }
}