#include "GameplayMods.h"
#include "core/game/MemoryOffsets.h"
#include "rules/Restrictions.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"

#include <imgui.h>
#include <array>
#include <string>
#include "core/gui/GUI.h"

REGISTER_RULE(GameplayMods, "Gameplay Mods", "Modify aspects of how the game works.")

static const char* masamuneModes[] { "No One", "Cloud", "Everyone", "Random Character" };
static const char* aerithModes[] { "Never", "Always", "Item" };

void GameplayMods::setup()
{
    BIND_EVENT(game->onStart, GameplayMods::onStart);
    BIND_EVENT_ONE_ARG(game->onFieldChanged, GameplayMods::onFieldChanged);
    BIND_EVENT_ONE_ARG(game->onFrame, GameplayMods::onFrame);
    BIND_EVENT_ONE_ARG(game->onCustomItemUsed, GameplayMods::onCustomItemUsed);
}

bool GameplayMods::onSettingsGUI()
{
    bool changed = false;

    ImGui::Spacing();
    ImGui::Text("Masamune Equippable By:");
    ImGui::SetItemTooltip("Modifies the Masamune to be equippable and usable.");
    ImGui::SameLine(DPI(200.0f));
    ImGui::SetNextItemWidth(DPI(200.0f));

    int masamuneModeIndex = (int)masamuneMode;
    if (ImGui::Combo("##GameplayMods_masamuneMode", &masamuneModeIndex, masamuneModes, IM_ARRAYSIZE(masamuneModes)))
    {
        masamuneMode = (MasamuneMode)masamuneModeIndex;
        changed = true;
    }

    ImGui::Spacing();
    ImGui::Text("Aerith Survives:");
    ImGui::SetItemTooltip("Never: Aerith dies as in vanilla.\nAlways: Aerith survives and rejoins via PHS.\nItem: Aerith dies, but a findable item can revive her.");
    ImGui::SameLine(DPI(200.0f));
    ImGui::SetNextItemWidth(DPI(200.0f));

    int aerithModeIndex = (int)aerithMode;
    if (ImGui::Combo("##GameplayMods_aerithMode", &aerithModeIndex, aerithModes, IM_ARRAYSIZE(aerithModes)))
    {
        aerithMode = (AerithMode)aerithModeIndex;
        changed = true;
    }

    if (aerithMode == AerithMode::Item)
    {
        ImGui::Text("Item Drop Odds:");
        ImGui::SameLine(DPI(200.0f));
        ImGui::SetNextItemWidth(DPI(200.0f));
        changed |= ImGui::SliderFloat("##GameplayMods_aerithItemOdds", &aerithItemOdds, 0.0f, 1.0f, "%.2f");
    }

    return changed;
}

void GameplayMods::loadSettings(const ConfigFile& cfg)
{
    masamuneMode = (MasamuneMode)cfg.get<int>("musamuneMode", (int)masamuneMode);
    aerithMode = (AerithMode)cfg.get<int>("aerithMode", (int)aerithMode);
    aerithItemOdds = cfg.get<float>("aerithItemOdds", aerithItemOdds);

    // Migrate the old boolean "Aerith Survives" setting to the mode dropdown.
    if (cfg.get<bool>("aerithSurvives", false))
    {
        aerithMode = AerithMode::Always;
    }
}

void GameplayMods::saveSettings(ConfigFile& cfg)
{
    cfg.set<int>("musamuneMode", (int)masamuneMode);
    cfg.set<int>("aerithMode", (int)aerithMode);
    cfg.set<float>("aerithItemOdds", aerithItemOdds);
}

void GameplayMods::onStart()
{
    rng.seed(game->getSeed());
    applyMasamuneMode();

    // Note: revival state is runtime-only for now, so a mid-run reload won't re-arm the field-script
    // softlock patches for an item-revived Aerith (her PHS availability is saved and persists, though).
    aerithRevived = false;
    aerithItemId = 0xFFFF;
    if (aerithMode == AerithMode::Item)
    {
        CustomItem item;
        item.name = "Aerith's Ribbon";
        item.targetsCharacter = false;
        item.spawnChance = aerithItemOdds;
        item.isUnique = true;
        aerithItemId = game->registerCustomItem(item);
        LOG("Registered Aerith's Ribbon as item ID: %d", aerithItemId);
    }
}

void GameplayMods::onCustomItemUsed(CustomItemUse use)
{
    if (aerithMode != AerithMode::Item || use.itemId != aerithItemId)
    {
        return;
    }

    aerithRevived = true;
    game->menu.showPopup("Aerith has been revived!");
    addAerithToPHS();
    LOG("Aerith Revive: revive item used; Aerith enabled on the PHS.");
}

void GameplayMods::addAerithToPHS()
{
    // Make Aerith appear on the PHS and clear her lock bit so she can be swapped into the party.
    uint16_t phsVisMask = game->read<uint16_t>(GameOffsets::PHSVisibilityMask);
    phsVisMask |= (1 << CharacterID::Aerith);
    game->write<uint16_t>(GameOffsets::PHSVisibilityMask, phsVisMask);

    uint16_t phsLockMask = game->read<uint16_t>(GameOffsets::PHSLockMask);
    phsLockMask &= ~(1 << CharacterID::Aerith);
    game->write<uint16_t>(GameOffsets::PHSLockMask, phsLockMask);

    LOG("Aerith Survives: enabled Aerith on the PHS.");
}

