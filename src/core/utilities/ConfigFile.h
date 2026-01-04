#pragma once

#include <string>
#include <map>

// Simple key/value storage for configuration
class ConfigFile
{
public:
    bool load(const std::string& filePath);
    bool save(const std::string& filePath);

    std::string keyPrefix = "";

    template <typename T>
    T get(const std::string& key, T defaultValue) const;

    template <typename T>
    void set(const std::string& key, T value);

private:
    std::map<std::string, std::string> configData;
};