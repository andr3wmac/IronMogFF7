#pragma once

#include "LiveModFF7Core/game/modules/EquipmentManager.h"
#include <array>
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

    // Shows portraitCharacterID's portrait for characterID in every menu, including name entry. Party
    // portraits already in VRAM change at the next menu transition.
    void setPortrait(uint8_t characterID, uint8_t portraitCharacterID);
    void clearPortraits();
    bool isPortraitRedirected(uint8_t characterID);

    // The character whose portrait characterID shows: their override, or themselves.
    uint8_t getPortrait(uint8_t characterID) const;

    // Starts characterID's name entry with defaultCharacterID's default name.
    void setNameEntryDefault(uint8_t characterID, uint8_t defaultCharacterID);
    void clearNameEntryDefaults();
    const char* getNameEntryStatus() const { return nameEntryStatus; }

    // Gives characters another character's weapons, equip restrictions and starting weapon.
    EquipmentManager equipment;

private:
    static constexpr uint8_t NoOverride = 0xFF;

    void onLoadUpdate();
    void onExit();
    bool portraitTableResident();
    void updatePortraits();
    void releasePortraits();
    void updateNameEntryPortrait();
    void applyNameEntryDefault();

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

    // Per character ID, NoOverride when unchanged.
    std::array<uint8_t, 9> portraits = {NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride};
    std::array<uint8_t, 9> nameDefaults = {NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride, NoOverride};
    const char* nameEntryStatus = "Waiting for a name entry.";
};