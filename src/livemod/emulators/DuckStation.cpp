#include "DuckStation.h"
#include "livemod/game/GameManager.h"
#include "livemod/game/MemoryOffsets.h"
#include "livemod/utilities/Logging.h"
#include "livemod/utilities/Platform.h"

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

bool DuckStation::resolveMemory()
{
    // DuckStation allocates PS1 RAM as a section object, so we can try to map it directly
    // into our address space for zero-copy reads and writes.
    ps1MappedView = Platform::mapSharedSection(processHandle, 0x200000,
        [](void* view, size_t) {
            for (const auto& [offset, expected] : Emulator::ps1MemoryChecks)
            {
                if (*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(view) + offset) != expected)
                    return false;
            }
            return true;
        });

    if (ps1MappedView)
    {
        return true;
    }

    // Fall back to scanning for the base address if the section mapping didn't work.

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

    LOG("Searching for DuckStation PS1 Memory Offset..");

    uintptr_t startAddr = 0;
    uintptr_t endAddr = 0;
    Platform::getApplicationAddressRange(startAddr, endAddr);

    Platform::MemoryRegion memRegion;
    std::vector<uint8_t> buffer;
    uintptr_t candidate;

    while (startAddr < endAddr)
    {
        if (!Platform::openMemoryRegion(processHandle, startAddr, memRegion))
        {
            break;
        }

        if (memRegion.isReadable && memRegion.isWritable && !memRegion.isGuarded)
        {
            buffer.resize(memRegion.size);

            if (Platform::read(processHandle, memRegion.baseAddress, buffer.data(), memRegion.size))
            {
                // The two pointers (g_ram and g_unprotected_ram) will occur in the same memory block,
                // we get fewer false positives by counting within the block.
                std::unordered_map<uintptr_t, int> subAddressCounter;

                for (size_t i = 0; i + sizeof(uintptr_t) <= memRegion.size; i += sizeof(uintptr_t))
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
                            ps1BaseAddress = pair.first;
                            return true;
                        }
                    }
                }
            }
        }

        startAddr += memRegion.size;
    }

    return false;
}