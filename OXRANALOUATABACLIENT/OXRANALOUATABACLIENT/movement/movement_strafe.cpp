#include "movement_strafe.hpp"

namespace movement {

void UpdateAutoStrafe(bool cs2Active)
{
    // Auto-strafe is currently integrated into UpdateBunnyHop()
    // This function is a placeholder for future separation if needed
    // The logic is controlled by config::g_autoStrafeEnabled flag
    // and executed within the bhop update loop
}

} // namespace movement
