#pragma once

#include <string>

class Process
{
public:
    static uint32_t GetIDByName(const std::string& processName);
    static uintptr_t Process::GetBaseAddress(void* processHandle);
};
