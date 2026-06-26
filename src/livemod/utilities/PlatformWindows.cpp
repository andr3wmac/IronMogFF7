#include "Platform.h"
#include "livemod/utilities/Logging.h"

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>

#pragma comment(lib, "Winmm.lib")
#include <timeapi.h>

#include <set>
#include <thread>

// NtReadVirtualMemory function signature
typedef NTSTATUS(WINAPI* NtReadVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T BufferSize,
    PSIZE_T NumberOfBytesRead
    );

// NtWriteVirtualMemory function signature
typedef NTSTATUS(WINAPI* NtWriteVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID  BaseAddress,
    PVOID  Buffer,
    SIZE_T BufferSize,
    PSIZE_T NumberOfBytesWritten
    );

// NtQuerySystemInformation function signature
typedef NTSTATUS(NTAPI* NtQuerySystemInformation_t)(
    SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

// NtMapViewOfSection function signature
typedef NTSTATUS(NTAPI* NtMapViewOfSection_t)(
    HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T,
    PLARGE_INTEGER, PSIZE_T, DWORD, ULONG, ULONG);

// NtUnmapViewOfSection function signature
typedef NTSTATUS(NTAPI* NtUnmapViewOfSection_t)(
    HANDLE, PVOID);

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO {
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT HandleValue;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION;

constexpr SYSTEM_INFORMATION_CLASS SystemHandleInformation = (SYSTEM_INFORMATION_CLASS)16;

#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define ViewUnmapped 1

void Platform::initialize()
{
    // Increases the precision of timing events on Windows. This is used for sleep()
    timeBeginPeriod(1);
}

void Platform::shutdown()
{
    timeEndPeriod(1);
}

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
    static NtReadVirtualMemory_t NtReadVirtualMemoryFn = nullptr;

    // Load the function once
    if (!NtReadVirtualMemoryFn)
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (ntdll == 0)
        {
            LOG("Platform::read failed to get ntdll.dll");
            return false;
        }

        NtReadVirtualMemoryFn = (NtReadVirtualMemory_t)GetProcAddress(ntdll, "NtReadVirtualMemory");

        if (!NtReadVirtualMemoryFn)
        {
            LOG("Platform::read failed to get NtReadVirtualMemory");
            return false;
        }
    }

    PVOID target = (PVOID)address;
    SIZE_T bytesRead = 0;
    NTSTATUS status = NtReadVirtualMemoryFn(processHandle, target, memOut, sizeInBytes, &bytesRead);

    if (status < 0)
    {
        // In the event of a read failure we fall back to the slower less error prone approach.
        status = ReadProcessMemory(processHandle, (LPCVOID)address, memOut, sizeInBytes, nullptr);
    }

    // NT_SUCCESS
    if (status < 0)
    {
        LOG("Platform::read NtReadVirtualMemory failed: offset=%llu status=0x%08X", (unsigned long long)address, status);
        return false;
    }

    return true;
}

// WriteProcessMemory results in calls to NtQueryVirtualMemory which can be expensive in some situations.
// Specifically on BizHawk it can take 5-7ms to execute NtQueryVirtualMemory on each write. Instead we 
// bypass the safety checks and call NtWriteVirtualMemory directly.
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

    NTSTATUS status = NtWriteVirtualMemoryFn(processHandle, target, memIn, sizeInBytes, &bytesWritten);

    // STATUS_PARTIAL_COPY
    if (status == 0x8000000D)
    {
        // In the event of a partial copy we fall back to the slower less error prone approach.
        LOG("Platform::write NtWriteVirtualMemory returned partial copy, retrying..");
        status = WriteProcessMemory(processHandle, target, memIn, sizeInBytes, &bytesWritten);
    }

    // NT_SUCCESS
    if (status < 0) 
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

    libraryOut.baseAddress = 0;
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
                        libraryOut.baseAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
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

    memoryRegionOut.baseAddress = 0;
    memoryRegionOut.size        = 0;
    memoryRegionOut.isReadable  = false;
    memoryRegionOut.isWritable  = false;
    memoryRegionOut.isGuarded   = false;

    if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(startAddr), &mbi, sizeof(mbi)) == 0)
    {
        return false;
    }

    memoryRegionOut.baseAddress = (uintptr_t)mbi.BaseAddress;
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
        } 
        while (Process32Next(snapshot, &entry));
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
        do 
        {
            if (seenPIDs.count(entry.th32ProcessID)) 
            {
                result.emplace_back(entry.szExeFile);
            }
        } 
        while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return result;
}

