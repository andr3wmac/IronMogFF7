#include "ConfigFile.h"
#include "core/utilities/Utilities.h"
#include <fstream>

bool ConfigFile::load(const std::string& filePath)
{
    std::ifstream file(filePath);
    std::string line;

    if (!file.is_open())
    {
        return false;
    }

    configData.clear();
    while (std::getline(file, line))
    {
        // Ignore comments and empty lines
        line = Utilities::trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) continue;

        std::string key = Utilities::trim(line.substr(0, delimiterPos));
        std::string value = Utilities::trim(line.substr(delimiterPos + 1));
        configData[key] = value;
    }

    return true;
}

bool ConfigFile::save(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        return false;
    }

    // Iterate through the map and write each pair
    for (const auto& [key, value] : configData)
    {
        file << key << " = " << value << "\n";
    }

    return true;
}

template <typename T>
T ConfigFile::get(const std::string& key, T defaultValue) const
{
    if (configData.count(keyPrefix + key) == 0)
    {
        return defaultValue;
    }

    const std::string value = configData.at(keyPrefix + key);

    if constexpr (std::is_same_v<T, int>) 
    {
        return std::stoi(value);
    }
    else if constexpr (std::is_same_v<T, float>) 
    {
        return std::stof(value);
    }
    else if constexpr (std::is_same_v<T, uint64_t>)
    {
        return std::stoull(value);
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        std::string lowerVal = value;
        for (auto& c : lowerVal) c = std::tolower(c);

        if (lowerVal == "true" || lowerVal == "1" || lowerVal == "yes" || lowerVal == "on") 
        {
            return true;
        }

        return false; // Default for "false", "0", "no", "off", or garbage text
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        return value;
    }
    else 
    {
        static_assert(sizeof(T) == 0, "Unsupported type for get<T>");
    }
}

template <typename T>
void ConfigFile::set(const std::string& key, T value)
{
    if constexpr (std::is_same_v<T, bool>)
    {
        configData[keyPrefix + key] = value ? "true" : "false";
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        configData[keyPrefix + key] = value;
    }
    else 
    {
        configData[keyPrefix + key] = std::to_string(value);
    }
}

template int ConfigFile::get(const std::string& key, int defaultValue) const;
template float ConfigFile::get(const std::string& key, float defaultValue) const;
template uint64_t ConfigFile::get(const std::string& key, uint64_t defaultValue) const;
template bool ConfigFile::get(const std::string& key, bool defaultValue) const;
template std::string ConfigFile::get(const std::string& key, std::string defaultValue) const;

template void ConfigFile::set(const std::string& key, int value);
template void ConfigFile::set(const std::string& key, float value);
template void ConfigFile::set(const std::string& key, uint64_t value);
template void ConfigFile::set(const std::string& key, bool value);
template void ConfigFile::set(const std::string& key, std::string value);