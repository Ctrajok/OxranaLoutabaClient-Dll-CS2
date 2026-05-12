#include "esp_spectators.hpp"
#include "../config/config_vars.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

namespace esp {

// Static spectators storage (already defined in esp_common.cpp)
// Using the accessor from esp_common.hpp

void RenderSpectatorList()
{
    // Функция включена? Если нет — вообще не рендерим
    if (!config::g_spectatorListEnabled)
        return;

    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(config::g_uiAccentColor.x, config::g_uiAccentColor.y, config::g_uiAccentColor.z, 0.6f));
    
    // Логика взаимодействия: если меню закрыто, окно прозрачно для кликов и зафиксировано
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    if (!config::g_menuOpen) {
        windowFlags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    // Задаем минимальный размер, чтобы окно не "схлопывалось" до микро-размера, когда никого нет
    ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(300, 500));
    
    ImGui::Begin("Spectators", nullptr, windowFlags);
    
    ImGui::PushFont(ImGui::GetFont());
    ImGui::TextColored(config::g_uiAccentColor, "SPECTATORS");
    ImGui::PopFont();
    ImGui::Separator();
    
    auto& spectators = GetSpectators();
    if (spectators.empty()) 
    {
        // Состояние "Пусто"
        ImGui::TextDisabled("No Spectators");
    } 
    else 
    {
        // Динамическое наполнение
        for (const auto& spec : spectators) {
            ImGui::Text("👀 %s", spec.c_str());
        }
    }
    
    ImGui::End();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

} // namespace esp
