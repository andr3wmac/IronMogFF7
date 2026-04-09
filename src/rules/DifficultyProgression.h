#pragma once
#include "Rule.h"
#include <cstdint>

class DifficultyProgression : public Rule
{
public:
    enum class ProgressionSource : uint8_t
    {
        GameProgress = 0,
        HighestLevel = 1
    };

    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    std::vector<std::string> describe(RuleDescripionType descType) override;

private:
    void onGameMomentChanged(uint16_t gameMoment);
    void onBattleExit();

    ProgressionSource progressionSource = ProgressionSource::GameProgress;
    int progressionEnd = 0;
    int progressionEndLevel = 30;

    int lastMaxLevel = 0;
};