void Platform::debuggerLog(const std::string& message)
{
    // Write to Visual Studio debug console
    OutputDebugStringA(message.c_str());
}

// Based on: https://blog.bearcats.nl/perfect-sleep-function/
void Platform::sleep(double sleepTimeMS)
{
    auto t0 = std::chrono::steady_clock::now();
    auto target = t0 + std::chrono::nanoseconds(int64_t(sleepTimeMS * 1e6));

    // Sleep
    int ticks = (int)(sleepTimeMS - 1.02);
    if (ticks > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ticks));
    }

    // Spin
    while (std::chrono::steady_clock::now() < target)
    {
        YieldProcessor();
    }
}

void* Platform::mapSharedSection(void* processHandle, size_t minSize, std::function<bool(void*, size_t)> validator)
{
    static NtQuerySystemInformation_t NtQuerySystemInformationFn = nullptr;
    static NtMapViewOfSection_t NtMapViewOfSectionFn = nullptr;
    static NtUnmapViewOfSection_t NtUnmapViewOfSectionFn = nullptr;

    if (!NtQuerySystemInformationFn || !NtMapViewOfSectionFn || !NtUnmapViewOfSectionFn)
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll)
        {
            return nullptr;
        }

        NtQuerySystemInformationFn = (NtQuerySystemInformation_t)GetProcAddress(ntdll, "NtQuerySystemInformation");
        NtMapViewOfSectionFn       = (NtMapViewOfSection_t)GetProcAddress(ntdll, "NtMapViewOfSection");
        NtUnmapViewOfSectionFn     = (NtUnmapViewOfSection_t)GetProcAddress(ntdll, "NtUnmapViewOfSection");
    }

    DWORD targetPid = GetProcessId(processHandle);

    // Query all system handles, growing buffer as needed
    ULONG bufSize = 1 << 20;
    std::unique_ptr<BYTE[]> buf;
    NTSTATUS status;
    do 
    {
        buf = std::make_unique<BYTE[]>(bufSize);
        status = NtQuerySystemInformationFn(SystemHandleInformation, buf.get(), bufSize, nullptr);
        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            bufSize *= 2;
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (!NT_SUCCESS(status))
    {
        return nullptr;
    }

    auto* handleInfo = reinterpret_cast<SYSTEM_HANDLE_INFORMATION*>(buf.get());

    for (ULONG i = 0; i < handleInfo->NumberOfHandles; i++)
    {
        auto& entry = handleInfo->Handles[i];

        // Only look at handles belonging to the target process
        if (entry.UniqueProcessId != targetPid)
        {
            continue;
        }

        // Try to duplicate the handle into our process
        HANDLE localHandle = nullptr;
        if (!DuplicateHandle(
            processHandle,
            (HANDLE)(uintptr_t)entry.HandleValue,
            GetCurrentProcess(),
            &localHandle,
            FILE_MAP_READ | FILE_MAP_WRITE,
            FALSE,
            0))
        {
            continue;
        }

        // Try to map it as a section
        PVOID view = nullptr;
        SIZE_T viewSize = 0;
        LARGE_INTEGER offset{};
        status = NtMapViewOfSectionFn(
            localHandle,
            GetCurrentProcess(),
            &view,
            0, 0,
            &offset,
            &viewSize,
            ViewUnmapped,
            0,
            PAGE_READWRITE);

        if (!NT_SUCCESS(status) || !view)
        {
            CloseHandle(localHandle);
            continue;
        }

        if (viewSize < minSize)
        {
            NtUnmapViewOfSectionFn(GetCurrentProcess(), view);
            CloseHandle(localHandle);
            continue;
        }

        if (validator(view, viewSize))
        {
            // Found a match. The view holds its own reference so we can
            // close the duplicated localHandle immediately.
            CloseHandle(localHandle);
            return view;
        }

        NtUnmapViewOfSectionFn(GetCurrentProcess(), view);
        CloseHandle(localHandle);
    }

    return nullptr;
}

void Platform::unmapSharedSection(void* view)
{
    static NtUnmapViewOfSection_t NtUnmapViewOfSectionFn = nullptr;

    if (!NtUnmapViewOfSectionFn)
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll)
        {
            return;
        }

        NtUnmapViewOfSectionFn = (NtUnmapViewOfSection_t)GetProcAddress(ntdll, "NtUnmapViewOfSection");
    }

    NtUnmapViewOfSectionFn(GetCurrentProcess(), view);
}