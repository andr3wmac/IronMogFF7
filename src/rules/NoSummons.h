#pragma once
#include "Rule.h"
#include <cstdint>

class NoSummons : public Rule
{
public:
    void setup() override;

private:
    void onFieldChanged(uint16_t fieldID);
    void onShopOpened();

    uint16_t replaceSummonMateria(uint16_t materiaID);

    bool needsFieldChecks = false;
    bool needsShopChecks = false;
};