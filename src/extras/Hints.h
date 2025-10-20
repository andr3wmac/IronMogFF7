#pragma once
#include "extras/Extra.h"
#include <cstdint>
#include <unordered_map>

class Hints : public Extra
{
public:
    void setup() override;

private:
    void onStart();
    void onFieldChanged(uint16_t fieldID);
};