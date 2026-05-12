#include "esp_common.hpp"
#include "../core/core_types.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"
#include "../config/config_vars.hpp"

#include <Windows.h>
#include <cmath>
#include <algorithm>
#include <limits>

// CS2 Dumper includes
#include "../../output/offsets.hpp"
#include "../../output/client_dll.hpp"

// Box2D definition (if not already in header)
struct Box2D {
    float left, top, right, bottom;
};

// Temporary aliases for compatibility
template <typename T>
static bool TryRead(uintptr_t address, T& out)
{
    return memory::TryRead(address, out);
}

#define g_offsetsRuntime memory::GetOffsets()

// WorldToScreen is now in esp_common.hpp

namespace esp {

// Module-local storage for ESP data
static std::vector<EspBox> s_espBoxes;
static std::vector<EspTargetWorld> s_espTargetsWorld;
static std::vector<BoneCacheEntryWorld> s_boneCache;
static std::vector<BoneSkeleton> s_boneSkeletons;
static std::vector<std::string> s_spectators;

// Game time tracking for flash bars
static double s_gameTimeBaseTickMs = 0.0;
static double s_gameTimeBaseSeconds = 0.0;

// Accessor functions
std::vector<EspBox>& GetEspBoxes()
{
    return s_espBoxes;
}

std::vector<EspTargetWorld>& GetEspTargetsWorld()
{
    return s_espTargetsWorld;
}

std::vector<BoneCacheEntryWorld>& GetBoneCache()
{
    return s_boneCache;
}

std::vector<BoneSkeleton>& GetBoneSkeletons()
{
    return s_boneSkeletons;
}

std::vector<std::string>& GetSpectators()
{
    return s_spectators;
}

float GetEstimatedGameTimeSeconds()
{
    if (s_gameTimeBaseTickMs <= 0.0)
        return 0.0f;

    double nowMs = (double)GetTickCount64();
    double dt = (nowMs - s_gameTimeBaseTickMs) / 1000.0;
    return (float)(s_gameTimeBaseSeconds + dt);
}

// Helper function to get 2D box from 3D AABB
static bool GetEntityBox2DFromWorldAABB(const Vec3& origin, const Vec3& mins, const Vec3& maxs, const float view[16], int width, int height, Box2D& out)
{
    const Vec3 corners[8] = {
        { origin.x + mins.x, origin.y + mins.y, origin.z + mins.z },
        { origin.x + mins.x, origin.y + maxs.y, origin.z + mins.z },
        { origin.x + maxs.x, origin.y + maxs.y, origin.z + mins.z },
        { origin.x + maxs.x, origin.y + mins.y, origin.z + mins.z },
        { origin.x + mins.x, origin.y + mins.y, origin.z + maxs.z },
        { origin.x + mins.x, origin.y + maxs.y, origin.z + maxs.z },
        { origin.x + maxs.x, origin.y + maxs.y, origin.z + maxs.z },
        { origin.x + maxs.x, origin.y + mins.y, origin.z + maxs.z },
    };

    float minX = (std::numeric_limits<float>::max)();
    float minY = (std::numeric_limits<float>::max)();
    float maxX = (std::numeric_limits<float>::lowest)();
    float maxY = (std::numeric_limits<float>::lowest)();
    bool anyProjected = false;

    for (const Vec3& c : corners)
    {
        Vec2 p2{};
        if (!WorldToScreen(view, c, width, height, p2))
            continue;
        anyProjected = true;
        if (p2.x < minX) minX = p2.x;
        if (p2.y < minY) minY = p2.y;
        if (p2.x > maxX) maxX = p2.x;
        if (p2.y > maxY) maxY = p2.y;
    }

    if (!anyProjected) return false;
    if (maxY <= minY || maxX <= minX) return false;

    const float pad = config::g_boxPadding;
    out = { minX - pad, minY - pad, maxX + pad, maxY + pad };
    return true;
}

void ProjectEspBoxes(const float view[16], int width, int height)
{
    s_espBoxes.clear();
    s_espBoxes.reserve(s_espTargetsWorld.size());

    for (const EspTargetWorld& t : s_espTargetsWorld)
    {
        Box2D box{};
        if (!GetEntityBox2DFromWorldAABB(t.origin, t.mins, t.maxs, view, width, height, box))
            continue;

        EspBox e{};
        e.box = box;
        e.hp = t.hp;
        e.pawn = t.pawn;
        memcpy(e.name, t.name, 64);
        memcpy(e.weapon, t.weapon, 64);
        e.flashed = t.flashed;
        e.flashDur = t.flashDur;
        e.flashEndTime = t.flashEndTime;
        e.weaponIconIndex = t.weaponIconIndex;
        e.distance = t.distance;
        s_espBoxes.push_back(e);
    }
}

// CollectEspData is still in main file (cs2_internal_dll.cpp) as UpdateESPInternal
// This is a placeholder that will be implemented in a future refactoring phase
void CollectEspData(int width, int height)
{
    // TODO: Move UpdateESPInternal logic here in future refactoring
    // For now, this function is called from main file
}

} // namespace esp
