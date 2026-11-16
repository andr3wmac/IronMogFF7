#include "RandomizeCharacters.h"
#include "AppFrame/AppFrame.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "LiveModFF7Core/utilities/Utilities.h"

#include <algorithm>
#include <numeric>
#include <random>

REGISTER_MOD(RandomizeCharacters, "Randomize Characters", "Randomizers")

std::string RandomizeCharacters::getDescription() const
{
    return "Shuffles the playable characters so you don't know who you're going to get.\n\n"
           "Each character keeps their own story role and stats but looks like someone else in fields, battles and menus, "
           "and uses that character's limit breaks and weapons. Special field animations fall back to standing poses.";
}

static const char* CharacterNames[] = { "Cloud", "Barret", "Tifa", "Aerith", "Red XIII", "Yuffie", "Cait Sith", "Vincent", "Cid" };
static constexpr uint32_t ShuffleSalt = 0x43484152; // "CHAR"

void RandomizeCharacters::setup()
{
    applied = false;
    reroll = 0;
    BIND_EVENT(game->onStart, RandomizeCharacters::onStart);
    BIND_EVENT(game->onLoadUpdate, RandomizeCharacters::onLoadUpdate);
}

std::vector<std::string> RandomizeCharacters::describe(ModDescriptionType descType)
{
    if (descType == ModDescriptionType::Randomized)
    {
        return { "Characters" };
    }
    return {};
}

// The seed is final here: the manager loads it from the save before starting.
void RandomizeCharacters::onStart()
{
    if (enabled)
    {
        applyShuffle();
    }
}

// Mods can be switched on and off while connected, so the swaps follow the enabled state.
void RandomizeCharacters::onLoadUpdate()
{
    if (enabled && !applied)
    {
        applyShuffle();
    }
    else if (!enabled && applied)
    {
        clearShuffle();
    }
}

void RandomizeCharacters::applyShuffle()
{
    std::iota(appearance.begin(), appearance.end(), 0);
    std::mt19937_64 rng(Utilities::makeSeed64(game->getSeed() + reroll, ShuffleSalt));
    std::shuffle(appearance.begin(), appearance.end(), rng);

    for (uint8_t id = 0; id < appearance.size(); ++id)
    {
        game->field.models.setSwap(getFieldGlobalModelID(id), getFieldGlobalModelID(appearance[id]));
        game->battle.models.setSwap(id, appearance[id]);
        game->menu.setPortrait(id, appearance[id]);
        game->menu.setNameEntryDefault(id, appearance[id]);
        game->battle.limits.setLimitSet(id, appearance[id]);
        game->menu.equipment.setEquipSource(id, appearance[id]);
        LOG("Characters: %s appears as %s.", CharacterNames[id], CharacterNames[appearance[id]]);
    }
    applied = true;
}

void RandomizeCharacters::clearShuffle()
{
    game->field.models.clearSwaps();
    game->battle.models.clearSwaps();
    game->menu.clearPortraits();
    game->menu.clearNameEntryDefaults();
    game->battle.limits.clearLimitSets();
    game->menu.equipment.clearEquipSources();
    applied = false;
}

void RandomizeCharacters::onDebugGUI()
{
    const FieldModelManager& field = game->field.models;
    const BattleModelManager& battle = game->battle.models;

    if (ImGui::BeginTable("CharacterAppearances", 2, ImGuiTableFlags_SizingFixedFit))
    {
        for (uint8_t id = 0; id < appearance.size(); ++id)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", CharacterNames[id]);
            ImGui::TableNextColumn();
            bool pending = appearance[id] != id && !game->menu.isPortraitRedirected(id);
            ImGui::Text("appears as %s%s", CharacterNames[appearance[id]], pending ? " (portrait pending)" : "");
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Reroll mapping"))
    {
        // Bodies already loaded change on their next load.
        game->queueAction([this]()
        {
            ++reroll;
            applyShuffle();
        });
    }

    ImGui::TextWrapped("Field %d: %s", field.getFieldID(), field.getStatus());
    ImGui::Text("Field matches / failures: %u / %u, scene clips aliased: %u", field.getMatches(), field.getFailures(), field.getAliasedClips());
    ImGui::TextWrapped("Battle: %s", battle.getStatus());
    ImGui::Text("Battle bodies matched / failed: %u / %u", battle.getMatches(), battle.getFailures());
    ImGui::TextWrapped("Limit breaks: %s", game->battle.limits.getStatus());
    ImGui::TextWrapped("Name entry: %s", game->menu.getNameEntryStatus());
    ImGui::TextWrapped("Equipment: %s", game->menu.equipment.getStatus());
}
