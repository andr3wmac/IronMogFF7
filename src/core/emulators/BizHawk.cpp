#include "BizHawk.h"
#include "core/game/GameManager.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Platform.h"

uintptr_t BizHawk::getPS1MemoryOffset()
{
    uintptr_t finalAddress = 0;

    // If its using the octoshock core we can find it faster by only searching
    // that DLL's memory space.
    Platform::ProcessLibrary library;
    if (Platform::findProcessLibrary(processHandle, "octoshock.dll", library))
    {
        finalAddress = findPS1MemoryOffset(library.baseAddr, library.baseAddr + library.size);
    }

    // If all else fails, search entire memory space (slower)
    if (finalAddress == 0)
    {
        finalAddress = findPS1MemoryOffset(0, 0);
    }

    if (verifyPS1MemoryOffset(finalAddress))
    {
        return finalAddress;
    }

    return 0;
}

uintptr_t BizHawk::findPS1MemoryOffset(uintptr_t startAddr, uintptr_t endAddr)
{
    LOG("Searching for BizHawk PS1 Memory Offset..");

    // If these are empty we use the total application space.
    if (startAddr == 0 && endAddr == 0)
    {
        Platform::getApplicationAddressRange(startAddr, endAddr);
    }

    Platform::MemoryRegion memRegion;
    std::vector<uint8_t> buffer;
    uint32_t candidate;

    while (startAddr < endAddr)
    {
        if (!Platform::openMemoryRegion(processHandle, startAddr, memRegion))
        {
            break;
        }

        if (memRegion.isReadable && memRegion.isWritable && !memRegion.isGuarded)
        {
            buffer.resize(memRegion.size);

            if (Platform::read(processHandle, memRegion.baseAddr, buffer.data(), memRegion.size))
            {
                for (size_t i = 0; i + sizeof(uint32_t) <= memRegion.size; i += sizeof(uint32_t))
                {
                    memcpy(&candidate, buffer.data() + i, sizeof(uint32_t));

                    if (candidate == Emulator::ps1MemoryChecks[0].second)
                    {
                        uintptr_t address = memRegion.baseAddr + i - Emulator::ps1MemoryChecks[0].first;
                        if (verifyPS1MemoryOffset(address))
                        {
                            return address;
                        }
                    }
                }
            }
        }

        startAddr += memRegion.size;
    }

    return 0;
}