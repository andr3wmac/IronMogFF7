#pragma once

#include <string>

class GameManager;
class Rule;
class Extra;

// Owns IronMog's rule/extra lifecycle and queries. The engine (GameManager) has no knowledge of
// rules or extras; this is the IronMog-side orchestration that drives their setup and exposes
// lookups over the static Rule/Extra registries.
namespace RuleManager
{
    // Sets up all enabled rules and extras (wires their GameManager pointer and calls setup()).
    // Call after GameManager::setup() so the seed is set before any rule runs.
    void setup(GameManager* game);

    bool isRuleEnabled(const std::string& ruleName);
    Rule* getRule(const std::string& ruleName);
    bool isExtraEnabled(const std::string& extraName);
    Extra* getExtra(const std::string& extraName);

    // Builds IronMog's grouped challenge summary (Ban / No / Randomized / Multipliers / Unique).
    std::string getSettingsSummary();
}
