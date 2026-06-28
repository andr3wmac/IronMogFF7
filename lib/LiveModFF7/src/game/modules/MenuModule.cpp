#include "LiveModFF7/game/modules/MenuModule.h"
#include "LiveModFF7/game/GameManager.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"

std::string CharacterNames[] = { "Cloud", "Barret", "Tifa", "Aeris", "Red XIII", "Yuffie", "Cait Sith", "Vincent", "Cid" };

void MenuModule::setup(GameManager* game)
{
    this->game = game;
    gameModule = game->getGameModule();

    BIND_EVENT_ONE_ARG(game->onModuleChanged, MenuModule::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onUpdate, MenuModule::onUpdate);
    BIND_EVENT_ONE_ARG(game->onFrame, MenuModule::onFrame);
}

void MenuModule::onModuleChanged(uint8_t newGameModule)
{
    if (gameModule != GameModule::Menu && newGameModule == GameModule::Menu)
    {
        uint8_t menuType = game->read<uint8_t>(GameOffsets::MenuType);
        if (menuType == MenuType::Shop)
        {
            waitingForShopData = true;
        }
        if (menuType == MenuType::NameEntry)
        {
            waitingForNameData = true;
        }
    }

    if (newGameModule != GameModule::Menu && inShopMenu)
    {
        // HACK: when we exit a shop sometimes the field doesn't overwrite the shop data
        // so it stays stale in memory, then the next time we open the shop isShopDataLoaded()
        // gets false positive from old memory. So, we corrupt one of the materia prices on exit.
        uint32_t lastMateriaPrice = game->read<uint32_t>(ShopOffsets::MateriaPricesStart + (68 * 4));
        if (lastMateriaPrice == 9000)
        {
            game->write<uint32_t>(ShopOffsets::MateriaPricesStart + (68 * 4), 0);
        }

        inShopMenu = false;
        shopMenuIndex = -1;
    }

    gameModule = newGameModule;
}

void MenuModule::onUpdate(bool justConnected)
{
    if (gameModule == GameModule::Menu)
    {
        if (waitingForShopData && isShopDataLoaded())
        {
            game->onShopOpened.invoke();
            waitingForShopData = false;
            inShopMenu = true;
        }

        if (waitingForNameData)
        {
            std::string name = game->readString(GameOffsets::NameEntryString, 9);

            for (int i = 0; i < 9; ++i)
            {
                if (name == CharacterNames[i])
                {
                    game->onNameEntryOpened.invoke(name);
                    waitingForNameData = false;
                    break;
                }
            }
        }
    }
}

void MenuModule::onFrame(int frameNumber)
{
    if (game->getGameModule() != GameModule::Menu)
    {
        shopMenuIndex = -1;
        return;
    }

    if (inShopMenu)
    {
        uint8_t menuIdx = game->read<uint8_t>(ShopOffsets::MenuIndex);
        if (shopMenuIndex != menuIdx)
        {
            onShopMenuChanged(menuIdx);
            shopMenuIndex = menuIdx;
        }
    }
}

// Detect if shop data is fully loaded by verifying the set of information
// we know about the shop is confirmed in memory.
bool MenuModule::isShopDataLoaded()
{
    if (gameModule != GameModule::Menu)
    {
        return false;
    }

    uintptr_t shopOffset = ShopOffsets::ShopStart + (84 * 2);

    uint16_t shopType = game->read<uint16_t>(shopOffset + 0);
    if (shopType > 8) { return false; }

    uint8_t invCount = game->read<uint8_t>(shopOffset + 2);
    if (invCount == 0 || invCount > SHOP_ITEM_MAX) { return false; }

    uint8_t padding = game->read<uint8_t>(shopOffset + 3);
    if (padding != 0) { return false; }

    for (int i = invCount; i < SHOP_ITEM_MAX; ++i)
    {
        uintptr_t itemOffset = shopOffset + 4 + (i * 8);

        uint32_t itemType = game->read<uint32_t>(itemOffset + 0);
        uint16_t itemID = game->read<uint32_t>(itemOffset + 4);
        uint16_t itemPadding = game->read<uint16_t>(itemOffset + 6);

        // If we're outside the specified item count we should see all zeroes.
        if (itemType != 0 || itemID != 0 || itemPadding != 0)
        {
            return false;
        }
    }

    // Check some hardcoded prices to ensure the price table is loaded
    {
        uint32_t lastItemPrice = game->read<uint32_t>(ShopOffsets::PricesStart + (101 * 4));
        if (lastItemPrice != 50) { return false; }

        uint32_t lastWeaponPrice = game->read<uint32_t>(ShopOffsets::PricesStart + (255 * 4));
        if (lastWeaponPrice != 999999) { return false; }

        uint32_t lastArmorPrice = game->read<uint32_t>(ShopOffsets::PricesStart + (287 * 4));
        if (lastArmorPrice != 2) { return false; }

        uint32_t lastAccessoryPrice = game->read<uint32_t>(ShopOffsets::PricesStart + (317 * 4));
        if (lastAccessoryPrice != 10000) { return false; }

        uint32_t lastMateriaPrice = game->read<uint32_t>(ShopOffsets::MateriaPricesStart + (68 * 4));
        if (lastMateriaPrice != 9000) { return false; }
    }

    return true;
}

void MenuModule::onShopMenuChanged(uint8_t menuIndex)
{
    // Trigger anything else that might modify the shop (e.g. randomizers, ban enforcement).
    game->onShopMenuChanged.invoke(menuIndex);
}