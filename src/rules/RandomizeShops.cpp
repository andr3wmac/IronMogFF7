#include "RandomizeShops.h"
#include "core/gui/GUI.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "rules/Restrictions.h"

#include <imgui.h>

REGISTER_RULE("Randomize Shops", RandomizeShops)

void RandomizeShops::setup()
{
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeShops::onFieldChanged);
    BIND_EVENT(game->onShopOpened, RandomizeShops::onShopOpened);
}

void RandomizeShops::onSettingsGUI()
{
    ImGui::Checkbox("Keep Prices", &keepPrices);
    ImGui::SetItemTooltip("Keep shop prices the same as the original items.");
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

    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        rng.seed(Utilities::makeKey(game->getSeed(), lastFieldID, i));

        uint8_t shopID = fieldData.shops[i].shopID;
        uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * shopID);
        uint8_t invCount = game->read<uint8_t>(shopOffset + 2);

        for (int j = 0; j < invCount; ++j)
        {
            uintptr_t itemOffset = shopOffset + 4 + (j * 8);
            uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
            uint16_t itemID = game->read<uint16_t>(itemOffset + 4);

            // Price offset is different based on item or materia.
            uintptr_t priceOffset = 0;
            uint32_t newPrice = 0;
            uint16_t newItemID = itemID;

            // Materia
            if (itemType == 1)
            {
                newItemID = randomizeShopMateria(itemID);

                std::string oldMateriaName = GameData::getMateriaName((uint8_t)itemID);
                std::string newMateriaName = GameData::getMateriaName((uint8_t)newItemID);
                LOG("Randomized materia in shop %d: %s changed to: %s", shopID, oldMateriaName.c_str(), newMateriaName.c_str());

                priceOffset = ShopOffsets::MateriaPricesStart + (newItemID * 4);
                newPrice = game->read<uint32_t>(ShopOffsets::MateriaPricesStart + (itemID * 4));
            }
            else
            {
                newItemID = randomizeShopItem(itemID);

                std::string oldItemName = GameData::getNameFromFieldScriptID(itemID);
                std::string newItemName = GameData::getNameFromFieldScriptID(newItemID);
                LOG("Randomized item in shop %d: %s changed to: %s", shopID, oldItemName.c_str(), newItemName.c_str());

                priceOffset = ShopOffsets::PricesStart + (newItemID * 4);
                newPrice = game->read<uint32_t>(ShopOffsets::PricesStart + (itemID * 4));
            }

            if (priceOffset > 0)
            {
                game->write<uint16_t>(itemOffset + 4, newItemID);

                if (keepPrices)
                {
                    // We reuse the existing price for the randomized item by overwriting the value with the original.
                    game->write<uint32_t>(priceOffset, newPrice);
                }
            }
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
    return GameData::getRandomMateria(rng);
}