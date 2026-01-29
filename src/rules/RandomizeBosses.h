#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>
#include <random>

class RandomizeBosses : public Rule
{
public:
    RandomizeBosses();

    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    enum class RandomMode : int
    {
        Shuffle = 0,
        WeightedRandom = 1
    };

    float statMultiplier = 1.0f;

    RandomMode randomMode = RandomMode::Shuffle;
    std::mt19937_64 rng;
    int elementCount = 7;
    std::vector<std::string> randomNames;
    std::vector<int> randomWeights;
    std::unordered_map<uint16_t, Boss> shuffledBosses;

    void onStart();
    void generateShuffledBosses();
    std::pair<uint64_t, uint64_t> getWeightedRandomElements(uint16_t bossID);
    void onBattleEnter();
};