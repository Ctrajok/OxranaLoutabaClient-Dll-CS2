#include "movement_bhop.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// CS2 dumper includes
#include "../../../output/client_dll.hpp"

#include <Windows.h>

namespace movement {

// CS2 force button values
static constexpr int FORCE_PRESS   = 65537;  // +jump
static constexpr int FORCE_NEUTRAL = 0;      // player controls

void UpdateBunnyHop(bool cs2Active)
{
    if (!cs2Active || !config::g_bhopEnabled)
        return;

    using namespace cs2_dumper::schemas::client_dll;

    static uintptr_t s_addrJump  = 0;
    static bool      s_jumpSent  = false;
    static ULONGLONG s_jumpTime  = 0;
    static bool      s_wasInAir  = false;

    __try
    {
        uintptr_t client = memory::GetClientBase();
        if (!client) { s_addrJump = 0; return; }

        auto& offsets = memory::GetOffsets();
        if (!s_addrJump) s_addrJump = client + offsets.dwForceJump;

        ULONGLONG now = GetTickCount64();

        // Auto-release jump after 2ms (one tick)
        if (s_jumpSent && (now - s_jumpTime >= 2))
        {
            (void)memory::TryWrite<int>(s_addrJump, FORCE_NEUTRAL);
            s_jumpSent = false;
        }

        // Check bhop key
        const bool keyDown = (config::g_bhopKey != 0) && ((GetAsyncKeyState(config::g_bhopKey) & 0x8000) != 0);
        if (!keyDown)
        {
            if (s_jumpSent) {
                (void)memory::TryWrite<int>(s_addrJump, FORCE_NEUTRAL);
                s_jumpSent = false;
            }
            s_wasInAir = false;
            return;
        }

        // Read player state
        uintptr_t localPawn = 0;
        if (!memory::TryRead<uintptr_t>(client + offsets.dwLocalPlayerPawn, localPawn) || !localPawn)
            return;

        uint32_t flags = 0;
        (void)memory::TryRead<uint32_t>(localPawn + C_BaseEntity::m_fFlags, flags);
        bool onGround = (flags & 1) != 0;

        // Perfect Bhop: jump on the exact tick player touches ground
        // Only jump when transitioning from air → ground (landing moment)
        if (onGround && s_wasInAir && !s_jumpSent)
        {
            (void)memory::TryWrite<int>(s_addrJump, FORCE_PRESS);
            s_jumpSent = true;
            s_jumpTime = now;
        }
        // First jump (standing still, press space)
        else if (onGround && !s_wasInAir && !s_jumpSent)
        {
            (void)memory::TryWrite<int>(s_addrJump, FORCE_PRESS);
            s_jumpSent = true;
            s_jumpTime = now;
            s_wasInAir = false; // Will become true next frame when airborne
        }

        if (!onGround)
            s_wasInAir = true;
        else if (s_jumpSent)
            s_wasInAir = false; // Will go to air after jump registers
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (s_addrJump) (void)memory::TryWrite<int>(s_addrJump, FORCE_NEUTRAL);
        s_addrJump = 0;
        s_jumpSent = false;
        s_wasInAir = false;
    }
}

} // namespace movement
