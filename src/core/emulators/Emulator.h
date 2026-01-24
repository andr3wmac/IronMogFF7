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

    // Returns true if the memory at the given address matches a number of
    // heuristics used to identify PS1 FF7 memory space.
    bool verifyPS1MemoryOffset(uintptr_t address);

    // Returns true if the read or write errors exceeds the given threshold.
    // Error counts are reset to 0 when this function returns true.
    bool pollErrors(int errorThreshold = 5);

protected:
    void* processHandle;
    uintptr_t ps1BaseAddress;
    int readErrorCount = 0;
    int writeErrorCount = 0;
};