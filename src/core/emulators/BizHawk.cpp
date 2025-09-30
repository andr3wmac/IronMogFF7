#include "BizHawk.h"
#include "core/game/GameManager.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

bool GetRemoteModuleInfo(HANDLE hProcess, const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
    {
        for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i)
        {
            wchar_t szModName[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, hMods[i], szModName, MAX_PATH))
            {
                if (_wcsicmp(szModName, moduleName) == 0) // Match module name
                {
                    MODULEINFO modInfo = {};
                    if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
                    {
                        baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                        moduleSize = static_cast<size_t>(modInfo.SizeOfImage);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

uintptr_t BizHawk::getPS1MemoryOffset()
{
    uintptr_t finalAddress = 0;
    uintptr_t moduleAddr = 0;
    size_t moduleSize = 0;

    // If its using the octoshock core we can find it faster by only searching
    // that DLL's memory space.
    if (GetRemoteModuleInfo(processHandle, L"octoshock.dll", moduleAddr, moduleSize))
    {
        finalAddress = findPS1MemoryOffset(moduleAddr, moduleAddr + moduleSize);
    }

    // If all else fails, search entire memory space (slower)
    if (finalAddress == 0)
    {
        finalAddress = findPS1MemoryOffset(0, 0);
    }

    if (verifyPS1MemoryOffset(finalAddress))
    {
        return finalAddress;
    }

    return 0;
}

uintptr_t BizHawk::findPS1MemoryOffset(uintptr_t startAddr, uintptr_t endAddr)
{
    LOG("Searching for BizHawk PS1 Memory Offset..");

    // If these are empty we use the total application space.
    if (startAddr == 0 && endAddr == 0)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        startAddr = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        endAddr = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
    }

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T bytesRead;
    std::vector<uint8_t> buffer;
    uint32_t candidate;

    while (startAddr < endAddr)
    {
        if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(startAddr), &mbi, sizeof(mbi)) == 0)
            break;

        // Only scan committed, readable memory
        bool isReadable =
            (mbi.State == MEM_COMMIT) &&
            ((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_READONLY) || (mbi.Protect & PAGE_WRITECOPY));

        if (isReadable && !(mbi.Protect & PAGE_GUARD))
        {
            buffer.resize(mbi.RegionSize);

            if (ReadProcessMemory(processHandle, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead))
            {
                for (size_t i = 0; i + sizeof(uint32_t) <= bytesRead; i += sizeof(uint32_t))
                {
                    memcpy(&candidate, buffer.data() + i, sizeof(uint32_t));

                    if (candidate == Emulator::ps1MemoryChecks[0].second)
                    {
                        uintptr_t address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + i - Emulator::ps1MemoryChecks[0].first;
                        if (verifyPS1MemoryOffset(address))
                        {
                            return address;
                        }
                    }
                }
            }
        }

        startAddr += mbi.RegionSize;
    }

    return 0;
}