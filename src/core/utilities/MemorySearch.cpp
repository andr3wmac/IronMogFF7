#include "MemorySearch.h"
#include "core/utilities/Utilities.h"

const uintptr_t PS1RAMSize = 0x200000; // 2 MB

MemorySearch::MemorySearch(GameManager* gameManager)
    : game(gameManager)
    , RAMData(nullptr)
{
    RAMData = new uint8_t[PS1RAMSize];
}

MemorySearch::~MemorySearch()
{
    if (RAMData != nullptr)
    {
        delete[] RAMData;
    }
}

// Searches for a sequence of bytes that matches searchBytes.
std::vector<uintptr_t> MemorySearch::search(std::vector<uint8_t> searchBytes)
{
    std::vector<uintptr_t> results;

    if (game == nullptr)
    {
        return results;
    }

    // Copy PS1 RAM into temporary storage
    if (!game->read(0, PS1RAMSize, RAMData))
    {
        return results;
    }

    // Search for pattern
    for (uintptr_t i = 0; i <= PS1RAMSize - searchBytes.size(); ++i)
    {
        bool match = true;
        for (size_t j = 0; j < searchBytes.size(); ++j)
        {
            if (RAMData[i + j] != searchBytes[j])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            results.push_back(i);
        }
    }

    // Return all addresses that match the pattern.
    return results;
}

// Searches the memory space for two bytes that are within maxDistance from each other.
std::vector<uintptr_t> MemorySearch::searchForCloseValues(uint8_t first, uint8_t second, uintptr_t maxDistance)
{
    std::vector<uintptr_t> results;

    if (game == nullptr)
    {
        return results;
    }

    // Copy PS1 RAM into temporary storage
    if (!game->read(0, PS1RAMSize, RAMData))
    {
        return results;
    }

    // Search for pattern
    uintptr_t lastFirstPosition = 0;
    uintptr_t lastSecondPosition = 0;
    for (uintptr_t i = 0; i <= PS1RAMSize; ++i)
    {
        bool checkValues = false;

        if (RAMData[i] == first)
        {
            lastFirstPosition = i;
            checkValues = true;
        }

        if (RAMData[i] == second)
        {
            lastSecondPosition = i;
            checkValues = true;
        }

        if (checkValues && lastFirstPosition != 0 && lastSecondPosition != 0)
        {
            if (std::abs((int)(lastFirstPosition - lastSecondPosition)) < maxDistance)
            {
                results.push_back(lastFirstPosition);
            }
        }
    }

    return results;
}

std::vector<uintptr_t> MemorySearch::checkAddresses(const std::vector<uintptr_t>& addresses, const std::vector<uint8_t>& searchBytes)
{
    std::vector<uintptr_t> results;

    if (game == nullptr || searchBytes.empty())
    {
        return results;
    }

    // Copy PS1 RAM into temporary storage
    if (!game->read(0, PS1RAMSize, RAMData))
    {
        return results;
    }

    for (uintptr_t address : addresses)
    {
        // Skip out-of-bounds checks
        if (address + searchBytes.size() > PS1RAMSize)
        {
            continue;
        }

        bool match = true;
        for (size_t i = 0; i < searchBytes.size(); ++i)
        {
            if (RAMData[address + i] != searchBytes[i])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            results.push_back(address);
        }
    }

    return results;
}

void MemorySearch::saveMemoryState(std::string outputFilePath)
{
    if (game == nullptr)
    {
        return;
    }

    // Copy PS1 RAM into temporary storage
    if (!game->read(0, PS1RAMSize, RAMData))
    {
        return;
    }

    Utilities::saveArrayToFile<uint8_t>(RAMData, PS1RAMSize, outputFilePath);
}
