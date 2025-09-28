#include "MemorySearch.h"
#include "core/utilities/Logging.h"
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

std::vector<uintptr_t> MemorySearch::searchForClosePositions(int posX, int posY, int posZ, int maxDistance)
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

    size_t positionStride = sizeof(uint32_t) * 3; // XYZ
    for (uintptr_t i = 0; i < (PS1RAMSize - positionStride); ++i)
    {
        uint32_t x;
        int32_t y;
        uint32_t z;
        memcpy(&x, &RAMData[i], sizeof(uint32_t));
        memcpy(&y, &RAMData[i + 4], sizeof(int32_t));
        memcpy(&z, &RAMData[i + 8], sizeof(uint32_t));

        if (std::abs(posX - (int)x) > maxDistance || std::abs(posY - y) > maxDistance || std::abs(posZ - (int)z) > maxDistance)
        {
            continue;
        }

        if (x == posX && y == posY && z == posZ)
        {
            // Skip exact matches for now.
            continue;
        }

        LOG("Potential Match: %d %d %d at %d", x, y, z, i);

        results.push_back(i);
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

void MemorySearch::loadMemoryState(std::string inputFilePath, uintptr_t startRange, uintptr_t endRange)
{
    if (game == nullptr)
    {
        return;
    }

    uint8_t* data = Utilities::loadArrayFromFile<uint8_t>(inputFilePath);
    for (uintptr_t addr = startRange; addr <= endRange; addr++)
    {
        game->write<uint8_t>(addr, data[addr]);
    }
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
