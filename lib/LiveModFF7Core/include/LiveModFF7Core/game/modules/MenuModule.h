#pragma once

#include <cstdint>
#include <string>

class GameManager;

class MenuModule
{
public:
    void setup(GameManager* game);
    void onModuleChanged(uint8_t newGameModule);
    void onUpdate(bool justConnected);
    void onFrame(int frameNumber);

    // True while any in-game menu is open, including the field menu on the world map (where
    // CurrentModule stays World). Backed by MenuOffsets::MenuOpenFlag; see GameManager::inMenu.
    bool isOpen();

    // Shows the field-menu character-response popup with arbitrary text. Only renders while a menu is
    // open. color 7 = white; other values give other colors.
    void showPopup(const std::string& text, uint8_t frames = 90, uint8_t color = 7);

    // Resolves the item currently highlighted in the field/main-menu item list to its inventory id.
    // FF7 stores no selected-item id; it is derived live from the visible list position. Optionally
    // returns the resolved inventory slot. Only meaningful while the item menu is open.
    uint16_t getSelectedItemID(uint16_t* outSlot = nullptr);

    // Backs out of the item's "choose a target" prompt to the item list (a programmatic cancel).
    void cancelItemTargetPrompt();

private:
    bool isShopDataLoaded();
    void onShopMenuChanged(uint8_t menuIndex);

    // Watches the item menu for a registered custom item being used and fires game->onCustomItemUsed.
    void updateCustomItemUse();

    GameManager* game = nullptr;
    uint8_t gameModule = 0;

    int shopMenuIndex = -1;
    bool waitingForShopData = false;
    bool inShopMenu = false;

    bool waitingForNameData = false;

    // Previous item target-select state, used to fire a custom item use only on the rising edge.
    uint8_t lastItemTargetActive = 0;
};