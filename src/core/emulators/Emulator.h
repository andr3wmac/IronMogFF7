#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Emulator
{
public:
    static Emulator* getEmulatorFromProcessName(std::string processName);
    static Emulator* getEmulatorCustom(std::string processName, uintptr_t memoryAddress);

    // Used to verify we attached to the correct memory space
    static std::vector<std::pair<uintptr_t, uint32_t>> ps1MemoryChecks;

public:
    Emulator();
    ~Emulator();

    virtual uintptr_t getPS1MemoryOffset() { return 0; }
    virtual bool connect(std::string processName);

    virtual bool read(uintptr_t offset, void* outBuffer, size_t size);
    virtual bool write(uintptr_t offset, void* inValue, size_t size);

    bool verifyPS1MemoryOffset(uintptr_t offset);

protected:
    void* processHandle;
    uintptr_t ps1BaseAddress;
};