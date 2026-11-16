#pragma once

#include "Mod.h"

#include <array>
#include <cstdint>

class RandomizeCharacters : public Mod
{
public:
    // Off unless a settings file turns it on, so existing settings files don't pick it up.
    RandomizeCharacters() { enabled = false; }

    std::string getDescription() const override;

    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    void onStart();
    void onLoadUpdate();
    void applyShuffle();
    void clearShuffle();

    bool applied = false;
    uint32_t reroll = 0;                    // Debug only: offsets the seed to try other mappings.
    std::array<uint8_t, 9> appearance{};    // Character each character appears as, by character ID.
};
