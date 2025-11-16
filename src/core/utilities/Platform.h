#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

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

    static void* openProcess(uint32_t pid);
    static void closeProcess(void* processHandle);
    static bool read(void* processHandle, uintptr_t address, void* memOut, size_t sizeInBytes);
    static bool write(void* processHandle, uintptr_t address, void* memIn, size_t sizeInBytes);

    static void getApplicationAddressRange(uintptr_t& minAddressOut, uintptr_t& maxAddressOut);
    static bool findProcessLibrary(void* processHandle, const std::string& libraryName, ProcessLibrary& libraryOut);
    static bool openMemoryRegion(void* processHandle, uintptr_t startAddr, MemoryRegion& memoryRegionOut);

    // Utility functions
    static uint32_t getProcessIDByName(const std::string& processName);
    static uintptr_t getProcessBaseAddress(void* processHandle);
    static std::vector<std::string> getRunningProcesses();
};
