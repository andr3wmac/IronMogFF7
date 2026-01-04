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
    ImGui::Checkbox("Disable Shops", &disableShops);
    ImGui::SetItemTooltip("Completely disables all shops.");

    ImGui::BeginDisabled(disableShops);
    {
        ImGui::Checkbox("Keep Prices", &keepPrices);
        ImGui::SetItemTooltip("Keep prices the same as the original shop.");
    }
    ImGui::EndDisabled();
}

void RandomizeShops::onDebugGUI()
{
    /*if (game->getGameModule() != GameModule::Menu)
    {
        return;
    }

    uint8_t menuType = game->read<uint8_t>(GameOffsets::MenuType);
    if (menuType != MenuType::Shop)
    {
        return;
    }*/

    FieldData fieldData = GameData::getField(lastFieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    std::set<uint8_t> displayedShopIDs;
    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        uint8_t shopID = fieldData.shops[i].shopID;
        if (displayedShopIDs.count(shopID) > 0)
        {
            continue;
        }

        uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * shopID);
        uint8_t invCount = game->read<uint8_t>(shopOffset + 2);

        if (invCount > SHOP_ITEM_MAX)
        {
            continue;
        }

        std::string shopText = "Shop " + std::to_string(shopID);
        ImGui::SeparatorText(shopText.c_str());

        for (int j = 0; j < invCount; ++j)
        {
            uintptr_t itemOffset = shopOffset + 4 + (j * 8);
            uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
            uint16_t itemID = game->read<uint16_t>(itemOffset + 4);

            // Materia
            if (itemType == 1)
            {
                uint32_t price = game->read<uint32_t>(ShopOffsets::MateriaPricesStart + (itemID * 4));
                std::string materiaName = GameData::getMateriaName((uint8_t)itemID);

                std::string debugText = "Materia: " + materiaName + " (" + std::to_string(price) + ")";
                ImGui::Text(debugText.c_str());
            }
            // Item
            else
            {
                uint32_t price = game->read<uint32_t>(ShopOffsets::PricesStart + (itemID * 4));
                std::string itemName = GameData::getNameFromFieldScriptID(itemID);

                std::string debugText = "Materia: " + itemName + " (" + std::to_string(price) + ")";
                ImGui::Text(debugText.c_str());
            }
        }

        displayedShopIDs.insert(shopID);
    }
}

