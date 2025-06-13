#pragma once
#include "Rule.h"
#include <cstdint>

class NoLimitBreaks : public Rule
{
public:
    void onStart() override;

private:
    void onFrame(uint32_t frameNumber);
};