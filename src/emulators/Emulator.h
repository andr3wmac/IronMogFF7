#pragma once

#include <cstdint>
#include <string>

class Emulator
{
public:
    static Emulator* getEmulatorFromProcessName(std::string processName);
    static Emulator* getEmulatorCustom(std::string processName, uintptr_t memoryAddress);

public:
    Emulator();
    ~Emulator();

    virtual uintptr_t getPS1MemoryOffset() { return 0; }
    virtual bool attach(std::string processName);

    virtual bool read(uintptr_t offset, void* outBuffer, size_t size);
    virtual bool write(uintptr_t offset, void* inValue, size_t size);

protected:
    void* processHandle;
    uintptr_t ps1BaseAddress;
};