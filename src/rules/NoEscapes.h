#pragma once
#include "Rule.h"
#include <cstdint>

class NoEscapes : public Rule
{
public:
    void setup() override;

private:
    void onBattleEnter();
};