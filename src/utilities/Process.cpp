#include "Process.h"

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <set>

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

std::vector<std::string> Process::GetRunningProcesses() 
{
    std::vector<std::string> result;
    std::set<DWORD> seenPIDs;

    // First, find PIDs with visible top-level windows
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (!(GetWindow(hwnd, GW_OWNER) == nullptr && IsWindowVisible(hwnd))) return TRUE;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0) 
        {
            reinterpret_cast<std::set<DWORD>*>(lParam)->insert(pid);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&seenPIDs));

    // Now match those PIDs to executable names
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return result;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &entry)) 
    {
        do {
            if (seenPIDs.count(entry.th32ProcessID)) 
            {
                result.emplace_back(entry.szExeFile);
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return result;
}

uintptr_t Process::ParseAddress(const std::string& input)
{
    std::string str = input;

    // Remove "0x" or "0X" prefix if present
    if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0)
        str = str.substr(2);

    // Parse as base-16 (hex)
    return static_cast<uintptr_t>(std::stoull(str, nullptr, 16));
}