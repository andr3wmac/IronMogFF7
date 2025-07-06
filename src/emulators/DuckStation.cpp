#include "DuckStation.h"
#include "game/GameManager.h"
#include "game/MemoryOffsets.h"
#include "utilities/Logging.h"

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <unordered_map>

bool isPossiblePointer(uintptr_t value) 
{
    // Pointers on 64-bit systems are usually 8-byte aligned
    if (value % 8 != 0) return false;

    // Plausible address range for heap allocations
    constexpr uint64_t MIN_VALID_PTR = 0x0000010000000000; // Reasonable low bound (usually above NULL, stack, etc)
    constexpr uint64_t MAX_VALID_PTR = 0x00007FFFFFFFFFFF; // User-mode address limit on Windows
    if (value < MIN_VALID_PTR || value > MAX_VALID_PTR) return false;

    return true;
}

// DuckStation dynamically allocates the heap space for the PS1 ram, and it keeps two variables that track that
// allocation, g_ram and g_unprotected_ram. Those two variables are defined globally and declared in the same
// compilation unit. 

// The strategy here is to search the process memory space for two matching 8 byte variables that also match some 
// heuristics that make them likely to be heap pointers. Looking for exactly two instances of this narrows the list 
// down quite a bit but still has a lot of false positives. Lastly, we check some specific spots offset from the
// potential address that seem to be unique static values when Final Fantasy 7 is loaded. 

// Surprisingly, this seems to narrow the list to two possibilities not one. The second instance might be another 
// copy of PS1 RAM that duckstation keeps. However, based on testing a few builds of duckstation it appears the
// first result found is the one we're looking for.

uintptr_t DuckStation::getPS1MemoryOffset()
{
    LOG("Searching for DuckStation PS1 Memory Offset..");

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    uintptr_t startAddr = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    uintptr_t endAddr = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T bytesRead;
    std::vector<uint8_t> buffer;
    uintptr_t candidate;

    while (startAddr < endAddr)
    {
        if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(startAddr), &mbi, sizeof(mbi)) == 0)
            break;

        // Only scan committed, readable memory
        bool isReadable =
            (mbi.State == MEM_COMMIT) &&
            ((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_READONLY) || (mbi.Protect & PAGE_WRITECOPY));

        if (isReadable && !(mbi.Protect & PAGE_GUARD))
        {
            buffer.resize(mbi.RegionSize);

            if (ReadProcessMemory(processHandle, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead))
            {
                // The two pointers (g_ram and g_unprotected_ram) will occur in the same memory block,
                // we get fewer false positives by counting within the block.
                std::unordered_map<uintptr_t, int> subAddressCounter;

                for (size_t i = 0; i + sizeof(uintptr_t) <= bytesRead; i += sizeof(uintptr_t))
                {
                    memcpy(&candidate, &buffer[i], sizeof(uintptr_t));

                    if (isPossiblePointer(candidate))
                    {
                        subAddressCounter[candidate]++;
                    }
                }

                for (const auto& pair : subAddressCounter)
                {
                    if (pair.second == 2)
                    {
                        if (verifyPS1MemoryOffset(pair.first))
                        {
                            return pair.first;
                        }
                    }
                }
            }
        }

        startAddr += mbi.RegionSize;
    }

    return 0;
}