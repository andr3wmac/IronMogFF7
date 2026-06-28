#include "RuleManager.h"
#include "LiveModFF7/game/GameManager.h"
#include "rules/Rule.h"
#include "extras/Extra.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <vector>

void RuleManager::setup(GameManager* game)
{
    for (Rule* rule : Rule::getList())
    {
        if (!rule->enabled)
        {
            continue;
        }
        rule->setManager(game);
        rule->setup();
    }

    for (Extra* extra : Extra::getList())
    {
        if (!extra->enabled)
        {
            continue;
        }
        extra->setManager(game);
        extra->setup();
    }
}

bool RuleManager::isRuleEnabled(const std::string& ruleName)
{
    for (Rule* rule : Rule::getList())
    {
        if (rule->enabled && rule->name == ruleName)
        {
            return true;
        }
    }

    return false;
}

Rule* RuleManager::getRule(const std::string& ruleName)
{
    for (Rule* rule : Rule::getList())
    {
        if (rule->enabled && rule->name == ruleName)
        {
            return rule;
        }
    }

    return nullptr;
}

bool RuleManager::isExtraEnabled(const std::string& extraName)
{
    for (Extra* extra : Extra::getList())
    {
        if (extra->enabled && extra->name == extraName)
        {
            return true;
        }
    }

    return false;
}

Extra* RuleManager::getExtra(const std::string& extraName)
{
    for (Extra* extra : Extra::getList())
    {
        if (extra->enabled && extra->name == extraName)
        {
            return extra;
        }
    }

    return nullptr;
}

std::string RuleManager::getSettingsSummary()
{
    std::map<std::string, std::vector<std::string>> groups;

    groups["No"] = {};
    groups["Randomized"] = {};
    groups["Multipliers"] = {};
    groups["Unique"] = {};
    groups["BanItems"] = {};
    groups["BanMateria"] = {};

    for (Rule* rule : Rule::getList())
    {
        if (!rule->enabled)
        {
            continue;
        }

        std::vector<std::string> negations = rule->describe(RuleDescripionType::Negation);
        groups["No"].insert(groups["No"].end(), negations.begin(), negations.end());

        std::vector<std::string> randomized = rule->describe(RuleDescripionType::Randomized);
        groups["Randomized"].insert(groups["Randomized"].end(), randomized.begin(), randomized.end());

        std::vector<std::string> multipliers = rule->describe(RuleDescripionType::Multiplier);
        groups["Multipliers"].insert(groups["Multipliers"].end(), multipliers.begin(), multipliers.end());

        std::vector<std::string> uniques = rule->describe(RuleDescripionType::Unique);
        groups["Unique"].insert(groups["Unique"].end(), uniques.begin(), uniques.end());

        std::vector<std::string> bannedItems = rule->describe(RuleDescripionType::BanItems);
        groups["BanItems"].insert(groups["BanItems"].end(), bannedItems.begin(), bannedItems.end());

        std::vector<std::string> bannedMateria = rule->describe(RuleDescripionType::BanMateria);
        groups["BanMateria"].insert(groups["BanMateria"].end(), bannedMateria.begin(), bannedMateria.end());
    }

    for (Extra* extra : Extra::getList())
    {
        if (!extra->enabled)
        {
            continue;
        }

        std::vector<std::string> randomized = extra->describe(ExtraDescripionType::Randomized);
        groups["Randomized"].insert(groups["Randomized"].end(), randomized.begin(), randomized.end());
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
