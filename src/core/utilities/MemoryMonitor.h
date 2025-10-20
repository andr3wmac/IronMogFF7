#pragma once

#include "core/game/GameManager.h"
#include <string>
#include <vector>

class MemoryMonitor
{
public:
    MemoryMonitor(GameManager* gameManager);
    ~MemoryMonitor();

    void setRegion(uintptr_t newRegionBegin, size_t newRegionSize);
    void setMonitor(size_t newMonitorStride, std::function<void(uint8_t*, uint8_t*, int)> newMonitorFunc);
    void onUpdate();

private:
    GameManager* game;

    uintptr_t regionBegin;
    size_t regionSize;
    uint8_t* regionData[2];
    int curDataIdx = 0;
    int prevDataIdx = 1;
    bool firstUpdate = false;

    size_t monitorStride = 1;
    std::function<void(uint8_t*, uint8_t*, int)> monitorFunc;
};
