#include "esp_oof_arrows.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// QAngle definition (temporary, should be in core module)
struct QAngle { float x, y, z; };

namespace esp {

void RenderOofArrows(ImDrawList* drawList)
{
    if (!config::g_oofArrowsEnabled)
        return;

    auto& espTargetsWorld = GetEspTargetsWorld();
    if (espTargetsWorld.empty())
        return;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    auto& offsets = memory::GetOffsets();

    QAngle* vpPtr = reinterpret_cast<QAngle*>(client + offsets.dwViewAngles);
    QAngle va{};
    if (!memory::TryRead<QAngle>((uintptr_t)vpPtr, va))
        return;

    uintptr_t localPawn = 0;
    if (!memory::TryRead<uintptr_t>(client + offsets.dwLocalPlayerPawn, localPawn) || !localPawn)
        return;

    uintptr_t localScene = 0;
    if (!memory::TryRead<uintptr_t>(localPawn + offsets.m_pGameSceneNode, localScene) || !localScene)
        return;

    Vec3 myPos{};
    (void)memory::TryRead<float>(localScene + offsets.m_vecAbsOrigin + 0x0, myPos.x);
    (void)memory::TryRead<float>(localScene + offsets.m_vecAbsOrigin + 0x4, myPos.y);

    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    // Анимация пульсации прозрачности (дыхание)
    float oofPulse = (sinf((float)ImGui::GetTime() * 6.0f) * 0.4f) + 0.6f; // От 0.6 до 1.0
    ImVec4 animColor = config::g_oofArrowsColor;
    animColor.w *= oofPulse; // Применяем пульсацию к альфа-каналу
    ImU32 arrowCol = ImGui::ColorConvertFloat4ToU32(animColor);

    // Переводим текущий угол взгляда (yaw) в радианы
    float viewYaw = va.y * (M_PI / 180.0f);

    for (const EspTargetWorld& t : espTargetsWorld)
    {
        // Проверяем, на экране ли враг
        Vec2 screenPos;
        bool onScreen = WorldToScreen(
            reinterpret_cast<const float*>(client + offsets.dwViewMatrix),
            t.origin,
            ImGui::GetIO().DisplaySize.x,
            ImGui::GetIO().DisplaySize.y,
            screenPos
        );

        // Если враг вне экрана
        if (!onScreen)
        {
            float dx = t.origin.x - myPos.x;
            float dy = t.origin.y - myPos.y;

            // Угол на врага в мире
            float angleToEnemy = std::atan2(dy, dx);

            // Разница между нашим взглядом и углом на врага
            // В CS2 X/Y оси повернуты, поэтому вычитаем
            float relativeAngle = viewYaw - angleToEnemy - (M_PI / 2.0f);

            // Позиция стрелки на экране
            float arrowX = center.x + std::cos(relativeAngle) * config::g_oofArrowsRadius;
            float arrowY = center.y + std::sin(relativeAngle) * config::g_oofArrowsRadius;

            // Рисуем треугольник (стрелку), указывающую в эту сторону
            float size = config::g_oofArrowsSize;
            ImVec2 p1(arrowX + std::cos(relativeAngle) * size, arrowY + std::sin(relativeAngle) * size);
            ImVec2 p2(arrowX + std::cos(relativeAngle + 2.5f) * size * 0.8f, arrowY + std::sin(relativeAngle + 2.5f) * size * 0.8f);
            ImVec2 p3(arrowX + std::cos(relativeAngle - 2.5f) * size * 0.8f, arrowY + std::sin(relativeAngle - 2.5f) * size * 0.8f);

            drawList->AddTriangleFilled(p1, p2, p3, arrowCol);
            drawList->AddTriangle(p1, p2, p3, IM_COL32(0, 0, 0, 200), 1.5f); // Обводка
        }
    }
}

} // namespace esp
