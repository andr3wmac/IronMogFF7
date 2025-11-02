#pragma once

#include "core/game/GameManager.h"
#include <set>
#include <string>
#include <vector>

// Monitors a given memory range for any changes to values of type T. Will then call monitorFunc to allow 
// you to inspect them in some way. Useful when you're looking for a value that changes into another 
// between save states or after pressing a button.
template<typename T>
class MemoryMonitor
{
public:
    MemoryMonitor(GameManager* gameManager, uintptr_t newRegionBegin, size_t newRegionEnd, std::function<void(T, T, int)> newMonitorFunc)
    {
        game          = gameManager;
        regionData[0] = new uint8_t[0x200000];
        regionData[1] = new uint8_t[0x200000];
        regionBegin   = newRegionBegin;
        regionSize    = newRegionEnd - newRegionBegin;
        monitorFunc   = newMonitorFunc;

        firstUpdate = true;
        BIND_EVENT(gameManager->onUpdate, MemoryMonitor::onUpdate);
    }

    ~MemoryMonitor()
    {
        delete[] regionData[0];
        delete[] regionData[1];
    }

    void addIgnoreAddresses(std::vector<uintptr_t>& addresses)
    {
        ignoreAddresses.insert(addresses.begin(), addresses.end());
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
                if (ignoreAddresses.count(i) > 0)
                {
                    continue;
                }

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

    std::set<uintptr_t> ignoreAddresses;
    std::function<void(T, T, int)> monitorFunc;
};

// The intent of this memory monitor is to provide a pattern check function that can compare the before and after
// values of memory in any way you want and the return -1 if its not what you're looking for, 0 if its neutral,
// and +1 if its the correct pattern. A message will be logged when a positive match is found matchThreshold times
// in a row without a negative result. This will, for instance, allow you to find descending or ascending variables
// over a number of frames or any other pattern of frame by frame changes to a variable.
template<typename T>
class PatternMemoryMonitor
{
public:
    PatternMemoryMonitor(GameManager* gameManager, uintptr_t newRegionBegin, size_t newRegionEnd, int newMatchThreshold, std::function<int(T, T)> newPatternCheckFunc)
    {
        game             = gameManager;
        regionData[0]    = new uint8_t[0x200000];
        regionData[1]    = new uint8_t[0x200000];
        regionBegin      = newRegionBegin;
        regionSize       = newRegionEnd - newRegionBegin;
        matchThreshold   = newMatchThreshold;
        patternCheckFunc = newPatternCheckFunc;

        firstUpdate = true;
        BIND_EVENT_ONE_ARG(gameManager->onFrame, PatternMemoryMonitor::onFrame);
    }

    ~PatternMemoryMonitor()
    {
        delete[] regionData[0];
        delete[] regionData[1];
    }

    void addIgnoreAddresses(std::vector<uintptr_t>& addresses)
    {
        ignoreAddresses.insert(addresses.begin(), addresses.end());
    }

    void onFrame(int frameNumber)
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
                if (ignoreAddresses.count(i) > 0)
                {
                    continue;
                }

                // -1 means pattern failed, 0 is neutral/ignore, 1 is pattern match.
                int checkResult = patternCheckFunc(previousDataTyped[0], latestDataTyped[0]);

                if (checkResult > 0)
                {
                    monitoredAddresses[i] += 1;
                }
                else if (checkResult < 0)
                {
                    monitoredAddresses[i] = 0;
                }

                if (monitoredAddresses[i] >= matchThreshold)
                {
                    LOG("Pattern match found at %d, last value: %d", i, latestDataTyped[0]);
                    ignoreAddresses.insert(i);
                }
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
    int matchThreshold;

    std::unordered_map<uintptr_t, int> monitoredAddresses;
    std::set<uintptr_t> ignoreAddresses;
    std::function<int(T, T)> patternCheckFunc;
};
