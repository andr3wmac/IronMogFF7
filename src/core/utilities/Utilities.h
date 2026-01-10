#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class Utilities
{
public:
    struct Color
    {
        uint8_t r, g, b;
    };

    // Parses memory address hex string into numeric form
    static uintptr_t parseAddress(const std::string& addressText)
    {
        std::string str = addressText;

        // Remove "0x" or "0X" prefix if present
        if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0)
            str = str.substr(2);

        // Parse as base-16 (hex)
        return static_cast<uintptr_t>(std::stoull(str, nullptr, 16));
    }

    // Case insensitive string search
    static inline bool containsIgnoreCase(const std::string& haystack, const std::string& needle)
    {
        auto toLower = [](const std::string& s)
            {
                std::string result = s;
                std::transform(result.begin(), result.end(), result.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                return result;
            };

        std::string haystack_lower = toLower(haystack);
        std::string needle_lower = toLower(needle);

        return haystack_lower.find(needle_lower) != std::string::npos;
    }

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

    static std::string replaceExtension(const std::string& filename, const std::string& from, const std::string& to)
    {
        std::string result = filename;

        if (result.size() >= from.size() &&
            result.compare(result.size() - from.size(), from.size(), from) == 0) {
            result.replace(result.size() - from.size(), from.size(), to);
        }

        return result;
    }

    static double getTimeMS()
    {
        using namespace std::chrono;
        return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
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

    template<typename T>
    static bool isBitSet(T value, unsigned int bitIndex) 
    {
        static_assert(std::is_integral<T>::value, "isBitSet requires an integral type");

        if (bitIndex >= sizeof(T) * 8) 
        {
            return false;
        }

        return (value & (T(1) << bitIndex)) != 0;
    }

    static uint64_t makeSeed64(uint32_t seed, uint32_t data)
    {
        uint64_t combined = (uint64_t(data) << 32) | seed;

        // MurmurHash3 64-bit mixer
        combined ^= combined >> 33;
        combined *= 0xff51afd7ed558ccd;
        combined ^= combined >> 33;
        combined *= 0xc4ceb9fe1a85ec53;
        combined ^= combined >> 33;

        return combined;
    }

    static uint64_t makeSeed64(uint32_t seed, uint16_t data16, uint8_t data8)
    {
        // Pack: [16-bit data16 | 8-bit data8 | 8-bit padding/zero]
        uint32_t packedData = (uint32_t(data16) << 16) | data8;
        return makeSeed64(seed, packedData);
    }

    static float getDistance(int x1, int y1, int x2, int y2)
    {
        int64_t dx = static_cast<int64_t>(x2) - x1;
        int64_t dy = static_cast<int64_t>(y2) - y1;
        return std::sqrt(static_cast<float>(dx * dx + dy * dy));
    }

    static std::string sanitizeName(const std::string& name) 
    {
        std::string result;
        result.reserve(name.size());

        for (unsigned char c : name) 
        {
            if (std::isalnum(c))
            {
                result += c;
            }
        }

        return result;
    }
};
