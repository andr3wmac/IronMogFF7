#include "GameplayMods.h"
#include "app/gui/GUI.h"
#include "AppFrame/AppFrame.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"
#include "rules/Restrictions.h"
#include "utilities/Flags.h"

REGISTER_RULE(GameplayMods, "Gameplay Mods", "Modify aspects of how the game works.")

static const char* masamuneModes[] { "No One", "Cloud", "Everyone", "Random Character" };

void GameplayMods::setup()
{
    BIND_EVENT(game->onStart, GameplayMods::onStart);
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

    return changed;
}

void GameplayMods::loadSettings(const ConfigFile& cfg)
{
    masamuneMode = (MasamuneMode)cfg.get<int>("musamuneMode", (int)masamuneMode);
}

void GameplayMods::saveSettings(ConfigFile& cfg)
{
    cfg.set<int>("musamuneMode", (int)masamuneMode);
}

void GameplayMods::onStart()
{
    rng.seed(game->getSeed());
    applyMasamuneMode();
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
