#pragma once
#include "Mod.h"
#include <cstdint>

class NoEscapes : public Mod
{
public:
    std::string getDescription() const override;

    void setup() override;
    std::vector<std::string> describe(ModDescriptionType descType) override;

private:
    void onBattleEnter();
};