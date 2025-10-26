#pragma once

#include "core/game/GameManager.h"
#include <string>
#include <vector>

class MemorySearch
{
public:
    MemorySearch(GameManager* gameManager);
    ~MemorySearch();

    void loadMemoryState(std::string inputFilePath, uintptr_t startRange, uintptr_t endRange);
    void saveMemoryState(std::string outputFilePath);

    // Search memory space for a contiguous block of bytes specified by searchBytes
    std::vector<uintptr_t> search(std::vector<uint8_t> searchBytes);

    // Search memory space for a value of particular type
    template<typename T>
    std::vector<uintptr_t> searchValue(T value)
    {
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        return search(bytes);
    }

    // Search for two bytes that are at most maxDistance apart in memory addresses
    std::vector<uintptr_t> searchForCloseValues(uint8_t first, uint8_t second, uintptr_t maxDistance);

    // Search for integer positions that are within maxDistance from the given position
    std::vector<uintptr_t> searchForClosePositions(int posX, int posY, int posZ, int maxDistance);

    // Checks a list of addresses and returns each one that has the searchBytes pattern of bytes at it
    std::vector<uintptr_t> checkAddresses(const std::vector<uintptr_t>& addresses, const std::vector<uint8_t>& searchBytes);

    // Search for polygon render commands where at least 2 occur in a row
    std::vector<uintptr_t> searchForPolygons();

    // Scan for clusters of polygon render commands that look like they're models
    void modelScan(uintptr_t startAddress, int maxSteps, bool verbose = false);

private:
    GameManager* game;
    uint8_t* RAMData;
};
