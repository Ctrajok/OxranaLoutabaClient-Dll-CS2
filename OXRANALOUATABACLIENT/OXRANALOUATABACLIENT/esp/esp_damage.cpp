#include "esp_damage.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

#include <Windows.h>
#include <map>
#include <cstdio>

namespace esp {

// Static damage texts storage
static std::vector<DamageText> g_damageTexts;

std::vector<DamageText>& GetDamageTexts()
{
    return g_damageTexts;
}

void AddDamageIndicator(uintptr_t targetPawn, const Vec3& worldPos, int damage)
{
    DamageText dt;
    dt.targetPawn = targetPawn;
    dt.worldPos = worldPos;
    dt.damage = damage;
    dt.spawnTime = GetTickCount64();
    dt.alpha = 1.0f;
    dt.currentYOffset = 0.0f;
    dt.targetYOffset = 0.0f;
    g_damageTexts.push_back(dt);
}

// Helper for reading view matrix safely (SEH requires no objects with destructors)
static bool TryReadViewMatrix(uintptr_t client, float* viewMatrix)
{
    if (!client) return false;
    __try {
        auto& offsets = memory::GetOffsets();
        for (int i = 0; i < 16; ++i) {
            viewMatrix[i] = *(float*)(client + offsets.dwViewMatrix + i * sizeof(float));
        }
        return true;
    } __except(1) {}
    return false;
}

void RenderDamageIndicators(ImDrawList* drawList)
{
    if (!config::g_damageIndicatorsEnabled || g_damageTexts.empty())
        return;

    ULONGLONG now = GetTickCount64();

    uintptr_t client = memory::GetClientBase();

    // Safe view matrix read
    float viewMatrix[16] = { 0 };
    bool hasViewMatrix = TryReadViewMatrix(client, viewMatrix);

    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float lifetimeMs = config::g_damageTextLifetime * 1000.0f;

    // Count stack per target for vertical offset
    std::map<uintptr_t, int> stackCount;

    // Iterate from end (newest first, at bottom)
    for (int i = (int)g_damageTexts.size() - 1; i >= 0; --i)
    {
        DamageText& dt = g_damageTexts[i];
        float ageMs = (float)(now - dt.spawnTime);

        // Remove old entries
        if (ageMs > lifetimeMs) {
            g_damageTexts.erase(g_damageTexts.begin() + i);
            continue;
        }

        // Stack logic: each new hit on same target goes 25px higher
        int index = stackCount[dt.targetPawn]++;
        dt.targetYOffset = index * 25.0f;

        // Smooth animation interpolation
        dt.currentYOffset += (dt.targetYOffset - dt.currentYOffset) * 0.15f;

        Vec2 screenPos;
        if (hasViewMatrix && WorldToScreen(viewMatrix, dt.worldPos, (int)ds.x, (int)ds.y, screenPos))
        {
            // Fade in last second of life
            if (ageMs > lifetimeMs - 1000.0f) {
                dt.alpha = (lifetimeMs - ageMs) / 1000.0f;
            } else {
                dt.alpha = 1.0f;
            }

            char buf[16];
            sprintf_s(buf, sizeof(buf), "-%d", dt.damage);

            ImU32 col = IM_COL32(
                (int)(config::g_damageColor.x * 255),
                (int)(config::g_damageColor.y * 255),
                (int)(config::g_damageColor.z * 255),
                (int)(dt.alpha * 255)
            );
            ImU32 outlineCol = IM_COL32(0, 0, 0, (int)(dt.alpha * 200));

            // Draw to the right of model, rising up
            ImVec2 textPos(screenPos.x + 30.0f, screenPos.y - 20.0f - dt.currentYOffset);

            // Premium outline (4 corners)
            ImFont* font = ImGui::GetFont();
            drawList->AddText(font, config::g_damageTextSize, ImVec2(textPos.x + 1, textPos.y + 1), outlineCol, buf);
            drawList->AddText(font, config::g_damageTextSize, ImVec2(textPos.x - 1, textPos.y - 1), outlineCol, buf);
            drawList->AddText(font, config::g_damageTextSize, ImVec2(textPos.x + 1, textPos.y - 1), outlineCol, buf);
            drawList->AddText(font, config::g_damageTextSize, ImVec2(textPos.x - 1, textPos.y + 1), outlineCol, buf);
            drawList->AddText(font, config::g_damageTextSize, textPos, col, buf);
        }
    }
}

} // namespace esp