void GameplayMods::applyMasamuneMode()
{
    const uintptr_t masamuneEquippableAddr = 0x74E82; // uint16_t

    if (masamuneMode == MasamuneMode::Cloud)
    {
        game->write<uint16_t>(masamuneEquippableAddr, 0x01);
    }

    if (masamuneMode == MasamuneMode::Everyone)
    {
        game->write<uint16_t>(masamuneEquippableAddr, 0xFF);
    }

    if (masamuneMode == MasamuneMode::RandomCharacter)
    {
        std::uniform_int_distribution dist(0, 8);
        uint8_t selectedCharacter = dist(rng);

        std::string characterName = getCharacterName(selectedCharacter);
        LOG("Masamune equippable by: %s", characterName.c_str());

        Flags<uint16_t> equipFlags = 0;
        equipFlags.setBit(selectedCharacter, true);
        game->write<uint16_t>(masamuneEquippableAddr, equipFlags.value());
    }
}

void GameplayMods::onFieldChanged(uint16_t fieldID)
{
    if (aerithActive())
    {
        applyAerithSurvives(fieldID);
    }
}

void GameplayMods::onFrame(uint32_t frameNumber)
{
    if (aerithActive())
    {
        // Northern Crater party split (las0_8). The player picks who goes left/right to form the descent party 
        // but with Aerith absent she can't be chosen, so a "send only one person left" choice leaves a two-member party.
        if (game->getFieldID() == 751 && !game->inParty(CharacterID::Aerith))
        {
            // Act only while Cloud's "This will be the end of it!" confirmation window is showing. By this point the 
            // descent party has been assembled from whoever the player sent left, and the window is waiting on the
            // player to confirm, which gives us a stable moment to slot Aerith in.
            std::string window = game->getWindowText(0);
            if (window.find("This will be the end of it") != std::string::npos)
            {
                // Fill the first empty slot with Aerith if there is one.
                std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
                for (int i = 0; i < 3; ++i)
                {
                    if (partyIDs[i] == 0xFF)
                    {
                        game->write<uint8_t>(GameOffsets::PartyIDList + i, CharacterID::Aerith);
                        LOG("Aerith Survives: added Aerith to the party at the Northern Crater split (slot %d).", i);
                        break;
                    }
                }
            }
        }
    }
}

void GameplayMods::applyAerithSurvives(uint16_t fieldID)
{
    // Re-enable Aerith on the PHS when Disc 2 starts.
    if (fieldID == 634 && game->getGameMoment() == 677)
    {
        addAerithToPHS();
    }

    // No need to patch if shes not in the party.
    if (!game->inParty(CharacterID::Aerith))
    {
        return;
    }

    // TODO: support CSR
    if (game->getGameVersion() != GameVersion::PlayStationUS)
    {
        return;
    }

    // Whirlwind Maze Sephiroth confrontation (trnad_4). Its narration script (group 13, script 9)
    // has two spin-wait loops gated on a per-character "done talking" bit that Aerith never toggles.
    if (fieldID == 705)
    {
        // Change conditional jump into unconditional to jump over loop.
        static const uintptr_t loopIfAddrs[] = { 0x0DDC, 0x0E31 };
        for (uintptr_t addr : loopIfAddrs)
        {
            game->write<uint8_t>(FieldScriptOffsets::ScriptStart + addr + 0, 0x10); // JMPF
            game->write<uint8_t>(FieldScriptOffsets::ScriptStart + addr + 1, 0x1D); // loop exit label
        }

        LOG("Aerith Survives: patched Whirlwind Maze narration loops to jump past the Aerith stall.");
    }

    // Mideel (itown1a). Group 11 "line01", script 5 spin-waits until the count of extra party members
    // that have run into the building exceeds 1 and with Aerith absent it only ever reaches 1.
    if (fieldID == 712)
    {
        // Relax the comparison value from 1 to 0 so a count of 1 exits.
        game->write<uint8_t>(FieldScriptOffsets::ScriptStart + 0x113B + 3, 0x00);
        LOG("Aerith Survives: relaxed Mideel count check from 1 to 0.");
    }

    // Rocket in space (rcktin5). Group 12 "cid", script 15 spin-waits until a party-member counter reaches 2, 
    // and with Aerith absent it only ever reaches 1.
    if (fieldID == 567)
    {
        // Relax the comparison value from 2 to 1 so a count of 1 exits. 
        game->write<uint8_t>(FieldScriptOffsets::ScriptStart + 0x17B9 + 3, 0x01);
        LOG("Aerith Survives: relaxed count check from 2 to 1.");
    }

    // Rocket in space, next room (rcktin6). Group 8 "cloud", script 5 spin-waits until a party-member
    // counter becomes nonzero and with Aerith absent nothing ever increments it.
    if (fieldID == 568)
    {
        // Redirect each backward jump to its loop's exit label by turning the JMPB into a JMPF
        static const uintptr_t loopBackAddrs[] = { 0x091D, 0x0951 };
        for (uintptr_t addr : loopBackAddrs)
        {
            game->write<uint8_t>(FieldScriptOffsets::ScriptStart + addr + 0, 0x10); // JMPF
            game->write<uint8_t>(FieldScriptOffsets::ScriptStart + addr + 1, 0x01); // loop exit label
        }

        LOG("Aerith Survives: redirected wait loops past the Aerith stall.");
    }
}