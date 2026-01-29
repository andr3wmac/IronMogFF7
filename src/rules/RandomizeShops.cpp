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
    BIND_EVENT(game->onStart, RandomizeShops::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, RandomizeShops::onFieldChanged);
    BIND_EVENT(game->onShopOpened, RandomizeShops::onShopOpened);
    BIND_EVENT_ONE_ARG(game->onFrame, RandomizeShops::onFrame);
}

bool RandomizeShops::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Disable Shops", &disableShops);
    ImGui::SetItemTooltip("Completely disables all shops.");

    ImGui::BeginDisabled(disableShops);
    {
        changed |= ImGui::Checkbox("Keep Prices", &keepPrices);
        ImGui::SetItemTooltip("Keep prices the same as the original shop.");

        changed |= ImGui::Checkbox("Exclude Rare Items", &excludeRareItems);
        ImGui::SetItemTooltip("Items and materia which are not intended to be bought and\nsold (1 gil price) will not be included in shop randomization.");
    }
    ImGui::EndDisabled();

    return changed;
}

void RandomizeShops::loadSettings(const ConfigFile& cfg)
{
    disableShops     = cfg.get<bool>("disableShops", false);
    keepPrices       = cfg.get<bool>("keepPrices", true);
    excludeRareItems = cfg.get<bool>("excludeRareItems", false);
}

void RandomizeShops::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("disableShops", disableShops);
    cfg.set<bool>("keepPrices", keepPrices);
    cfg.set<bool>("excludeRareItems", excludeRareItems);
}

void RandomizeShops::onDebugGUI()
{
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

        uintptr_t shopOffset = ShopOffsets::ShopStart + (ShopOffsets::ShopStride * shopID);
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
                std::string itemName = GameData::getItemName(itemID);

                std::string debugText = "Materia: " + itemName + " (" + std::to_string(price) + ")";
                ImGui::Text(debugText.c_str());
            }
        }

        displayedShopIDs.insert(shopID);
    }
}

void RandomizeShops::onStart()
{
    generateRandomizedShops();
}

