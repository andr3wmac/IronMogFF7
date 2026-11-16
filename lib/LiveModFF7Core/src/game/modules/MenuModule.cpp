#include "LiveModFF7Core/game/modules/MenuModule.h"
#include "LiveModFF7Core/game/CustomItem.h"
#include "LiveModFF7Core/game/GameData.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include <algorithm>
#include <array>
#include <vector>

std::string CharacterNames[] = { "Cloud", "Barret", "Tifa", "Aeris", "Red XIII", "Yuffie", "Cait Sith", "Vincent", "Cid" };

void MenuModule::setup(GameManager* game)
{
    this->game = game;
    gameModule = game->getGameModule();

    BIND_EVENT_ONE_ARG(game->onModuleChanged, MenuModule::onModuleChanged);
    BIND_EVENT_ONE_ARG(game->onUpdate, MenuModule::onUpdate);
    BIND_EVENT_ONE_ARG(game->onFrame, MenuModule::onFrame);
    BIND_EVENT(game->onLoadUpdate, MenuModule::onLoadUpdate);
    BIND_EVENT(game->onExit, MenuModule::onExit);

    equipment.setup(game);
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

        // The name menu stays in RAM between name entries, so the previous entry's name can still be in
        // the buffer while the next menu loads. Wait until the menu has started for the requested
        // character (it stores the index just before filling in the default) and shows its default.
        if (waitingForNameData)
        {
            int16_t named = game->read<int16_t>(MenuOffsets::NameEntryCharacter);
            std::string name = game->readString(GameOffsets::NameEntryString, 9);
            if (named >= 0 && named < 9 && game->read<int32_t>(MenuOffsets::NameEntryIndex) == named && name == CharacterNames[named])
            {
                applyNameEntryDefault();
                game->onNameEntryOpened.invoke(name);
                waitingForNameData = false;
            }
        }
    }

    // On the world map opening the menu stays in World module.
    if (gameModule == GameModule::Menu || gameModule == GameModule::World)
    {
        updateCustomItemUse();
    }
}

bool MenuModule::isOpen()
{
    uint8_t module = game->getGameModule();

    // The flag's RAM is reused by battle/minigames, so only trust it where a menu can actually be up.
    if (module != GameModule::Menu && module != GameModule::World && module != GameModule::Field)
    {
        return false;
    }

    return game->read<int32_t>(MenuOffsets::MenuOpenFlag) != -1;
}

void MenuModule::showPopup(const std::string& text, uint8_t frames, uint8_t color)
{
    std::vector<uint8_t> msg = GameData::encodeString(text);
    msg.push_back(0xFF);
    for (size_t b = 0; b < msg.size(); ++b)
    {
        game->write<uint8_t>(MenuOffsets::ItemPopupText + b, msg[b]);
    }

    // Blank the other two response line slots so stale text doesn't show.
    game->write<uint8_t>(MenuOffsets::ItemPopupText + MenuOffsets::ItemPopupStride, 0xFF);
    game->write<uint8_t>(MenuOffsets::ItemPopupText + (MenuOffsets::ItemPopupStride * 2), 0xFF);

    // The game lazily initializes the text pointer/color on the first real popup so we set them
    // explicitly so it's always correct even on a fresh menu open.
    game->write<uint32_t>(MenuOffsets::PopupTextPtr, 0x80000000 | MenuOffsets::ItemPopupText);
    game->write<uint8_t>(MenuOffsets::PopupTextColor, color);
    game->write<uint8_t>(MenuOffsets::PopupPhaseA, 2);
    game->write<uint8_t>(MenuOffsets::PopupTimer, frames);
    game->write<uint8_t>(MenuOffsets::PopupPhaseB, 2);
}

uint16_t MenuModule::getSelectedItemID(uint16_t* outSlot)
{
    uint8_t scroll = game->read<uint8_t>(MenuOffsets::ItemListScroll);
    uint8_t cursor = game->read<uint8_t>(MenuOffsets::ItemListCursor);
    uint16_t slot = (uint16_t)scroll + (uint16_t)cursor;

    if (outSlot != nullptr)
    {
        *outSlot = slot;
    }

    uint16_t entry = game->read<uint16_t>(GameOffsets::Inventory + (slot * 2));
    return entry & 0x01FF;
}

void MenuModule::cancelItemTargetPrompt()
{
    game->write<uint8_t>(MenuOffsets::ItemTargetActive, 1);
}

