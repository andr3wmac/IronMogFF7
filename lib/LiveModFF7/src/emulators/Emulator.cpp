#include "LiveModFF7/emulators/Emulator.h"
#include "LiveModFF7/emulators/CustomEmulator.h"
#include "LiveModFF7/emulators/DuckStation.h"
#include "LiveModFF7/emulators/BizHawk.h"
#include "LiveModFF7/game/MemoryOffsets.h"
#include "LiveModFF7/utilities/Logging.h"
#include "LiveModFF7/utilities/Platform.h"
#include "LiveModFF7/utilities/Utilities.h"

// These values seem to be the same regardless of what game is loaded.
std::vector<std::pair<uintptr_t, uint32_t>> Emulator::ps1MemoryChecks = {
    {0x80, 1008336896},
    {0x84, 660212864},
    {0x88, 54525960}
};

uint8_t ff7Disc1ID[] = { 0x53, 0x43, 0x55, 0x53, 0x5F, 0x39, 0x34, 0x31, 0x2E, 0x36, 0x33 }; // SCUS_941.63
uint8_t ff7Disc2ID[] = { 0x53, 0x43, 0x55, 0x53, 0x5F, 0x39, 0x34, 0x31, 0x2E, 0x36, 0x34 }; // SCUS_941.64
uint8_t ff7Disc3ID[] = { 0x53, 0x43, 0x55, 0x53, 0x5F, 0x39, 0x34, 0x31, 0x2E, 0x36, 0x35 }; // SCUS_941.65

Emulator* Emulator::getEmulatorFromProcessName(std::string processName)
{
    if (Utilities::containsIgnoreCase(processName, "duckstation"))
    {
        return new DuckStation();
    }

    if (Utilities::containsIgnoreCase(processName, "emuhawk"))
    {
        return new BizHawk();
    }

    return nullptr;
}

Emulator* Emulator::getEmulatorCustom(std::string processName, uintptr_t memoryAddress)
{
    return new CustomEmulator(memoryAddress);
}

Emulator::Emulator()
    : processHandle(0), ps1BaseAddress(0)
{
}

Emulator::~Emulator()
{
    if (ps1MappedView)
    {
        Platform::unmapSharedSection(ps1MappedView);
    }

    if (processHandle != 0)
    {
        Platform::closeProcess(processHandle);
    }
}

bool Emulator::connect(std::string processName)
{
    uint32_t pid = Platform::getProcessIDByName(processName);
    if (pid == 0)
    {
        LOG("Emulator process not found.");
        return false;
    }

    processHandle = Platform::openProcess(pid);
    if (!processHandle)
    {
        LOG("Failed to open emulator process.");
        return false;
    }

    if (!resolveMemory())
    {
        LOG("Failed to locate PS1 memory.");
        return false;
    }

    // This is just to ensure we actually attached to the right memory. This will fail if the offset is wrong.
    uintptr_t fieldXOffset = FieldOffsets::FieldX;
    int32_t fieldX = 0;
    if (!read(fieldXOffset, &fieldX, sizeof(fieldX)))
    {
        LOG("Failed to read playstation memory.");
        return false;
    }

    if (ps1MappedView)
    {
        LOG("Successfully connected to emulator via shared section mapping.");
    }
    else
    {
        LOG("Successfully connected to emulator at: 0x%X", ps1BaseAddress);
    }

    return true;
}

bool Emulator::read(uintptr_t offset, void* outBuffer, size_t size)
{
    if (ps1MappedView)
    {
        memcpy(outBuffer, static_cast<uint8_t*>(ps1MappedView) + offset, size);
        return true;
    }

    if (!Platform::read(processHandle, ps1BaseAddress + offset, outBuffer, size))
    {
        readErrorCount++;
        LOG("Failed to read memory from: %d", offset);
        return false;
    }

    return true;
}

bool Emulator::write(uintptr_t offset, void* inValue, size_t size)
{
    if (ps1MappedView)
    {
        memcpy(static_cast<uint8_t*>(ps1MappedView) + offset, inValue, size);
        return true;
    }

    if (!Platform::write(processHandle, ps1BaseAddress + offset, inValue, size))
    {
        writeErrorCount++;
        LOG("Failed to write memory to: %d", offset);
        return false;
    }

    return true;
}

bool Emulator::verifyPS1MemoryOffset(uintptr_t address)
{
    int checksPassed = 0;
    uint32_t checkValue = 0;

    // Run through the memchecks to make sure we found the right memory space.
    for (int i = 0; i < Emulator::ps1MemoryChecks.size(); ++i)
    {
        if (Platform::read(processHandle, address + Emulator::ps1MemoryChecks[i].first, &checkValue, sizeof(checkValue)))
        {
            if (checkValue == Emulator::ps1MemoryChecks[i].second)
            {
                checksPassed++;
            }
        }
    }

    // Check the disc ID to ensure this is Final Fantasy 7
    bool discCheckPassed = false;
    uint8_t discID[11];
    if (Platform::read(processHandle, address + 0x9E19, &discID[0], 11))
    {
        discCheckPassed |= memcmp(&discID[0], &ff7Disc1ID[0], 11) == 0;
        discCheckPassed |= memcmp(&discID[0], &ff7Disc2ID[0], 11) == 0;
        discCheckPassed |= memcmp(&discID[0], &ff7Disc3ID[0], 11) == 0;
    }

    return (discCheckPassed && checksPassed == Emulator::ps1MemoryChecks.size());
}

bool Emulator::pollErrors(int errorThreshold)
{
    bool result = readErrorCount > errorThreshold || writeErrorCount > errorThreshold;
    if (result)
    {
        readErrorCount = 0;
        writeErrorCount = 0;
    }
    return result;
}