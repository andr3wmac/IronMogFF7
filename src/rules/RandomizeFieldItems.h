#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>

class RandomizeFieldItems : public Rule
{
public:
    void setup() override;
    bool hasDebugGUI() override { return true; }
    void onDebugGUI() override;

private:
    void onStart();
    void onFrame(uint32_t frameNumber);
    void onFieldChanged(uint16_t fieldID);

    // Shuffles items and materia between maps based on the game seed.
    void generateRandomizedItems();

    // Applies randomization to current field.
    void apply();

    std::unordered_map<uint32_t, FieldItemData> randomizedItems;
    std::unordered_map<uint32_t, FieldItemData> randomizedMateria;
};