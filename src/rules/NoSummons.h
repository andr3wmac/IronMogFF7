#pragma once
#include "Rule.h"
#include <cstdint>

class NoSummons : public Rule
{
public:
    void setup() override;

private:
    void onFrame(uint32_t frameNumber);

    bool needsFieldChecks = false;
    bool needsShopChecks = false;
};