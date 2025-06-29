#pragma once
#include "Rule.h"
#include <cstdint>
#include <random>

class RandomizeShops : public Rule
{
public:
    void setup() override;

private:
    void onFieldChanged(uint16_t fieldID);
    void onShopOpened();

    uint16_t randomizeShopItem(uint16_t itemID);
    uint16_t randomizeShopMateria(uint16_t materiaID);

    std::mt19937_64 rng;
    uint16_t lastFieldID = 0;
};