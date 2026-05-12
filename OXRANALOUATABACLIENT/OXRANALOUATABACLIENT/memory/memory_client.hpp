#ifndef MEMORY_CLIENT_HPP
#define MEMORY_CLIENT_HPP

#include <cstdint>

// Memory module: Client.dll base address retrieval
// This file contains functions for getting the base address of client.dll

namespace memory {

// Get the base address of client.dll module
// Returns 0 if module is not loaded
uintptr_t GetClientBase();

} // namespace memory

#endif // MEMORY_CLIENT_HPP
