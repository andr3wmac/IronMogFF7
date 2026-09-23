#include "ModManager.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "mods/Mod.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <vector>

void ModManager::setup(GameManager* game)
{
    for (Mod* mod : Mod::getList())
    {
        if (!mod->enabled)
        {
            continue;
        }
        mod->setManager(game);
        mod->setup();
    }
}

bool ModManager::isModEnabled(const std::string& modName)
{
    for (Mod* mod : Mod::getList())
    {
        if (mod->enabled && mod->name == modName)
        {
            return true;
        }
    }

    return false;
}

Mod* ModManager::getMod(const std::string& modName)
{
    for (Mod* mod : Mod::getList())
    {
        if (mod->enabled && mod->name == modName)
        {
            return mod;
        }
    }

    return nullptr;
}

std::string ModManager::getSettingsSummary()
{
    std::map<std::string, std::vector<std::string>> groups;

    groups["No"] = {};
    groups["Randomized"] = {};
    groups["Multipliers"] = {};
    groups["Unique"] = {};
    groups["BanItems"] = {};
    groups["BanMateria"] = {};

    for (Mod* mod : Mod::getList())
    {
        if (!mod->enabled)
        {
            continue;
        }

        std::vector<std::string> negations = mod->describe(ModDescriptionType::Negation);
        groups["No"].insert(groups["No"].end(), negations.begin(), negations.end());

        std::vector<std::string> randomized = mod->describe(ModDescriptionType::Randomized);
        groups["Randomized"].insert(groups["Randomized"].end(), randomized.begin(), randomized.end());

        std::vector<std::string> multipliers = mod->describe(ModDescriptionType::Multiplier);
        groups["Multipliers"].insert(groups["Multipliers"].end(), multipliers.begin(), multipliers.end());

        std::vector<std::string> uniques = mod->describe(ModDescriptionType::Unique);
        groups["Unique"].insert(groups["Unique"].end(), uniques.begin(), uniques.end());

        std::vector<std::string> bannedItems = mod->describe(ModDescriptionType::BanItems);
        groups["BanItems"].insert(groups["BanItems"].end(), bannedItems.begin(), bannedItems.end());

        std::vector<std::string> bannedMateria = mod->describe(ModDescriptionType::BanMateria);
        groups["BanMateria"].insert(groups["BanMateria"].end(), bannedMateria.begin(), bannedMateria.end());
    }

    std::stringstream ss;
    if (groups["BanItems"].size() > 0 || groups["BanMateria"].size() > 0)
    {
        bool banItems = groups["BanItems"].size() > 0;
        bool banMateria = groups["BanMateria"].size() > 0;

        if (banItems)
        {
            auto& subjects = groups["BanItems"];
            std::sort(subjects.begin(), subjects.end());

            ss << "- Ban ";
            for (size_t i = 0; i < subjects.size(); ++i)
            {
                std::string subject = subjects[i];
                std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
                ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
            }
            if (banMateria)
            {
                ss << ".";
            }
            else
            {
                ss << ".\n";
            }
        }

        if (banMateria)
        {
            auto& subjects = groups["BanMateria"];
            std::sort(subjects.begin(), subjects.end());

            if (banItems)
            {
                ss << " Ban ";
            }
            else
            {
                ss << "- Ban ";
            }

            for (size_t i = 0; i < subjects.size(); ++i)
            {
                std::string subject = subjects[i];
                std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
                ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
            }
            ss << " materia.\n";
        }
    }

    if (groups["No"].size() > 0)
    {
        auto& subjects = groups["No"];
        std::sort(subjects.begin(), subjects.end());

        ss << "- No ";
        for (size_t i = 0; i < subjects.size(); ++i)
        {
            std::string subject = subjects[i];
            std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
            ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " or " : ", "));
        }
        ss << ".\n";
    }

    if (groups["Randomized"].size() > 0)
    {
        auto& subjects = groups["Randomized"];
        std::sort(subjects.begin(), subjects.end());

        ss << "- Randomized ";
        for (size_t i = 0; i < subjects.size(); ++i)
        {
            std::string subject = subjects[i];
            std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
            ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
        }
        ss << ".\n";
    }

    if (groups["Multipliers"].size() > 0)
    {
        auto& subjects = groups["Multipliers"];

        ss << "- ";
        for (size_t i = 0; i < subjects.size(); ++i)
        {
            std::string subject = subjects[i];
            std::transform(subject.begin(), subject.end(), subject.begin(), [](unsigned char c) { return std::tolower(c); });
            ss << subject << (i == subjects.size() - 1 ? "" : (i == subjects.size() - 2 ? " and " : ", "));
        }
        ss << ".\n";
    }

    if (groups["Unique"].size() > 0)
    {
        auto& subjects = groups["Unique"];

        for (size_t i = 0; i < subjects.size(); ++i)
        {
            ss << "- " << subjects[i] << ".\n";
        }
    }

    return ss.str();
}
