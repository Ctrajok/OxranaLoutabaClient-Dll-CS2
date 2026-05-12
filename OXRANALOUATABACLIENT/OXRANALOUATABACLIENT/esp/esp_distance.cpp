#include "esp_distance.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

#include <cmath>
#include <cstdio>

namespace esp {

void RenderDistance(ImDrawList* drawList)
{
    if (!config::g_distanceEnabled)
        return;

    auto& espBoxes = GetEspBoxes();
    auto& espTargetsWorld = GetEspTargetsWorld();
    
    if (espBoxes.empty() || espTargetsWorld.empty())
        return;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;
    
    auto& offsets = memory::GetOffsets();
    
    uintptr_t localPawn = 0;
    if (!memory::TryRead<uintptr_t>(client + offsets.dwLocalPlayerPawn, localPawn) || !localPawn) return;
    uintptr_t localScene = 0;
    if (!memory::TryRead<uintptr_t>(localPawn + offsets.m_pGameSceneNode, localScene) || !localScene) return;
    
    Vec3 myPos{};
    (void)memory::TryRead<float>(localScene + offsets.m_vecAbsOrigin, myPos.x);
    (void)memory::TryRead<float>(localScene + offsets.m_vecAbsOrigin + 4, myPos.y);
    (void)memory::TryRead<float>(localScene + offsets.m_vecAbsOrigin + 8, myPos.z);

    int idx = 0;
    for (const auto& tgt : espTargetsWorld)
    {
        if (idx >= (int)espBoxes.size()) break;
        const EspBox& eb = espBoxes[idx++];

        float dx = tgt.origin.x - myPos.x;
        float dy = tgt.origin.y - myPos.y;
        float dz = tgt.origin.z - myPos.z;
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz) * 0.01905f; // units→metres
        
        char distBuf[16];
        sprintf_s(distBuf, sizeof(distBuf), "%.0fm", dist);
        ImVec2 distPos((eb.box.left + eb.box.right) * 0.5f - 12.0f, eb.box.top - 28.0f);
        drawList->AddText(distPos, IM_COL32(200, 200, 200, 200), distBuf);
    }
}

} // namespace esp
