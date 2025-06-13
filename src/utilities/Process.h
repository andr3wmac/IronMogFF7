#pragma once

#include <string>
#include <vector>

class Process
{
public:
    static uint32_t GetIDByName(const std::string& processName);
    static uintptr_t GetBaseAddress(void* processHandle);
    static std::vector<std::string> GetRunningProcesses();
    static uintptr_t ParseAddress(const std::string& addressText);
};
