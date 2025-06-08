#pragma once
#include "Feature.h"
#include <cstdint>

class DisableLimitBreaks : public Feature
{
public:
    void onEnable() override;
    void onDisable() override;

private:
    void onFrame(uint32_t frameNumber);
};