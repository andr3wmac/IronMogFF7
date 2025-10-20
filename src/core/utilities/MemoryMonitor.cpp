#include "MemoryMonitor.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Utilities.h"

const uintptr_t PS1RAMSize = 0x200000; // 2 MB

MemoryMonitor::MemoryMonitor(GameManager* gameManager)
    : game(gameManager)
{
    regionData[0] = new uint8_t[PS1RAMSize];
    regionData[1] = new uint8_t[PS1RAMSize];
    regionBegin = 0;
    regionSize = 0x200000;
    firstUpdate = true;

    BIND_EVENT(gameManager->onUpdate, MemoryMonitor::onUpdate);
}

MemoryMonitor::~MemoryMonitor()
{
    if (regionData[0] != nullptr)
    {
        delete[] regionData[0];
    }
    if (regionData[1] != nullptr)
    {
        delete[] regionData[1];
    }
}

void MemoryMonitor::setRegion(uintptr_t newRegionBegin, size_t newRegionSize)
{
    regionBegin = newRegionBegin;
    regionSize = newRegionSize;
}

void MemoryMonitor::setMonitor(size_t newMonitorStride, std::function<void(uint8_t*, uint8_t*, int)> newMonitorFunc)
{
    monitorStride = newMonitorStride;
    monitorFunc = newMonitorFunc;
}

void MemoryMonitor::onUpdate()
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

    for (int i = regionBegin; i < (regionBegin + regionSize); i += monitorStride)
    {
        for (int j = 0; j < monitorStride; ++j)
        {
            if (previousData[i + j] != latestData[i + j])
            {
                monitorFunc(previousData, latestData, i);
                break;
            }
        }
    }
}