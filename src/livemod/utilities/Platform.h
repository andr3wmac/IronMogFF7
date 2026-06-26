#pragma once

#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

// Platform serves as an OS agnostic wrapper around system function calls.
// TODO: implement PlatformLinux.cpp and PlatformOSX.cpp

class Platform
{
public:
    struct ProcessLibrary
    {
        uintptr_t baseAddress; 
        size_t size;
    };

    struct MemoryRegion
    {
        uintptr_t baseAddress;
        size_t size;
        bool isReadable;
        bool isWritable;
        bool isGuarded;
    };

    static void initialize();
    static void shutdown();

    static void* openProcess(uint32_t pid);
    static void closeProcess(void* processHandle);
    static bool read(void* processHandle, uintptr_t address, void* memOut, size_t sizeInBytes);
    static bool write(void* processHandle, uintptr_t address, void* memIn, size_t sizeInBytes);

    static void getApplicationAddressRange(uintptr_t& minAddressOut, uintptr_t& maxAddressOut);
    static bool findProcessLibrary(void* processHandle, const std::string& libraryName, ProcessLibrary& libraryOut);
    static bool openMemoryRegion(void* processHandle, uintptr_t startAddr, MemoryRegion& memoryRegionOut);

    static uint32_t getProcessIDByName(const std::string& processName);
    static uintptr_t getProcessBaseAddress(void* processHandle);
    static std::vector<std::string> getRunningProcesses();

    // Searches the target process for a shared section object that satisfies the given
    // validator and maps it into the current process's address space. Returns a pointer
    // to the mapped view, or nullptr if no matching section is found.
    static void* mapSharedSection(void* processHandle, size_t minSize, std::function<bool(void* view, size_t viewSize)> validator);
    static void unmapSharedSection(void* view);

    static void debuggerLog(const std::string& message);

    static void sleep(double sleepTimeMS);
};
