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
    delete[] data;
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

std::vector<uintptr_t> MemorySearch::searchForPolygons()
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

    for (uintptr_t i = 0; i < (PS1RAMSize - 36); ++i)
    {
        uint32_t word0;
        memcpy(&word0, &RAMData[i], sizeof(uint32_t));
        uint8_t cmd0 = (word0 >> 24) & 0xFF;

        // polygon, gouraud, quad, textured
        if (cmd0 == 0x3C)
        {
            uint32_t word1;
            memcpy(&word1, &RAMData[i + 52], sizeof(uint32_t));
            uint8_t cmd1 = (word1 >> 24) & 0xFF;

            if (cmd1 == 0x3C || cmd1 == 0x34 || cmd1 == 0x30 || cmd1 == 0x38)
            {
                results.push_back(i);
            }
        }

        // polygon, gouraud, tri, textured
        if (cmd0 == 0x34)
        {
            uint32_t word1;
            memcpy(&word1, &RAMData[i + 40], sizeof(uint32_t));
            uint8_t cmd1 = (word1 >> 24) & 0xFF;

            if (cmd1 == 0x3C || cmd1 == 0x34 || cmd1 == 0x30 || cmd1 == 0x38)
            {
                results.push_back(i);
            }
        }

        // polygon, gouraud, tri
        if (cmd0 == 0x30)
        {
            uint32_t word1;
            memcpy(&word1, &RAMData[i + 28], sizeof(uint32_t));
            uint8_t cmd1 = (word1 >> 24) & 0xFF;

            if (cmd1 == 0x3C || cmd1 == 0x34 || cmd1 == 0x30 || cmd1 == 0x38)
            {
                results.push_back(i);
            }
        }

        // polygon, gouraud, quad 
        if (cmd0 == 0x38)
        {
            uint32_t word1;
            memcpy(&word1, &RAMData[i + 36], sizeof(uint32_t));
            uint8_t cmd1 = (word1 >> 24) & 0xFF;

            if (cmd1 == 0x3C || cmd1 == 0x34 || cmd1 == 0x30 || cmd1 == 0x38)
            {
                results.push_back(i);
            }
        }
    }

    return results;
}

std::string getOpCodeName(int curType)
{
    if (curType == 0x3C) return "QUAD TEX";
    if (curType == 0x34) return "TRI TEX";
    if (curType == 0x30) return "TRI COLOR";
    if (curType == 0x38) return "QUAD COLOR";

    return "UNKNOWN";
}