void RandomizeShops::generateRandomizedShops()
{
    randomizedShops.clear();

    // Sell Prices are initially populated with the original item values and
    // then will be reduced if any shop randomizes them to a lower price. This
    // prevents infinite money glitches from being possible.

    for (uint16_t i = 0; i < 320; ++i)
    {
        itemSellPrices[i] = GameData::getItemPrice(i);
    }

    for (uint8_t i = 0; i < 91; ++i)
    {
        materiaSellPrices[i] = GameData::getMateriaPrice(i);
    }

    // Below we randomize each shops items/materia. There is an extra step thats done
    // where we order the choices so that the highest value randomly selected item is 
    // assigned to the highest value original item. When Keep Prices is enabled this
    // adds a minor degree of balance.

    for (const auto& [fieldID, fieldData] : GameData::fieldData)
    {
        for (int i = 0; i < fieldData.shops.size(); ++i)
        {
            uint8_t shopID = fieldData.shops[i].shopID;
            if (randomizedShops.count(shopID) > 0)
            {
                continue;
            }

            rng.seed(Utilities::makeSeed64(game->getSeed(), fieldID, shopID));

            const Shop& shop = GameData::shops[shopID];
            RandomizedShop& randomizedShop = randomizedShops[shopID];
            uintptr_t shopOffset = ShopOffsets::ShopStart + (ShopOffsets::ShopStride * shopID);

            for (int j = 0; j < shop.items.size(); ++j)
            {
                uintptr_t itemOffset = shopOffset + 4 + (shop.items[j].index * 8);
                uint16_t itemID = shop.items[j].id;
                uint32_t price = GameData::getItemPrice(itemID);

                randomizedShop.items.push_back({ itemOffset, itemID, price });
            }
            for (int j = 0; j < shop.materia.size(); ++j)
            {
                uintptr_t materiaOffset = shopOffset + 4 + (shop.materia[j].index * 8);
                uint8_t materiaID = (uint8_t)shop.materia[j].id;
                uint32_t price = GameData::getMateriaPrice(materiaID);

                randomizedShop.materia.push_back({ materiaOffset, materiaID, price });
            }

            // Sort by lowest prices first
            std::sort(randomizedShop.items.begin(), randomizedShop.items.end(),
                [](const RandomizedShopItem& a, const RandomizedShopItem& b)
                {
                    return a.price < b.price;
                });

            std::sort(randomizedShop.materia.begin(), randomizedShop.materia.end(),
                [](const RandomizedShopItem& a, const RandomizedShopItem& b)
                {
                    return a.price < b.price;
                });

            // Select new random items
            {
                std::set<uint16_t> chosenItems;

                for (int j = 0; j < randomizedShop.items.size(); ++j)
                {
                    uint16_t oldItemID = randomizedShop.items[j].id;
                    uint32_t oldPrice = randomizedShop.items[j].price;
                    uint16_t newItemID = randomizeShopItem(oldItemID, chosenItems);

                    uint32_t price = GameData::getItemPrice(newItemID);
                    if (price <= 2)
                    {
                        price = price * 20000;
                    }
                    randomizedShop.newItems.push_back({ 0, newItemID, price });
                    chosenItems.insert(newItemID);

                    // We want the item to always sell for the lowest price its obtainable for.
                    itemSellPrices[newItemID] = std::min(itemSellPrices[newItemID], oldPrice);
                }

                // Sort by lowest prices first
                std::sort(randomizedShop.newItems.begin(), randomizedShop.newItems.end(),
                    [](const RandomizedShopItem& a, const RandomizedShopItem& b)
                    {
                        return a.price < b.price;
                    });
            }

            // Select new random materia
            {
                std::set<uint16_t> chosenMateria;

                for (int j = 0; j < randomizedShop.materia.size(); ++j)
                {
                    uint16_t oldMateriaID = randomizedShop.materia[j].id;
                    uint32_t oldPrice = randomizedShop.materia[j].price;
                    uint8_t newMateriaID = (uint8_t)randomizeShopMateria(oldMateriaID, chosenMateria);

                    uint32_t price = GameData::getMateriaPrice(newMateriaID);
                    if (price <= 1)
                    {
                        price = price * 20000;
                    }
                    randomizedShop.newMateria.push_back({ 0, newMateriaID, price });
                    chosenMateria.insert(newMateriaID);

                    // We want the materia to always sell for the lowest price its obtainable for.
                    materiaSellPrices[newMateriaID] = std::min(materiaSellPrices[newMateriaID], oldPrice);
                }

                // Sort by lowest prices first
                std::sort(randomizedShop.newMateria.begin(), randomizedShop.newMateria.end(),
                    [](const RandomizedShopItem& a, const RandomizedShopItem& b)
                    {
                        return a.price < b.price;
                    });
            }
        }
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
    shopOpen = false;
    shopMenuIndex = -1;

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

void RandomizeShops::onShopOpened()
{
    FieldData fieldData = GameData::getField(lastFieldID);
    if (!fieldData.isValid())
    {
        return;
    }

    fieldShopIDs.clear();

    for (int i = 0; i < fieldData.shops.size(); ++i)
    {
        uint8_t shopID = fieldData.shops[i].shopID;
        if (fieldShopIDs.count(shopID) > 0)
        {
            continue;
        }

        if (randomizedShops.count(shopID) == 0)
        {
            LOG("No randomized shop data found for shop ID: %d", shopID);
            continue;
        }

        fieldShopIDs.insert(shopID);
    }

    shopOpen = true;
}

void RandomizeShops::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Menu)
    {
        shopMenuIndex = -1;
        shopOpen = false;
        return;
    }

    if (!shopOpen)
    {
        return;
    }

    uint8_t menuIdx = game->read<uint8_t>(ShopOffsets::MenuIndex);
    if (shopMenuIndex != menuIdx)
    {
        // Buy Menu
        if (menuIdx == 0)
        {
            // Apply randomization
            for (uint8_t shopID : fieldShopIDs)
            {
                const RandomizedShop& shop = randomizedShops[shopID];

                // We match up the old items to the new ones, both lists have been sorted by price
                // so this gives us a reasonable match up between values.
                for (int j = 0; j < shop.items.size(); ++j)
                {
                    const RandomizedShopItem& origItem = shop.items[j];
                    const RandomizedShopItem& newItem = shop.newItems[j];

                    game->write<uint16_t>(origItem.offset + 4, newItem.id);
                    if (keepPrices)
                    {
                        // We reuse the existing price for the randomized item by overwriting the value with the original.
                        game->write<uint32_t>(ShopOffsets::PricesStart + (newItem.id * 4), origItem.price);
                    }

                    std::string oldItemName = GameData::getItemName(origItem.id);
                    std::string newItemName = GameData::getItemName(newItem.id);
                    LOG("Randomized item in shop %d: %s changed to: %s", shopID, oldItemName.c_str(), newItemName.c_str());
                }
                for (int j = 0; j < shop.materia.size(); ++j)
                {
                    const RandomizedShopItem& origMat = shop.materia[j];
                    const RandomizedShopItem& newMat = shop.newMateria[j];

                    game->write<uint16_t>(origMat.offset + 4, newMat.id);
                    if (keepPrices)
                    {
                        // We reuse the existing price for the randomized materia by overwriting the value with the original.
                        game->write<uint32_t>(ShopOffsets::MateriaPricesStart + (newMat.id * 4), origMat.price);
                    }

                    std::string oldMateriaName = GameData::getMateriaName((uint8_t)origMat.id);
                    std::string newMateriaName = GameData::getMateriaName((uint8_t)newMat.id);
                    LOG("Randomized materia in shop %d: %s changed to: %s", shopID, oldMateriaName.c_str(), newMateriaName.c_str());
                }
            }
        }
        // Sell Menu
        else if (menuIdx == 1)
        {
            if (keepPrices)
            {
                LOG("Applied global sell prices.");
                game->write(ShopOffsets::PricesStart, (uint8_t*)itemSellPrices.data(), sizeof(itemSellPrices));
                game->write(ShopOffsets::MateriaPricesStart, (uint8_t*)materiaSellPrices.data(), sizeof(materiaSellPrices));
            }
        }

        shopMenuIndex = menuIdx;
    }
}

uint16_t RandomizeShops::randomizeShopItem(uint16_t itemID, const std::set<uint16_t>& previouslyChosen)
{
    uint16_t selectedID = itemID;

    // Item
    if (itemID < 128)
    {
        selectedID = GameData::getRandomItem(rng, true, excludeRareItems, previouslyChosen);
    }
    // Weapon
    else if (itemID < 256)
    {
        selectedID = 128 + GameData::getRandomWeapon(rng, true, excludeRareItems, previouslyChosen);
    }
    // Armor
    else if (itemID < 288)
    {
        selectedID = 256 + GameData::getRandomArmor(rng, true, excludeRareItems, previouslyChosen);
    }
    // Accessory
    else
    {
        selectedID = 288 + GameData::getRandomAccessory(rng, true, excludeRareItems, previouslyChosen);
    }

    return selectedID;
}

uint16_t RandomizeShops::randomizeShopMateria(uint16_t materiaID, const std::set<uint16_t>& previouslyChosen)
{
    return GameData::getRandomMateria(rng, true, excludeRareItems, previouslyChosen);
}