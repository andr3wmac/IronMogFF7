#pragma once
#include "Rule.h"
#include "core/game/GameData.h"
#include <cstdint>

class RandomizeFieldItems : public Rule
{
public:
    void setup() override;

private:
    void onStart();
    void onFieldChanged(uint16_t fieldID);

    // Shuffles items and materia between maps based on the game seed.
    void generateRandomizedItems();

    std::unordered_map<uint32_t, FieldItemData> randomizedItems;
    std::unordered_map<uint32_t, FieldItemData> randomizedMateria;
};