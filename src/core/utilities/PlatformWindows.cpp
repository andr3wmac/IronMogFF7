#include "Platform.h"
#include "core/utilities/Logging.h"

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <set>

// --- NtWriteVirtualMemory typedef ---
typedef NTSTATUS(WINAPI* NtWriteVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID  BaseAddress,
    PVOID  Buffer,
    SIZE_T BufferSize,
    PSIZE_T NumberOfBytesWritten
    );

void* Platform::openProcess(uint32_t pid)
{
    return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
}

void Platform::closeProcess(void* processHandle)
{
    CloseHandle(processHandle);
}

bool Platform::read(void* processHandle, uintptr_t address, void* memOut, size_t sizeInBytes)
{
    return ReadProcessMemory(processHandle, (LPCVOID)address, memOut, sizeInBytes, nullptr);
}

// WriteProcessMemory results in calls to NtQueryVirtualMemory which can be expensive in some situations.
// Specifically on BizHawk it can take 5-7ms to execute NtQueryVirtualMemory on each write. Instead we 
// bypass the safety checks and call NtWriteVirtualMemory directly. The same is not true for ReadProcessMemory.
bool Platform::write(void* processHandle, uintptr_t address, void* memIn, size_t sizeInBytes)
{
    static NtWriteVirtualMemory_t NtWriteVirtualMemoryFn = nullptr;

    // Load the function once
    if (!NtWriteVirtualMemoryFn)
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (ntdll == 0)
        {
            LOG("Platform::write failed to get ntdll.dll");
            return false;
        }

        NtWriteVirtualMemoryFn = (NtWriteVirtualMemory_t)GetProcAddress(ntdll, "NtWriteVirtualMemory");

        if (!NtWriteVirtualMemoryFn)
        {
            LOG("Platform::write failed to get NtWriteVirtualMemory");
            return false;
        }
    }

    PVOID target = (PVOID)address;
    SIZE_T bytesWritten = 0;

    NTSTATUS status = NtWriteVirtualMemoryFn(
        processHandle,
        target,
        memIn,
        sizeInBytes,
        &bytesWritten
    );

    if (status < 0) // NT_SUCCESS(status)
    {
        LOG("Platform::write NtWriteVirtualMemory failed: offset=%llu status=0x%08X", (unsigned long long)address, status);
        return false;
    }

    return true;
}

void Platform::getApplicationAddressRange(uintptr_t& minAddressOut, uintptr_t& maxAddressOut)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    minAddressOut = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    maxAddressOut = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
}

bool Platform::findProcessLibrary(void* processHandle, const std::string& libraryName, ProcessLibrary& libraryOut)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;

    libraryOut.baseAddr = 0;
    libraryOut.size = 0;

    if (EnumProcessModulesEx(processHandle, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
    {
        for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i)
        {
            char szModName[MAX_PATH];
            if (GetModuleBaseNameA(processHandle, hMods[i], szModName, MAX_PATH))
            {
                if (strcmp(szModName, libraryName.c_str()) == 0) // Match module name
                {
                    MODULEINFO modInfo = {};
                    if (GetModuleInformation(processHandle, hMods[i], &modInfo, sizeof(modInfo)))
                    {
                        libraryOut.baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                        libraryOut.size = static_cast<size_t>(modInfo.SizeOfImage);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool Platform::openMemoryRegion(void* processHandle, uintptr_t startAddr, MemoryRegion& memoryRegionOut)
{
    MEMORY_BASIC_INFORMATION mbi;

    memoryRegionOut.baseAddr    = 0;
    memoryRegionOut.size        = 0;
    memoryRegionOut.isReadable  = false;
    memoryRegionOut.isWritable  = false;
    memoryRegionOut.isGuarded   = false;

    if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(startAddr), &mbi, sizeof(mbi)) == 0)
    {
        return false;
    }

    memoryRegionOut.baseAddr    = (uintptr_t)mbi.BaseAddress;
    memoryRegionOut.size        = mbi.RegionSize;
    memoryRegionOut.isReadable  = (mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_READWRITE) ||(mbi.Protect & PAGE_READONLY) || (mbi.Protect & PAGE_WRITECOPY) || (mbi.Protect & PAGE_EXECUTE_READ) || (mbi.Protect & PAGE_EXECUTE_READWRITE));
    memoryRegionOut.isWritable  = (mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_READWRITE) ||(mbi.Protect & PAGE_WRITECOPY) || (mbi.Protect & PAGE_EXECUTE_READWRITE));
    memoryRegionOut.isGuarded   = (mbi.Protect & PAGE_GUARD) != 0;

    return true;
}

uint32_t Platform::getProcessIDByName(const std::string& processName)
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

uintptr_t Platform::getProcessBaseAddress(void* processHandle)
{
    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(processHandle, &hMod, sizeof(hMod), &cbNeeded)) 
    {
        return reinterpret_cast<uintptr_t>(hMod);
    }

    return 0;
}

std::vector<std::string> Platform::getRunningProcesses()
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