#pragma once

#include "core/game/GameManager.h"
#include <string>
#include <vector>

template<typename T>
class MemoryMonitor
{
public:
    MemoryMonitor(GameManager* gameManager, uintptr_t newRegionBegin, size_t newRegionEnd, std::function<void(T, T, int)> newMonitorFunc)
    {
        game            = gameManager;
        regionData[0]   = new uint8_t[0x200000];
        regionData[1]   = new uint8_t[0x200000];
        regionBegin     = newRegionBegin;
        regionSize      = newRegionEnd - newRegionBegin;
        monitorFunc     = newMonitorFunc;

        firstUpdate = true;
        BIND_EVENT(gameManager->onUpdate, MemoryMonitor::onUpdate);
    }

    ~MemoryMonitor()
    {
        delete[] regionData[0];
        delete[] regionData[1];
    }

    void onUpdate()
    {
        if (firstUpdate)
        {
            game->read(regionBegin, regionSize, &regionData[curDataIdx][regionBegin]);
            game->read(regionBegin, regionSize, &regionData[prevDataIdx][regionBegin]);

            firstUpdate = false;
            return;
        }

        std::swap(prevDataIdx, curDataIdx);
        game->read(regionBegin, regionSize, &regionData[curDataIdx][regionBegin]);
        uint8_t* previousData = regionData[prevDataIdx];
        uint8_t* latestData = regionData[curDataIdx];

        for (int i = regionBegin; i < (regionBegin + regionSize); i += sizeof(T))
        {
            T* previousDataTyped = (T*)(&previousData[i]);
            T* latestDataTyped = (T*)(&latestData[i]);

            if (previousDataTyped[0] != latestDataTyped[0])
            {
                monitorFunc(previousDataTyped[0], latestDataTyped[0], i);
            }
        }
    }

private:
    GameManager* game;

    uintptr_t regionBegin;
    size_t regionSize;
    uint8_t* regionData[2];
    int curDataIdx = 0;
    int prevDataIdx = 1;
    bool firstUpdate = false;

    std::function<void(T, T, int)> monitorFunc;
};
