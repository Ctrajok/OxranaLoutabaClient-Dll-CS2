#ifndef MEMORY_UTILS_HPP
#define MEMORY_UTILS_HPP

#include <Windows.h>
#include <cstdint>

// Memory module: Safe memory read/write utilities
// This file contains template functions for safe memory access using SEH

namespace memory {

// Safe memory read using Structured Exception Handling (SEH)
// Returns true if read was successful, false if access violation occurred
template <typename T>
bool TryRead(uintptr_t address, T& out)
{
    __try
    {
        out = *reinterpret_cast<const T*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// Safe memory write using Structured Exception Handling (SEH)
// Returns true if write was successful, false if access violation occurred
template <typename T>
bool TryWrite(uintptr_t address, const T& value)
{
    __try
    {
        *reinterpret_cast<T*>(address) = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

} // namespace memory

#endif // MEMORY_UTILS_HPP
