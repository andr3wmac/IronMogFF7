#include "RandomizeLimitBreaks.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/gui/GUI.h"
#include "core/gui/IconsFontAwesome5.h"
#include "core/utilities/Flags.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

#include <imgui.h>
#include <random>
#include <set>

REGISTER_RULE(RandomizeLimitBreaks, "Randomize Limit Breaks", "Shuffles limit breaks.")

using LimitMap = std::array<std::array<std::vector<int>, 4>, 9>;

const LimitMap CharacterLimits = { {
    {{{0, 1}, {2, 3}, {4, 5}, {6}}},          // Cloud
    {{{7, 9}, {8, 10}, {11, 12}, {13}}},      // Barret
    {{{21, 22}, {23, 24}, {25, 26}, {27}}},   // Tifa
    {{{14, 15}, {16, 17}, {18, 19}, {20}}},   // Aerith
    {{{35, 36}, {37, 38}, {39, 40}, {41}}},   // Red XIII
    {{{49, 50}, {51, 52}, {53, 54}, {55}}},   // Yuffie
    {{{42}, {44}, {}, {}}},                   // Cait Sith
    {{{45}, {46}, {47}, {48}}},               // Vincent
    {{{28, 29}, {30, 31}, {32, 33}, {34}}}    // Cid
} };

void RandomizeLimitBreaks::setup()
{
    BIND_EVENT(game->onStart, RandomizeLimitBreaks::onStart);
    BIND_EVENT(game->onBattleEnter, RandomizeLimitBreaks::onBattleEnter);
}

bool RandomizeLimitBreaks::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Keep First Limit", &keepFirstLimit);
    ImGui::SetItemTooltip("Keeps all characters first limits unrandomized.");

    changed |= ImGui::Checkbox("Unrestricted " ICON_FA_EXCLAMATION_TRIANGLE, &unrestricted);
    ImGui::SetItemTooltip("Allows limits breaks to be randomized between characters.\nUse at your own risk.");

    return changed;
}

void RandomizeLimitBreaks::loadSettings(const ConfigFile& cfg)
{
    keepFirstLimit = cfg.get<bool>("keepFirstLimit", keepFirstLimit);
    unrestricted   = cfg.get<bool>("unrestricted", unrestricted);
}

void RandomizeLimitBreaks::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool>("keepFirstLimit", keepFirstLimit);
    cfg.set<bool>("unrestricted", unrestricted);
}

void RandomizeLimitBreaks::onDebugGUI()
{
    if (ImGui::CollapsingHeader("Name Table"))
    {
        uintptr_t stringTableStart = 0x669A6;

        for (int i = 0; i < GameData::limitBreaks.size(); ++i)
        {
            uint16_t offset = game->read<uint16_t>(stringTableStart + (i * 2));
            std::string limitName = game->readString(0x668A4 + offset + 4, 12);

            std::string limitText = std::to_string(i) + ") " + std::to_string(offset) + " " + limitName;
            ImGui::Text(limitText.c_str());
        }

    }

    if (ImGui::CollapsingHeader("Battle"))
    {
        const auto& [scene, formation] = game->getBattleFormation();

        if (scene == nullptr || formation == nullptr)
        {
            ImGui::Text("Not currently in a battle.");
            return;
        }

        // Max 3 unique enemies per fight
        for (int i = 0; i < 3; ++i)
        {
            std::string enemyName = game->readString(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::Name, 32);
            std::string enemyText = std::to_string(i) + ") " + enemyName;
            ImGui::Text(enemyText.c_str());

            // Maximum of 4 item slots per enemy
            for (int j = 0; j < 4; ++j)
            {
                uint16_t dropID = game->read<uint16_t>(BattleSceneOffsets::Enemies[i] + BattleSceneOffsets::DropIDs[j]);

                // Empty slot
                if (dropID == 65535)
                {
                    std::string dropText = "  Drop " + std::to_string(j) + ": Empty.";
                    ImGui::Text(dropText.c_str());
                    continue;
                }

                std::string dropText = "  Drop " + std::to_string(j) + ": " + GameData::getItemName(dropID) + "(" + std::to_string(dropID) + ")";
                ImGui::Text(dropText.c_str());
            }
        }
    }
}

std::vector<std::string> RandomizeLimitBreaks::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Randomized)
    {
        return { "Limit Breaks" };
    }

    return {};
}

void RandomizeLimitBreaks::onStart()
{
    generateRandomLimitMap();
    updateLimitStringTable();
}

void RandomizeLimitBreaks::onBattleEnter()
{
    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    for (int p = 0; p < 3; ++p)
    {
        if (partyIDs[p] == 0xFF)
        {
            continue;
        }

        uint8_t characterID = partyIDs[p];
        uintptr_t characterOffset = getCharacterDataOffset(characterID);
        std::string characterName = getCharacterName(characterID);
        uint8_t limitLevel = game->read<uint8_t>(characterOffset + CharacterDataOffsets::CurrentLimitLevel);

        if (keepFirstLimit&& limitLevel <= 1)
        {
            continue;
        }

        uintptr_t playerOffset = PlayerOffsets::Players[p];

        const std::vector<int>& limits = CharacterLimits[characterID][limitLevel - 1];
        for (int i = 0; i < limits.size(); ++i)
        {
            int limitID = limits[i];
            int randomizedLimitID = limitMap[limitID];

            AttackData attackData = GameData::limitBreaks[randomizedLimitID].attackData;
            uintptr_t limitOffset = playerOffset + PlayerOffsets::LimitData + (i * 28);
            game->write(limitOffset, (uint8_t*)(&attackData), 28);

            const std::string& oldName = GameData::limitBreaks[limitID].name;
            const std::string& newName = GameData::limitBreaks[randomizedLimitID].name;

            LOG("Randomized %s limit %s to %s.", characterName.c_str(), oldName.c_str(), newName.c_str());
        }
    }
}

void RandomizeLimitBreaks::generateRandomLimitMap()
{
    uint16_t currentOffset = 1594;
    limitStringOffsets.resize(GameData::limitBreaks.size());
    limitMap.resize(GameData::limitBreaks.size());
    for (int i = 0; i < limitMap.size(); ++i)
    {
        // Store each offset for the vanilla arrangement
        limitStringOffsets[i] = currentOffset;

        // 3 bytes added to each length for color code at start and null terminator at end
        size_t stringLength = GameData::limitBreaks[i].name.size() + 3;
        currentOffset += stringLength;

        // Store index to be shuffled below
        limitMap[i] = i;
    }

    std::mt19937 rng(game->getSeed());
    std::shuffle(limitMap.begin(), limitMap.end(), rng);
}

void RandomizeLimitBreaks::updateLimitStringTable()
{
    uintptr_t stringTableStart = 0x669A6;

    for (int i = 0; i < GameData::limitBreaks.size(); ++i)
    {
        int limitIdx = limitMap[i];
        game->write<uint16_t>(stringTableStart + (i * 2), limitStringOffsets[limitIdx]);
    }
}