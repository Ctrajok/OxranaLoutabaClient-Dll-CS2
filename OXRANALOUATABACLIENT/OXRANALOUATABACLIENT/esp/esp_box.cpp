#include "esp_box.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

namespace esp {

void RenderBoxes(ImDrawList* drawList)
{
    if (!config::g_whEnabled)
        return;

    const ImU32 staticBoxCol = ImGui::ColorConvertFloat4ToU32(config::g_boxColor);
    auto& espBoxes = GetEspBoxes();

    for (const EspBox& e : espBoxes)
    {
        const Box2D& box = e.box;
        
        // Dynamic box color based on HP
        ImU32 boxCol = staticBoxCol;
        if (config::g_dynamicBoxColor) {
            float hpPct = e.hp / 100.0f;
            if (hpPct < 0.0f) hpPct = 0.0f;
            if (hpPct > 1.0f) hpPct = 1.0f;
            // Green (100%) -> Yellow (50%) -> Red (0%)
            float r = (hpPct > 0.5f) ? (1.0f - (hpPct - 0.5f) * 2.0f) : 1.0f;
            float g = (hpPct > 0.5f) ? 1.0f : (hpPct * 2.0f);
            boxCol = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, 0.1f, 1.0f));
        }
        
        // Боксы (Corner Box style с черной аутлайн-тенью)
        float bW = box.right - box.left;
        float bH = box.bottom - box.top;
        float lX = bW * 0.25f; // Длина уголка (25% от ширины)
        float lY = bW * 0.25f; // Длина уголка по вертикали
        float thick = config::g_boxThickness;

        ImU32 outlineCol = IM_COL32(0, 0, 0, 200); // Черная обводка для читаемости

        auto drawCorner = [&](float x1, float y1, float x2, float y2, float x3, float y3) {
            // Тень (Outline)
            drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), outlineCol, thick + 2.0f);
            drawList->AddLine(ImVec2(x2, y2), ImVec2(x3, y3), outlineCol, thick + 2.0f);
            // Основная линия
            drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), boxCol, thick);
            drawList->AddLine(ImVec2(x2, y2), ImVec2(x3, y3), boxCol, thick);
        };

        // Top Left
        drawCorner(box.left, box.top + lY, box.left, box.top, box.left + lX, box.top);
        // Top Right
        drawCorner(box.right - lX, box.top, box.right, box.top, box.right, box.top + lY);
        // Bottom Left
        drawCorner(box.left, box.bottom - lY, box.left, box.bottom, box.left + lX, box.bottom);
        // Bottom Right
        drawCorner(box.right - lX, box.bottom, box.right, box.bottom, box.right, box.bottom - lY);
    }
}

} // namespace esp
