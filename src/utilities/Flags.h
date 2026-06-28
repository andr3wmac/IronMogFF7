#pragma once
#include <type_traits>

template<typename T>
class Flags
{
    static_assert(std::is_integral<T>::value, "Flags template parameter must be an integral type");

private:
    T flags;

public:
    // Default constructor
    Flags() : flags(0) {}

    // Constructor to initialize with a value
    Flags(T initialFlags) : flags(initialFlags) {}

    // Set a flag
    void set(T flag) { flags |= flag; }

    // Remove a flag
    void remove(T flag) { flags &= ~flag; }

    // Clear all flags
    void clear() { flags = 0; }

    // Check if a flag is active
    bool isSet(T flag) const { return (flags & flag) != 0; }

    // Set or clear a bit by index
    void setBit(size_t index, bool value = true)
    {
        const T mask = static_cast<T>(T(1) << index);
        if (value)
            flags |= mask;
        else
            flags &= ~mask;
    }

    // Check if a bit at an index is set
    bool isBitSet(size_t index) const
    {
        const T mask = static_cast<T>(T(1) << index);
        return (flags & mask) != 0;
    }

    // Get the underlying raw flags value
    T value() const { return flags; }

    // Overload bitwise OR operator
    Flags operator|(T flag) const { return Flags(flags | flag); }

    // Overload bitwise AND operator
    Flags operator&(T flag) const { return Flags(flags & flag); }

    // Overload bitwise OR assignment operator
    Flags& operator|=(T flag) { flags |= flag; return *this; }

    // Overload bitwise AND assignment operator
    Flags& operator&=(T flag) { flags &= flag; return *this; }

    // Overload bitwise NOT operator
    Flags operator~() const { return Flags(~flags); }

    // Overload equality operator
    bool operator==(const Flags& other) const { return flags == other.flags; }

    // Overload inequality operator
    bool operator!=(const Flags& other) const { return flags != other.flags; }
};
