#include "RandomizeCharacters.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"
#include "rules/Restrictions.h"

#include <algorithm>
#include <imgui.h>
#include <random>

REGISTER_RULE(RandomizeCharacters, "Randomize Characters", "Randomize character names, starting stats, and stat growth.")

std::string OriginalMaleNames[] = { "Cloud", "Barret", "Red XIII", "Cait Sith", "Vincent", "Cid" };
std::string OriginalFemaleNames[] = { "Tifa", "Aeris", "Yuffie" };

void RandomizeCharacters::setup()
{
    BIND_EVENT(game->onStart, RandomizeCharacters::onStart);
    BIND_EVENT_ONE_ARG(game->onNameEntryOpened, RandomizeCharacters::onNameEntryOpened);
}

bool RandomizeCharacters::onSettingsGUI()
{
    bool changed = false;

    changed |= ImGui::Checkbox("Randomize Names", &randomizeNames);
    ImGui::SetItemTooltip("Randomizes weapons with other weapons, armor with other armor, etc");

    return changed;
}

void RandomizeCharacters::loadSettings(const ConfigFile& cfg)
{
    randomizeNames = cfg.get<bool>("randomizeNames", randomizeNames);
}

void RandomizeCharacters::saveSettings(ConfigFile& cfg)
{
    cfg.set<bool> ("randomizeNames", randomizeNames);
}

void RandomizeCharacters::onDebugGUI()
{
    if (game->getGameModule() == GameModule::Menu)
    {
        std::string name = game->readString(GameOffsets::NameEntryString, 9);
        std::string nameText = "Name Entry: " + name;
        ImGui::Text(nameText.c_str());
    }
}

std::vector<std::string> RandomizeCharacters::describe(RuleDescripionType descType)
{
    if (descType == RuleDescripionType::Randomized)
    {
        return { "Characters" };
    }

    return {};
}

void RandomizeCharacters::onStart()
{
    rng.seed(game->getSeed());

    std::vector<std::string> maleNames = Utilities::loadListFromFile("settings/names_male.txt");
    if (maleNames.size() < 6)
    {
        for (int i = 0; i < (6 - maleNames.size()); ++i)
        {
            maleNames.push_back(OriginalMaleNames[i]);
        }
    }

    std::shuffle(maleNames.begin(), maleNames.end(), rng);

    for (int i = 0; i < 6; ++i)
    {
        nameMap[OriginalMaleNames[i]] = maleNames[i];
    }

    std::vector<std::string> femaleNames = Utilities::loadListFromFile("settings/names_female.txt");
    if (femaleNames.size() < 3)
    {
        for (int i = 0; i < (3 - femaleNames.size()); ++i)
        {
            femaleNames.push_back(OriginalFemaleNames[i]);
        }
    }

    std::shuffle(femaleNames.begin(), femaleNames.end(), rng);

    for (int i = 0; i < 3; ++i)
    {
        nameMap[OriginalFemaleNames[i]] = femaleNames[i];
    }
}

void RandomizeCharacters::onNameEntryOpened(std::string name)
{
    if (!randomizeNames)
    {
        return;
    }

    if (nameMap.count(name) == 0)
    {
        LOG("Name was not in the randomized name map: %s", name.c_str());
        return;
    }

    size_t sizeWritten = game->writeString(GameOffsets::NameEntryString, 9, nameMap[name]);

    // Write null terminator
    game->write<uint8_t>(GameOffsets::NameEntryString + sizeWritten, 0xFF);

    // Update cursor
    uint8_t cursorPos = sizeWritten >= 9 ? 8 : (uint8_t)sizeWritten;
    game->write<uint8_t>(GameOffsets::NameEntryCursor, cursorPos);
}