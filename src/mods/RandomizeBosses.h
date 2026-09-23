#pragma once
#include "Mod.h"
#include "LiveModFF7Core/game/GameData.h"
#include <cstdint>
#include <random>

class RandomizeBosses : public Mod
{
public:
    std::string getDescription() const override;

    RandomizeBosses();

    void setup() override;
    bool hasSettings() override { return true; }
    bool onSettingsGUI() override;
    void loadSettings(const ConfigFile& cfg) override;
    void saveSettings(ConfigFile& cfg) override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    enum class RandomMode : int
    {
        Shuffle = 0,
        WeightedRandom = 1
    };

    void onStart();
    void generateShuffledBosses();
    void generateBossStatMultipliers();
    std::pair<uint64_t, uint64_t> getWeightedRandomElements(uint16_t bossID);
    void onBattleEnter();
    void onBattleTransition(uint16_t newFormationID);
    void onDifficultyScaleChanged(float newDifficultyScale);

    void applyBossRandomization();

    float minStatMultiplier = 1.0f;
    float maxStatMultiplier = 1.0f;
    bool defenseSoftCap = true;

    RandomMode randomMode = RandomMode::Shuffle;
    std::mt19937_64 rng;
    int elementCount = 7;
    std::vector<std::string> randomNames;
    std::vector<int> randomWeights;
    std::unordered_map<uint16_t, Boss> shuffledBosses;
    std::unordered_map<uint16_t, StatMultiplierSet> bossStatMultipliers;
};