#include "esp_name.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

namespace esp {

void RenderNames(ImDrawList* drawList)
{
    if (!config::g_nameEspEnabled)
        return;

    auto& espBoxes = GetEspBoxes();

    for (const EspBox& e : espBoxes)
    {
        if (!e.pawn)
            continue;

        if (e.name[0] == '\0')
            continue;

        const Box2D& box = e.box;
        ImU32 nameColU32 = ImGui::ColorConvertFloat4ToU32(config::g_nameColor);
        ImVec2 textSize = ImGui::CalcTextSize(e.name);
        float xCenter = (box.left + box.right) * 0.5f;
        ImVec2 pos(xCenter - textSize.x * 0.5f, box.top - config::g_nameOffsetY - textSize.y);
        drawList->AddText(pos, nameColU32, e.name);
    }
}

} // namespace esp
