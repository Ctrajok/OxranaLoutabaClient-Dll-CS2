// Memory module: Client.dll base address retrieval
// Implementation file

#include "memory_client.hpp"
#include <Windows.h>

namespace memory {

uintptr_t GetClientBase()
{
    HMODULE hClient = GetModuleHandleW(L"client.dll");
    if (!hClient) 
        return 0;
    return reinterpret_cast<uintptr_t>(hClient);
}

} // namespace memory
