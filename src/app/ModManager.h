#pragma once

#include <string>

class GameManager;
class Mod;

// Manages the application's mods without adding mod-specific knowledge to GameManager.
namespace ModManager
{
    // Call after GameManager::setup() so the seed is set before any mod runs.
    void setup(GameManager* game);

    bool isModEnabled(const std::string& modName);
    Mod* getMod(const std::string& modName);

    // Builds the grouped mod summary (Ban / No / Randomized / Multipliers / Unique).
    std::string getSettingsSummary();
}
