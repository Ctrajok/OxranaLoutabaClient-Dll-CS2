#include "esp_snaplines.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

namespace esp {

void RenderSnaplines(ImDrawList* drawList)
{
    if (!config::g_snaplinesEnabled)
        return;

    auto& espBoxes = GetEspBoxes();
    if (espBoxes.empty())
        return;

    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImVec2 bottom(ds.x * 0.5f, ds.y);

    for (const EspBox& eb : espBoxes)
    {
        ImVec2 boxBottom((eb.box.left + eb.box.right) * 0.5f, eb.box.bottom);
        drawList->AddLine(bottom, boxBottom, IM_COL32(255, 200, 80, 130), 1.0f);
    }
}

} // namespace esp
