#pragma once

#include <string>
#include <vector>

class Utilities
{
public:
    // Process related utility functions
    static uint32_t getProcessIDByName(const std::string& processName);
    static uintptr_t getProcessBaseAddress(void* processHandle);
    static std::vector<std::string> getRunningProcesses();

    // Parses memory address hex string into numeric form
    static uintptr_t parseAddress(const std::string& addressText);

    static inline std::string seedToHexString(uint32_t seed) 
    {
        char buffer[9]; // 8 hex digits + null terminator
        snprintf(buffer, sizeof(buffer), "%08X", seed);
        return std::string(buffer);
    }

    template<typename T>
    static void saveVectorToFile(const std::vector<T>& vec, const std::string& filename) 
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");

        std::ofstream out(filename, std::ios::binary);
        if (!out) throw std::runtime_error("Failed to open file for writing.");

        size_t size = vec.size();
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
    }

    template<typename T>
    static std::vector<T> loadVectorFromFile(const std::string& filename) 
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");

        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Failed to open file for reading.");

        size_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));

        std::vector<T> vec(size);
        in.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));

        return vec;
    }
};
