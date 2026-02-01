#include "NoDuping.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"

#include <bitset>
#include <imgui.h>
#include <sstream>

REGISTER_RULE("No Duping", NoDuping)

void NoDuping::setup()
{
    BIND_EVENT(game->onStart, NoDuping::onStart);
    BIND_EVENT(game->onBattleEnter, NoDuping::onBattleEnter);
    BIND_EVENT_ONE_ARG(game->onFrame, NoDuping::onFrame);
}

void NoDuping::onDebugGUI()
{
    // Controller input state
    {
        uint8_t btn = game->read<uint8_t>(BattleOffsets::ControllerInputs);

        std::ostringstream stringStream;
        stringStream << std::bitset<8>(btn) << std::endl;

        std::string btnText = "Buttons: " + stringStream.str();
        ImGui::Text(btnText.c_str());
    }

    // Cache state
    ImGui::Text("Cache: ");
    for (int i = 0; i < wItemCache.size(); ++i)
    {
        auto& [itemIndex, itemQuantity] = wItemCache[i];

        std::string entryText = " " + std::to_string(i) + ") " + std::to_string(itemIndex) + " " + std::to_string(itemQuantity);
        ImGui::Text(entryText.c_str());
    }
}

void NoDuping::onStart()
{
    if (game->getGameModule() == GameModule::Battle)
    {
        checkPartyMembers();
    }
}

void NoDuping::onBattleEnter()
{
    checkPartyMembers();

    cancelWasPressed = false;
    wItemCache.clear();
}

void NoDuping::checkPartyMembers()
{
    wItemPartyMembers = { false, false, false };

    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    for (int p = 0; p < 3; ++p)
    {
        if (partyIDs[p] == 0xFF)
        {
            continue;
        }

        uintptr_t characterOffset = getCharacterDataOffset(partyIDs[p]);
        bool hasWItemMateria = false;

        // Weapon Materia Slots
        for (int i = 0; i < 8; ++i)
        {
            uintptr_t materiaOffset = characterOffset + CharacterDataOffsets::WeaponMateria[i];
            uint32_t materiaID = game->read<uint32_t>(materiaOffset);

            // Check if its W-Item Materia
            if ((materiaID & 0xFF) == 21)
            {
                hasWItemMateria = true;
            }
        }

        // Armor Materia Slots
        for (int i = 0; i < 8; ++i)
        {
            uintptr_t materiaOffset = characterOffset + CharacterDataOffsets::ArmorMateria[i];
            uint32_t materiaID = game->read<uint32_t>(materiaOffset);

            // Check if its W-Item Materia
            if ((materiaID & 0xFF) == 21)
            {
                hasWItemMateria = true;
            }
        }

        if (hasWItemMateria)
        {
            wItemPartyMembers[p] = true;
        }
    }
}

void NoDuping::onFrame(uint32_t frameNumber)
{
    if (game->getGameModule() != GameModule::Battle)
    {
        return;
    }

    uint8_t activePlayer = game->read<uint8_t>(BattleMenuOffsets::ActivePlayer);
    if (activePlayer >= 3)
    {
        return;
    }

    // Changed players
    if (lastActivePlayer != activePlayer)
    {
        cancelWasPressed = false;
        wItemCache.clear();
        lastActivePlayer = activePlayer;
    }

    // Make sure the active menu is someone with w-item
    if (!wItemPartyMembers[activePlayer])
    {
        return;
    }

    // And that w-item is the selected menu index
    uint8_t activeMenuIndex = game->read<uint8_t>(BattleMenuOffsets::PlayerMenu[activePlayer] + BattleMenuOffsets::MenuIndex);
    if (activeMenuIndex != 3)
    {
        cancelWasPressed = false;
        wItemCache.clear();
        return;
    }

    // This will be zero while we're in the item menu. Not entirely sure what it is but seems reliable.
    uint8_t subMenuData = game->read<uint8_t>(0x51322);
    if (subMenuData != 0)
    {
        cancelWasPressed = false;
        wItemCache.clear();
        return;
    }

    // Check if the cancel button was pressed. This works even if controller was remapped.
    uint8_t btn = game->read<uint8_t>(BattleOffsets::ControllerInputs);
    if ((btn >> 6) & 1) 
    {
        cancelWasPressed = true;
    }

    // This value flips between 0x00 and 0xFF depending on whether or not we're
    // targeting something. We use this as a trigger to check if we should 
    // evaluate whether an item duplication occurred or not.
    uint8_t targetTrigger = game->read<uint8_t>(0xFAFF1);
    if (targetTrigger != lastTargetTrigger)
    {
        if (targetTrigger == 0x00)
        {
            uint8_t itemScroll = game->read<uint8_t>(BattleMenuOffsets::PlayerMenu[activePlayer] + BattleMenuOffsets::ItemMenuScroll);
            uint8_t menuIndex = game->read<uint8_t>(BattleMenuOffsets::PlayerMenu[activePlayer] + BattleMenuOffsets::ItemMenuIndex);
            uint16_t selectedItemIndex = itemScroll + menuIndex;
            uint8_t selectedItemQuantity = game->read<uint8_t>(BattleOffsets::Inventory + (selectedItemIndex * 6) + 2);

            wItemCache.push_back({ selectedItemIndex, selectedItemQuantity });
        }
        else if (targetTrigger == 0xFF && wItemCache.size() == 1)
        {
            if (cancelWasPressed)
            {
                cancelWasPressed = false;
                wItemCache.clear();
            }
            else 
            {
                // After we select the first item 1 is consumed. If we do not subtract this here
                // we would be able to clone 1.
                wItemCache[0].second = wItemCache[0].second - 1;
            }
        }
        else if (targetTrigger == 0xFF && wItemCache.size() > 1)
        {
            wItemCache.pop_back();

            for (int i = 0; i < wItemCache.size(); ++i)
            {
                auto& [itemIndex, previousQuantity] = wItemCache[i];

                uint8_t currentQuantity = game->read<uint8_t>(BattleOffsets::Inventory + (itemIndex * 6) + 2);
                if (currentQuantity > previousQuantity)
                {
                    game->write<uint8_t>(BattleOffsets::Inventory + (itemIndex * 6) + 2, previousQuantity);

                    uint16_t itemID = game->read<uint16_t>(BattleOffsets::Inventory + (itemIndex * 6));
                    LOG("Item duplication prevented: %d", itemID);
                }
            }
        }

        lastTargetTrigger = targetTrigger;
    }
}