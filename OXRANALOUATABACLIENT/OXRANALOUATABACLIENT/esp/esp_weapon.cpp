#include "esp_weapon.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

#include <Windows.h>

namespace esp {

void RenderWeapons(ImDrawList* drawList)
{
    if (!config::g_gunEspEnabled)
        return;

    auto& espBoxes = GetEspBoxes();

    for (const EspBox& e : espBoxes)
    {
        if (!e.pawn || e.weapon[0] == '\0')
            continue;

        const Box2D& box = e.box;

        char wepText[64];
        // Убрали квадратные скобки
        wsprintfA(wepText, "%s", e.weapon);
        
        // Используем config::g_weaponIconColor, который меняется в меню
        ImU32 gunColU32 = ImGui::ColorConvertFloat4ToU32(config::g_weaponIconColor);
        
        ImVec2 textSize = ImGui::CalcTextSize(wepText);
        float xCenter = (box.left + box.right) * 0.5f;
        ImVec2 pos(xCenter - textSize.x * 0.5f, box.bottom + config::g_gunOffsetY);
        drawList->AddText(pos, gunColU32, wepText);
    }
}

} // namespace esp
