#pragma once

#include <vector>
#include <string>

// Wrapper around list of strings to make it easier to work with imgui combos/lists.
class StringList 
{
public:
    StringList() = default;

    StringList(std::vector<std::string> vec) : strings(std::move(vec)) 
    {
        rebuildPointers();
    }

    StringList& operator=(std::vector<std::string> vec) 
    {
        strings = std::move(vec);
        rebuildPointers();
        return *this;
    }

    void push_back(const std::string& s) 
    {
        strings.push_back(s);
        rebuildPointers();
    }

    void clear() 
    {
        strings.clear();
        c_strs.clear();
    }

    const char* const* data() const 
    {
        return c_strs.data();
    }

    size_t size() const 
    {
        return strings.size();
    }

    const std::string& operator[](size_t index) const 
    {
        return strings[index];
    }

private:
    std::vector<std::string> strings;
    std::vector<const char*> c_strs;

    void rebuildPointers()
    {
        c_strs.clear();
        c_strs.reserve(strings.size());
        for (const auto& s : strings) 
        {
            c_strs.push_back(s.c_str());
        }
    }
};