#pragma once
#include "Rule.h"
#include <cstdint>

class NoEscapes : public Rule
{
public:
    void setup() override;
    std::vector<std::string> describe(RuleDescripionType descType) override;

private:
    void onBattleEnter();
};