#include "Emulator.h"
#include "DuckStation.h"
#include "utilities/Process.h"

#include <iostream>
#include <algorithm>

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

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

    return nullptr;
}

Emulator::Emulator()
    : processHandle(0), ps1BaseAddress(0)
{

}

Emulator::~Emulator()
{
    if (processHandle != 0)
    {
        CloseHandle(processHandle);
    }
}

bool Emulator::attach(std::string processName)
{
    DWORD pid = Process::GetIDByName(processName);

    if (pid == 0)
    {
        std::cout << "Emulator process not found.\n";
        return false;
    }

    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!processHandle)
    {
        std::cout << "Failed to open emulator process.\n";
        return false;
    }

    ps1BaseAddress = getPS1MemoryOffset();

    // This is just to ensure we actually attached to the right memory. This will fail if the offset is wrong.
    uintptr_t fieldXOffset = 0x74EB0;
    int32_t fieldX = 0;
    if (!ReadProcessMemory(processHandle, (LPCVOID)(ps1BaseAddress + fieldXOffset), &fieldX, sizeof(fieldX), nullptr))
    {
        std::cout << "Failed to read playstation memory.\n";
        return false;
    }

    return true;
}

bool Emulator::read(uintptr_t offset, void* outBuffer, size_t size)
{
    if (!ReadProcessMemory(processHandle, (LPCVOID)(ps1BaseAddress + offset), outBuffer, size, nullptr))
    {
        std::cout << "Failed to read memory from: " << offset << std::endl;
        return false;
    }

    return true;
}

bool Emulator::write(uintptr_t offset, void* inValue, size_t size)
{
    if (!WriteProcessMemory(processHandle, (LPVOID)(ps1BaseAddress + offset), inValue, size, nullptr))
    {
        std::cout << "Failed to write memory to: " << offset << std::endl;
        return false;
    }

    return true;
}