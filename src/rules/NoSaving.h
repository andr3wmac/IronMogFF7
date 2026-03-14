#pragma once
#include "Rule.h"
#include <cstdint>

class NoSaving : public Rule
{
public:
    void setup() override;
    std::vector<std::string> describe(RuleDescripionType descType) override;

private:
    void onFrame(uint32_t frameNumber);
};