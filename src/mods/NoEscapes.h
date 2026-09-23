#pragma once
#include "Mod.h"
#include <cstdint>

class NoEscapes : public Mod
{
public:
    void setup() override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    void onBattleEnter();
};