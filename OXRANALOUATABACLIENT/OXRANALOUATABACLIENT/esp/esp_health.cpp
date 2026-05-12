#include "esp_health.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

#include <map>

namespace esp {

void RenderHealthBars(ImDrawList* drawList)
{
    if (!config::g_hpBarEnabled)
        return;

    auto& espBoxes = GetEspBoxes();

    // Статическая мапа для хранения "текущего" анимированного HP каждого игрока
    static std::map<uintptr_t, float> lerpedHpMap;

    for (const EspBox& e : espBoxes)
    {
        const Box2D& box = e.box;

        float targetHp = (float)e.hp;
        if (lerpedHpMap.find(e.pawn) == lerpedHpMap.end()) 
            lerpedHpMap[e.pawn] = targetHp;
        
        // Плавное приближение (Lerp) к реальному HP, скорость зависит от FPS (DeltaTime)
        lerpedHpMap[e.pawn] += (targetHp - lerpedHpMap[e.pawn]) * 10.0f * ImGui::GetIO().DeltaTime;
        float displayHp = lerpedHpMap[e.pawn];

        const float hp01 = displayHp / 100.0f;
        float r = (hp01 > 0.5f) ? (1.0f - (hp01 - 0.5f) * 2.0f) : 1.0f;
        float g = (hp01 > 0.5f) ? 1.0f : (hp01 * 2.0f);
        
        // Добавим легкое свечение (Glow) для полоски здоровья
        ImU32 hpCol = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, 0.2f, 1.0f));
        ImU32 hpGlow = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, 0.2f, 0.4f)); 

        const float barW = config::g_hpBarWidth;
        const float barPad = config::g_hpBarOffset;
        const float barX0 = box.left - barPad - barW;
        const float barY0 = box.top;
        const float barX1 = box.left - barPad;
        const float barY1 = box.bottom;
        const float barH = (barY1 - barY0);
        const float fillH = barH * hp01;

        // Тень/Фон бара
        drawList->AddRectFilled(ImVec2(barX0, barY0), ImVec2(barX1, barY1), IM_COL32(0, 0, 0, 180), 2.0f);
        // Сама полоска с закруглениями
        drawList->AddRectFilled(ImVec2(barX0 + 1.0f, barY1 - fillH + 1.0f), ImVec2(barX1 - 1.0f, barY1 - 1.0f), hpCol, 1.5f);
        // Glow эффект вокруг полоски
        drawList->AddRect(ImVec2(barX0 + 1.0f, barY1 - fillH + 1.0f), ImVec2(barX1 - 1.0f, barY1 - 1.0f), hpGlow, 1.5f, 0, 2.0f);
        // Обводка бара
        drawList->AddRect(ImVec2(barX0, barY0), ImVec2(barX1, barY1), IM_COL32(0, 0, 0, 255), 2.0f, 0, 1.0f);
    }
}

} // namespace esp
