#pragma once
#include "Rule.h"
#include <cstdint>

class NoSummons : public Rule
{
public:
    void onStart() override;

private:
    void onFrame(uint32_t frameNumber);
};