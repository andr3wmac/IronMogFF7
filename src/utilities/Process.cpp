#include "Process.h"

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

uint32_t Process::GetIDByName(const std::string& processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 entry = { 0 };
    entry.dwSize = sizeof(entry);

    if (Process32First(snapshot, &entry)) 
    {
        do 
        {
            if (processName == entry.szExeFile) 
            {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

uintptr_t Process::GetBaseAddress(void* processHandle)
{
    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(processHandle, &hMod, sizeof(hMod), &cbNeeded)) 
    {
        return reinterpret_cast<uintptr_t>(hMod);
    }

    return 0;
}