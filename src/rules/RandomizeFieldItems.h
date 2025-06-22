#pragma once
#include "Rule.h"
#include "game/GameData.h"
#include <cstdint>

class RandomizeFieldItems : public Rule
{
public:
    void onStart() override;

private:
    void onFieldChanged(uint16_t fieldID);
    void onFrame(uint32_t frameNumber);
    void onBattleExit();

    void generateRandomizedItems();
    void randomizeFieldItems(uint16_t fieldID);

    std::unordered_map<uint32_t, FieldItemData> randomizedItems;
    std::unordered_map<uint32_t, FieldItemData> randomizedMateria;
    bool fieldNeedsRandomize = false;
};