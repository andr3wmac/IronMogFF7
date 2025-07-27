#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

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

    static inline uint32_t hexStringToSeed(const std::string& hex)
    {
        uint32_t seed;
        std::istringstream iss(hex);
        iss >> std::hex >> seed;
        return seed;
    }

    template<typename T>
    static void saveArrayToFile(const T* arr, const size_t size, const std::string& filename)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");

        std::ofstream out(filename, std::ios::binary);
        if (!out) throw std::runtime_error("Failed to open file for writing.");

        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out.write(reinterpret_cast<const char*>(arr), size * sizeof(T));
    }

    template<typename T>
    static T* loadArrayFromFile(const std::string& filename)
    {
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");

        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Failed to open file for reading.");

        size_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));

        T* arr = new T[size];
        in.read(reinterpret_cast<char*>(arr), size * sizeof(T));

        return arr;
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

    static inline std::string trim(const std::string& str) 
    {
        const char* whitespace = " \t\n\r";
        const auto start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        const auto end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    // Loads key/value pair config file, like a simpler ini format
    static std::unordered_map<std::string, std::string> loadConfig(const std::string& filename) 
    {
        std::unordered_map<std::string, std::string> config;
        std::ifstream file(filename);
        std::string line;

        if (!file.is_open()) 
        {
            return config;
        }

        while (std::getline(file, line)) 
        {
            // Ignore comments and empty lines
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            auto delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) continue;

            std::string key = trim(line.substr(0, delimiterPos));
            std::string value = trim(line.substr(delimiterPos + 1));
            config[key] = value;
        }

        return config;
    }

    static std::string replaceExtension(const std::string& filename, const std::string& from, const std::string& to)
    {
        std::string result = filename;

        if (result.size() >= from.size() &&
            result.compare(result.size() - from.size(), from.size(), from) == 0) {
            result.replace(result.size() - from.size(), from.size(), to);
        }

        return result;
    }

    static uint64_t getTimeMS() 
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    static std::string formatTime(uint32_t seconds) 
    {
        uint32_t hours = seconds / 3600;
        uint32_t minutes = (seconds % 3600) / 60;
        uint32_t secs = seconds % 60;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << secs;
        return oss.str();
    }
};