void MemorySearch::modelScan(uintptr_t startAddress, int maxSteps, bool verbose)
{
    uintptr_t curAddr = startAddress;
    uintptr_t startAddr = 0;

    int polygonCount = 0;
    int nonPolygonCount = 0;
    int curType = 0;
    int curCount = 0;

    for (int i = 0; i < maxSteps; ++i)
    {
        uint32_t data0 = game->read<uint32_t>(curAddr);

        uint8_t cmd3 = (data0 >> 0) & 0xFF;
        uint8_t cmd2 = (data0 >> 8) & 0xFF;
        uint8_t cmd1 = (data0 >> 16) & 0xFF;
        uint8_t cmd0 = (data0 >> 24) & 0xFF;

        if (polygonCount == 0)
        {
            curType = 0;
            curCount = 0;
        }

        // polygon, gouraud, quad, textured
        if (cmd0 == 0x3C)
        {
            if (curType != 0x3C && curType != 0)
            {
                LOG("%s ended with %d", getOpCodeName(curType).c_str(), curCount);
                curCount = 0;
            }

            uintptr_t cmdAddr = curAddr;
            if (polygonCount == 0) startAddr = curAddr;
            curAddr += 13 * 4;
            polygonCount++;
            curType = 0x3C;
            curCount++;

            if (verbose)
            {
                LOG("QUAD TEX %d %d %d Addr: %d", cmd3, cmd2, cmd1, cmdAddr);
            }
            
            continue;
        }

        // polygon, gouraud, tri, textured
        if (cmd0 == 0x34)
        {
            if (curType != 0x34 && curType != 0)
            {
                LOG("%s ended with %d", getOpCodeName(curType).c_str(), curCount);
                curCount = 0;
            }

            uintptr_t cmdAddr = curAddr;
            if (polygonCount == 0) startAddr = curAddr;
            curAddr += 10 * 4;
            polygonCount++;
            curType = 0x34;
            curCount++;

            if (verbose)
            {
                LOG("TRI TEX %d %d %d Addr: %d", cmd3, cmd2, cmd1, cmdAddr);
            }

            continue;
        }

        // polygon, gouraud, tri
        if (cmd0 == 0x30)
        {
            if (curType != 0x30 && curType != 0)
            {
                LOG("%s ended with %d", getOpCodeName(curType).c_str(), curCount);
                curCount = 0;
            }

            uintptr_t cmdAddr = curAddr;
            if (polygonCount == 0) startAddr = curAddr;
            curAddr += 7 * 4;
            polygonCount++;
            curType = 0x30;
            curCount++;

            if (verbose)
            {
                LOG("TRI COLOR %d %d %d Addr: %d", cmd3, cmd2, cmd1, cmdAddr);
            }

            continue;
        }

        // polygon, gouraud, quad 
        if (cmd0 == 0x38)
        {
            if (curType != 0x38 && curType != 0)
            {
                LOG("%s ended with %d", getOpCodeName(curType).c_str(), curCount);
                curCount = 0;
            }

            uintptr_t cmdAddr = curAddr;
            if (polygonCount == 0) startAddr = curAddr;
            curAddr += 9 * 4;
            polygonCount++;
            curType = 0x38;
            curCount++;

            if (verbose)
            {
                LOG("QUAD COLOR %d %d %d Addr: %d", cmd3, cmd2, cmd1, cmdAddr);
            }
            continue;
        }

        if (curType != 0)
        {
            LOG("%s ended with %d", getOpCodeName(curType).c_str(), curCount);
            curCount = 0;
        }

        if (verbose)
        {
            uint8_t polygon_render = (cmd0 >> 5) & 0b111;
            uint8_t line_render = (cmd0 >> 5) & 0b010;
            uint8_t rect_render = (cmd0 >> 5) & 0b011;

            if (polygon_render == 1)
            {
                uint8_t gouraud_flag = (cmd0 >> 4) & 0b1;   // bit 28
                uint8_t quad_flag = (cmd0 >> 3) & 0b1;      // bit 27
                uint8_t textured_flag = (cmd0 >> 2) & 0b1;  // bit 26
                uint8_t semi_transp = (cmd0 >> 1) & 0b1;    // bit 25
                uint8_t raw_tex_mod = cmd0 & 0b1;           // bit 24

                LOG("Unknown polygon after %d polygons. Address: %d, Cmd: %d, Flags: %d %d %d %d %d", polygonCount, curAddr, cmd0, gouraud_flag, quad_flag, textured_flag, semi_transp, raw_tex_mod);
            }
            else if (line_render == 1)
            {
                LOG("Line render after %d polygons. Address: %d", polygonCount, curAddr);
            }
            else if (rect_render == 1)
            {
                LOG("Rect render after %d polygons. Address: %d", polygonCount, curAddr);
            }
            else
            {
                LOG("Unknown data reached after %d polygons. Address: %d, Data: 0x%02X (%d) 0x%02X (%d) 0x%02X (%d) 0x%02X (%d)", polygonCount, curAddr, cmd3, cmd3, cmd2, cmd2, cmd1, cmd1, cmd0, cmd0);
            }
        }

        curAddr += 4;

        if (polygonCount > 0 && polygonCount < 4)
        {
            LOG("Small polygon count model found, discarding as false positive.");
            curAddr = startAddr + 4;
        }
        else if (polygonCount > 4)
        {
            LOG("MODEL FOUND. Address: %d Poly Count: %d", startAddr, polygonCount);
        }
        polygonCount = 0;
    }

    LOG("Finished scan with %d last found polygons.", polygonCount);
}