#include "Emulator.h"
#include "CustomEmulator.h"
#include "DuckStation.h"
#include "BizHawk.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"
#include "core/utilities/Platform.h"
#include "core/utilities/Utilities.h"

#include <iostream>
#include <algorithm>

// These values seem to be the same regardless of what state my emulator is in
// or what game I have loaded.
std::vector<std::pair<uintptr_t, uint32_t>> Emulator::ps1MemoryChecks = {
    {0x80, 1008336896},
    {0x84, 660212864},
    {0x88, 54525960}
};

std::vector<uint8_t> ff7Disc1ID = { 0x53, 0x43, 0x55, 0x53, 0x5F, 0x39, 0x34, 0x31, 0x2E, 0x36, 0x33 }; // SCUS_941.63
std::vector<uint8_t> ff7Disc2ID = { 0x53, 0x43, 0x55, 0x53, 0x5F, 0x39, 0x34, 0x31, 0x2E, 0x36, 0x34 }; // SCUS_941.64
std::vector<uint8_t> ff7Disc3ID = { 0x53, 0x43, 0x55, 0x53, 0x5F, 0x39, 0x34, 0x31, 0x2E, 0x36, 0x35 }; // SCUS_941.65

// Case insensitive string search
bool lowercaseContainsString(const std::string& haystack, const std::string& needle) 
{
    auto toLower = [](const std::string& s) 
    {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    };

    std::string haystack_lower = toLower(haystack);
    std::string needle_lower = toLower(needle);

    return haystack_lower.find(needle_lower) != std::string::npos;
}

Emulator* Emulator::getEmulatorFromProcessName(std::string processName)
{
    if (lowercaseContainsString(processName, "duckstation"))
    {
        return new DuckStation();
    }

    if (lowercaseContainsString(processName, "emuhawk"))
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
    if (processHandle != 0)
    {
        Platform::closeProcess(processHandle);
    }
}

bool Emulator::attach(std::string processName)
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

    ps1BaseAddress = getPS1MemoryOffset();

    // This is just to ensure we actually attached to the right memory. This will fail if the offset is wrong.
    uintptr_t fieldXOffset = FieldOffsets::FieldX;
    int32_t fieldX = 0;
    if (!Platform::read(processHandle, ps1BaseAddress + fieldXOffset, &fieldX, sizeof(fieldX)))
    {
        LOG("Failed to read playstation memory.");
        return false;
    }

    LOG("Successfully attached to emulator at: 0x%X", ps1BaseAddress);
    return true;
}

bool Emulator::read(uintptr_t offset, void* outBuffer, size_t size)
{
    if (!Platform::read(processHandle, ps1BaseAddress + offset, outBuffer, size))
    {
        LOG("Failed to read memory from: %d", offset);
        return false;
    }

    return true;
}

bool Emulator::write(uintptr_t offset, void* inValue, size_t size)
{
    if (!Platform::write(processHandle, ps1BaseAddress + offset, inValue, size))
    {
        LOG("Failed to read memory from: %d", offset);
        return false;
    }

    return true;
}

bool Emulator::verifyPS1MemoryOffset(uintptr_t offset)
{
    int checksPassed = 0;
    uint32_t checkValue = 0;

    // Run through the memchecks to make sure we found the right memory space.
    for (int i = 0; i < Emulator::ps1MemoryChecks.size(); ++i)
    {
        if (Platform::read(processHandle, offset + Emulator::ps1MemoryChecks[i].first, &checkValue, sizeof(checkValue)))
        {
            if (checkValue == Emulator::ps1MemoryChecks[i].second)
            {
                checksPassed++;
            }
        }
    }

    // Check the Disc ID to ensure this is Final Fantasy 7
    bool discCheckPassed = false;
    uint8_t discID[11];
    if (Platform::read(processHandle, offset + 0x9E19, &discID[0], 11))
    {
        discCheckPassed |= memcmp(&discID[0], ff7Disc1ID.data(), 11) == 0;
        discCheckPassed |= memcmp(&discID[0], ff7Disc2ID.data(), 11) == 0;
        discCheckPassed |= memcmp(&discID[0], ff7Disc3ID.data(), 11) == 0;
    }

    return (discCheckPassed && checksPassed == Emulator::ps1MemoryChecks.size());
}