void MenuModule::updateCustomItemUse()
{
    // Nothing to watch for unless a menu is actually open and there are registered custom items.
    // Reset the edge tracker whenever we bail so a stale value can't fire a use later.
    if (!game->hasCustomItems() || !isOpen())
    {
        lastItemTargetActive = 0;
        return;
    }

    uint8_t targetActive = game->read<uint8_t>(MenuOffsets::ItemTargetActive);
    uint8_t previousTargetActive = lastItemTargetActive;
    lastItemTargetActive = targetActive;

    // Fire only on the rising edge into target-select (2), so a stale value left in this menu RAM
    // while walking around can't trigger a use.
    if (targetActive != 2 || previousTargetActive == 2)
    {
        return;
    }

    uint16_t slot = 0;
    uint16_t itemID = getSelectedItemID(&slot);

    const CustomItem* item = game->findCustomItem(itemID);
    if (item == nullptr)
    {
        return;
    }

    if (item->getBlockedMessage)
    {
        std::string blockedMessage = item->getBlockedMessage();
        if (!blockedMessage.empty())
        {
            // Back out of the target prompt without consuming the item.
            cancelItemTargetPrompt();
            showPopup(blockedMessage);
            return;
        }
    }

    // Remove one from the stack (empty the slot if it was the last), then cancel the target prompt
    // back to the item list. Targeting items are not handled yet.
    uint16_t entry = game->read<uint16_t>(GameOffsets::Inventory + (slot * 2));
    uint8_t qty = (uint8_t)(entry >> 9);
    if (qty > 0)
    {
        qty--;
    }

    uint16_t newEntry = (qty == 0) ? 0xFFFF : (uint16_t)((qty << 9) | (itemID & 0x01FF));
    game->write<uint16_t>(GameOffsets::Inventory + (slot * 2), newEntry);
    cancelItemTargetPrompt();

    game->onCustomItemUsed.invoke({ itemID, 0xFF });
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

// Every portrait file is two sectors long, stored in character ID order.
static constexpr uint64_t FirstPortraitEntry = UINT64_C(0x000010000000D2BF);
static constexpr uint8_t ChocoboPortrait = 11;

uint64_t portraitEntry(uint8_t portraitID)
{
    return FirstPortraitEntry + 2 * portraitID;
}

// The name menu's embedded portraits: TIM, 8bpp with a 256x1 CLUT, 24x48 halfwords.
static constexpr uint32_t PortraitPaletteOffset = 0x14, PortraitPaletteBytes = 512;
static constexpr uint32_t PortraitPixelsOffset = 0x220, PortraitPixelsBytes = 2304;
static constexpr uint32_t PortraitBytes = PortraitPixelsOffset + PortraitPixelsBytes;

bool isPortrait(const std::array<uint8_t, PortraitBytes>& tim)
{
    auto u32 = [&](uint32_t offset) { return uint32_t(tim[offset]) | (tim[offset + 1] << 8) | (tim[offset + 2] << 16) | (uint32_t(tim[offset + 3]) << 24); };
    return u32(0) == 0x10 && u32(4) == 9 && u32(8) == 0x20C && u32(0x10) == 0x00010100 && u32(0x214) == 0x000C + PortraitPixelsBytes && u32(0x21C) == 0x00300018;
}

void MenuModule::setPortrait(uint8_t characterID, uint8_t portraitCharacterID)
{
    if (characterID < portraits.size() && portraitCharacterID < portraits.size())
    {
        portraits[characterID] = characterID == portraitCharacterID ? NoOverride : portraitCharacterID;
    }
}

void MenuModule::clearPortraits()
{
    portraits.fill(NoOverride);
    releasePortraits();
}

bool MenuModule::isPortraitRedirected(uint8_t characterID)
{
    return characterID < portraits.size() && portraits[characterID] != NoOverride &&
           game->read<uint64_t>(MenuOffsets::PortraitArchive + characterID * 8) == portraitEntry(portraits[characterID]);
}

uint8_t MenuModule::getPortrait(uint8_t characterID) const
{
    return characterID < portraits.size() && portraits[characterID] != NoOverride ? portraits[characterID] : characterID;
}

void MenuModule::setNameEntryDefault(uint8_t characterID, uint8_t defaultCharacterID)
{
    if (characterID < nameDefaults.size() && defaultCharacterID < nameDefaults.size())
    {
        nameDefaults[characterID] = characterID == defaultCharacterID ? NoOverride : defaultCharacterID;
    }
}

void MenuModule::clearNameEntryDefaults()
{
    nameDefaults.fill(NoOverride);
}

// The portrait table is kernel data, recognized by its Chocobo entry, which is never overridden.
bool MenuModule::portraitTableResident()
{
    return game->read<uint64_t>(MenuOffsets::PortraitArchive + ChocoboPortrait * 8) == portraitEntry(ChocoboPortrait);
}

void MenuModule::onLoadUpdate()
{
    if (!portraitTableResident())
    {
        return;
    }
    updatePortraits();
    updateNameEntryPortrait();
}

void MenuModule::onExit()
{
    releasePortraits();
}

// The kernel reads each character's portrait from disc through its portrait table, so pointing an
// entry at another file changes that portrait in every menu. Party portraits stay in VRAM until the
// kernel's dirty flag asks for a reload, so raise it on change. The game never writes this table, so
// it is re-applied after state loads.
void MenuModule::updatePortraits()
{
    bool changed = false;

    for (uint8_t id = 0; id < portraits.size(); ++id)
    {
        uint64_t wanted = portraitEntry(portraits[id] == NoOverride ? id : portraits[id]);
        uintptr_t entry = MenuOffsets::PortraitArchive + id * 8;
        if (game->read<uint64_t>(entry) != wanted)
        {
            game->write<uint64_t>(entry, wanted);
            changed = true;
        }
    }

    if (changed)
    {
        game->write<uint8_t>(MenuOffsets::PortraitsDirty, 1);
        LOG("Menu portraits updated.");
    }
}

void MenuModule::releasePortraits()
{
    if (!portraitTableResident())
    {
        return;
    }
    bool changed = false;
    for (uint8_t id = 0; id < portraits.size(); ++id)
    {
        uintptr_t entry = MenuOffsets::PortraitArchive + id * 8;
        if (game->read<uint64_t>(entry) != portraitEntry(id))
        {
            game->write<uint64_t>(entry, portraitEntry(id));
            changed = true;
        }
    }
    if (changed)
    {
        game->write<uint8_t>(MenuOffsets::PortraitsDirty, 1);
    }
}

// The name menu embeds every portrait and uploads the named character's once, right after its disc
// read. While it loads, give that portrait the override's palette and pixels. Headers are kept
// because their VRAM coordinates differ between portraits.
void MenuModule::updateNameEntryPortrait()
{
    if (game->read<uint8_t>(GameOffsets::MenuType) != MenuType::NameEntry)
    {
        return;
    }
    int16_t named = game->read<int16_t>(MenuOffsets::NameEntryCharacter);
    if (named < 0 || named >= (int16_t)portraits.size() || portraits[named] == NoOverride)
    {
        return;
    }

    // The window is the overlay's disc read, which can stall frames.
    game->requestFastUpdates();

    std::array<uint8_t, PortraitBytes> original, replacement;
    uintptr_t originalAddress = MenuOffsets::NameEntryPortraits + named * MenuOffsets::NameEntryPortraitStride;
    uintptr_t replacementAddress = MenuOffsets::NameEntryPortraits + portraits[named] * MenuOffsets::NameEntryPortraitStride;
    if (!game->read(originalAddress, PortraitBytes, original.data()) || !game->read(replacementAddress, PortraitBytes, replacement.data()) ||
        !isPortrait(original) || !isPortrait(replacement))
    {
        return; // Overlay not loaded yet.
    }

    bool palette = std::equal(original.begin() + PortraitPaletteOffset, original.begin() + PortraitPaletteOffset + PortraitPaletteBytes,
                              replacement.begin() + PortraitPaletteOffset);
    bool pixels = std::equal(original.begin() + PortraitPixelsOffset, original.end(), replacement.begin() + PortraitPixelsOffset);
    if (palette && pixels)
    {
        return;
    }

    // Repeats if a stale copy is overwritten by the incoming read; the portrait data never changes.
    game->write(originalAddress + PortraitPaletteOffset, replacement.data() + PortraitPaletteOffset, PortraitPaletteBytes);
    game->write(originalAddress + PortraitPixelsOffset, replacement.data() + PortraitPixelsOffset, PortraitPixelsBytes);
    nameEntryStatus = "Name entry portrait replaced.";
}

// Copies the override's entry from the menu's own default-name table, as the menu does itself. The
// dialog waits for input, so this can happen any time after the menu has filled in its default.
void MenuModule::applyNameEntryDefault()
{
    int16_t named = game->read<int16_t>(MenuOffsets::NameEntryCharacter);
    if (!portraitTableResident() || named < 0 || named >= (int16_t)nameDefaults.size() || nameDefaults[named] == NoOverride)
    {
        return;
    }

    std::array<uint8_t, 12> defaultName;
    if (!game->read(MenuOffsets::NameEntryDefaults + nameDefaults[named] * 12, defaultName.size(), defaultName.data()))
    {
        return;
    }

    uint8_t length = 0;
    while (length < 9 && defaultName[length] != 0xFF)
    {
        ++length;
    }
    game->write(GameOffsets::NameEntryString, defaultName.data(), length);
    game->write<uint16_t>(GameOffsets::NameEntryString + length, 0xFFFF);
    game->write<uint8_t>(GameOffsets::NameEntryCursor, std::min<uint8_t>(length, 8));
    nameEntryStatus = "Name entry default replaced.";
    LOG("Name entry for character %d starts with character %d's default name.", named, nameDefaults[named]);
}
