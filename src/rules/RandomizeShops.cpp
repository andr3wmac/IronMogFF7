#include "RandomizeShops.h"
#include "game/GameData.h"
#include "game/MemoryOffsets.h"
#include "rules/Restrictions.h"
#include "utilities/Logging.h"

REGISTER_RULE("Randomize Shops", RandomizeShops)

void RandomizeShops::setup()
{
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeShops::onFieldChanged);
    BIND_EVENT(game->onShopOpened, RandomizeShops::onShopOpened);
}

uint64_t makeKey(uint32_t seed, uint16_t fieldID, uint8_t index)
{
    uint32_t fieldKey = (uint32_t(fieldID) << 16 | index);
    uint64_t combined = (uint64_t(fieldKey) << 32) | seed;

    // Mix bits to spread entropy
    combined ^= combined >> 33;
    combined *= 0xff51afd7ed558ccd;
    combined ^= combined >> 33;
    combined *= 0xc4ceb9fe1a85ec53;
    combined ^= combined >> 33;

    return combined;
}

void RandomizeShops::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    lastFieldID = fieldID;
}

void RandomizeShops::onShopOpened()
{
    FieldData fieldData = GameData::getField(lastFieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    /*
        Shop Iventory Count = 0x1D4716
        Padding = 0x1D4717 should equal 0

        Each entry:
         4 bytes for type, seems like 1 = materia, everything else?
         2 bytes for index, follows same storage pattern as other items
         2 bytes for padding, seems like it should always be zero?

        Max 10 entries, looks like the memory is all zeroes in the empty slots. Total 84 bytes used for shop definition.
    */

    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        rng.seed(makeKey(game->getSeed(), lastFieldID, i));

        uint8_t shopID = fieldData.shops[i].shopID;
        uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * shopID);
        uint8_t invCount = game->read<uint8_t>(shopOffset + 2);

        for (int j = 0; j < invCount; ++j)
        {
            uintptr_t itemOffset = shopOffset + 4 + (j * 8);
            uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
            uint16_t itemID = game->read<uint16_t>(itemOffset + 4);

            uint16_t newItemID = itemID;
            if (itemType == 1)
            {
                newItemID = randomizeShopMateria(itemID);

                std::string oldMateriaName = GameData::getMateriaName((uint8_t)itemID);
                std::string newMateriaName = GameData::getMateriaName((uint8_t)newItemID);
                LOG("Randomized materia in shop %d: %s changed to: %s", shopID, oldMateriaName.c_str(), newMateriaName.c_str());
            }
            else
            {
                newItemID = randomizeShopItem(itemID);

                std::string oldItemName = GameData::getNameFromFieldScriptID(itemID);
                std::string newItemName = GameData::getNameFromFieldScriptID(newItemID);
                LOG("Randomized item in shop %d: %s changed to: %s", shopID, oldItemName.c_str(), newItemName.c_str());
            }

            game->write<uint16_t>(itemOffset + 4, newItemID);
        }
    }
}

uint16_t RandomizeShops::randomizeShopItem(uint16_t itemID)
{
    uint16_t selectedID = itemID;

    // Loop in case we select a bad ID.
    while (true)
    {
        // Item
        if (itemID < 128)
        {
            selectedID = GameData::getRandomItem(rng);
            break;
        }
        // Weapon
        else if (itemID < 256)
        {
            selectedID = 128 + GameData::getRandomWeapon(rng);
            break;
        }
        // Armor
        else if (itemID < 288)
        {
            selectedID = 256 + GameData::getRandomArmor(rng);
            break;
        }
        // Accessory
        else
        {
            selectedID = 288 + GameData::getRandomAccessory(rng);
            break;
        }
    }

    return selectedID;
}

uint16_t RandomizeShops::randomizeShopMateria(uint16_t materiaID)
{
    uint16_t selectedID = materiaID;

    while (true)
    {
        selectedID = GameData::getRandomMateria(rng);
        if (!Restrictions::isMateriaBanned((uint8_t)selectedID))
        {
            break;
        }
        else 
        {
            LOG("Skipped banned materia: %d", selectedID);
        }
    }

    return selectedID;
}