void RandomizeShops::onFieldChanged(uint16_t fieldID)
{
    FieldData fieldData = GameData::getField(fieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    lastFieldID = fieldID;

    // Disable shops
    if (disableShops)
    {
        for (int i = 0; i < fieldData.shops.size(); ++i)
        {
            // Overwrite the MENU opcode and parameters with 0x5F which is NOP
            game->write<uint32_t>(FieldScriptOffsets::ScriptStart + fieldData.shops[i].offset, 0x5F5F5F5F);
        }
    }
}

struct ShopEntry
{
    uintptr_t offset;
    uint16_t id;
    uint32_t price;
};

void RandomizeShops::onShopOpened()
{
    FieldData fieldData = GameData::getField(lastFieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    // There can be multiple entries for different shop IDs, we don't want
    // to randomize them multiple times.
    std::set<uint8_t> randomizedShopIDs;

    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        uint8_t shopID = fieldData.shops[i].shopID;
        if (randomizedShopIDs.count(shopID) > 0)
        {
            continue;
        }

        rng.seed(Utilities::makeKey(game->getSeed(), lastFieldID, shopID));

        uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * shopID);
        uint8_t invCount = game->read<uint8_t>(shopOffset + 2);

        std::vector<ShopEntry> shopItems;
        std::vector<ShopEntry> shopMateria;

        for (int j = 0; j < invCount; ++j)
        {
            uintptr_t itemOffset = shopOffset + 4 + (j * 8);
            uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
            uint16_t itemID = game->read<uint16_t>(itemOffset + 4);

            // Materia
            if (itemType == 1)
            {
                uint32_t price = game->read<uint32_t>(ShopOffsets::MateriaPricesStart + (itemID * 4));
                shopMateria.push_back({ itemOffset, itemID, price });
            }
            // Item
            else 
            {
                uint32_t price = game->read<uint32_t>(ShopOffsets::PricesStart + (itemID * 4));
                shopItems.push_back({ itemOffset, itemID, price });
            }
        }

        // Sort by lowest prices first
        std::sort(shopItems.begin(), shopItems.end(),
            [](const ShopEntry& a, const ShopEntry& b)
            {
                return a.price < b.price;
            });

        std::sort(shopMateria.begin(), shopMateria.end(),
            [](const ShopEntry& a, const ShopEntry& b)
            {
                return a.price < b.price;
            });

        // Select new random items
        std::vector<ShopEntry> newShopItems;
        {
            std::set<uint16_t> chosenItems;

            for (int j = 0; j < shopItems.size(); ++j)
            {
                uint16_t newItemID = randomizeShopItem(shopItems[j].id, chosenItems);
                uint32_t price = game->read<uint32_t>(ShopOffsets::PricesStart + (newItemID * 4));
                if (price <= 2)
                {
                    price = price * 20000;
                }
                newShopItems.push_back({ 0, newItemID, price });
                chosenItems.insert(newItemID);
            }

            // Sort by lowest prices first
            std::sort(newShopItems.begin(), newShopItems.end(),
                [](const ShopEntry& a, const ShopEntry& b)
                {
                    return a.price < b.price;
                });
        }
        std::vector<ShopEntry> newShopMateria;
        {
            std::set<uint16_t> chosenMateria;

            for (int j = 0; j < shopMateria.size(); ++j)
            {
                uint16_t newMateriaID = randomizeShopMateria(shopMateria[j].id, chosenMateria);
                uint32_t price = game->read<uint32_t>(ShopOffsets::PricesStart + (newMateriaID * 4));
                if (price <= 2)
                {
                    price = price * 20000;
                }
                newShopMateria.push_back({ 0, newMateriaID, price });
                chosenMateria.insert(newMateriaID);
            }

            // Sort by lowest prices first
            std::sort(newShopMateria.begin(), newShopMateria.end(),
                [](const ShopEntry& a, const ShopEntry& b)
                {
                    return a.price < b.price;
                });
        }

        // We match up the old items to the new ones, both lists have been sorted by price
        // so this gives us a reasonable match up between values.
        for (int j = 0; j < shopItems.size(); ++j)
        {
            ShopEntry& origItem = shopItems[j];
            ShopEntry& newItem = newShopItems[j];

            game->write<uint16_t>(origItem.offset + 4, newItem.id);
            if (keepPrices)
            {
                // We reuse the existing price for the randomized item by overwriting the value with the original.
                game->write<uint32_t>(ShopOffsets::PricesStart + (newItem.id * 4), origItem.price);
            }

            std::string oldItemName = GameData::getNameFromFieldScriptID((uint8_t)origItem.id);
            std::string newItemName = GameData::getNameFromFieldScriptID((uint8_t)newItem.id);
            LOG("Randomized item in shop %d: %s changed to: %s", shopID, oldItemName.c_str(), newItemName.c_str());
        }
        for (int j = 0; j < shopMateria.size(); ++j)
        {
            ShopEntry& origMat = shopMateria[j];
            ShopEntry& newMat = newShopMateria[j];

            game->write<uint16_t>(origMat.offset + 4, newMat.id);
            if (keepPrices)
            {
                // We reuse the existing price for the randomized item by overwriting the value with the original.
                game->write<uint32_t>(ShopOffsets::MateriaPricesStart + (newMat.id * 4), origMat.price);
            }

            std::string oldMateriaName = GameData::getMateriaName((uint8_t)origMat.id);
            std::string newMateriaName = GameData::getMateriaName((uint8_t)newMat.id);
            LOG("Randomized materia in shop %d: %s changed to: %s", shopID, oldMateriaName.c_str(), newMateriaName.c_str());
        }

        randomizedShopIDs.insert(shopID);
    }
}

uint16_t RandomizeShops::randomizeShopItem(uint16_t itemID)
{
    uint16_t selectedID = itemID;

    // Item
    if (itemID < 128)
    {
        selectedID = GameData::getRandomItem(rng);
    }
    // Weapon
    else if (itemID < 256)
    {
        selectedID = 128 + GameData::getRandomWeapon(rng);
    }
    // Armor
    else if (itemID < 288)
    {
        selectedID = 256 + GameData::getRandomArmor(rng);
    }
    // Accessory
    else
    {
        selectedID = 288 + GameData::getRandomAccessory(rng);
    }

    return selectedID;
}

uint16_t RandomizeShops::randomizeShopItem(uint16_t itemID, std::set<uint16_t> previouslyChosen)
{
    while (true)
    {
        uint16_t chosen = randomizeShopItem(itemID);
        if (previouslyChosen.count(chosen) == 0)
        {
            return chosen;
        }
    }

    return 0;
}

uint16_t RandomizeShops::randomizeShopMateria(uint16_t materiaID)
{
    return GameData::getRandomMateria(rng);
}

uint16_t RandomizeShops::randomizeShopMateria(uint16_t materiaID, std::set<uint16_t> previouslyChosen)
{
    while (true)
    {
        uint16_t chosen = randomizeShopMateria(materiaID);
        if (previouslyChosen.count(chosen) == 0)
        {
            return chosen;
        }
    }

    return 0;
}