#include "ui_widgets.hpp"
#include "ui_cyber_widgets.hpp"
#include "../config/config_vars.hpp"
#include "../config/config_manager.hpp"

#include "../../imgui-1.92.5/imgui.h"
#include <Windows.h>

namespace ui {

bool PremiumToggle(const char* label, bool* v)
{
    // Stage 2 of imgui-cyberpunk-overhaul: PremiumToggle now delegates to CyberToggle.
    // Hitbox (40x20) and routing are identical, so every call-site in ui_legacy.cpp
    // keeps working without changes.
    return CyberToggle(label, v);
}

bool KeybindWidget(const char* id, int& vk)
{
	ImGui::PushID(id);
	static ImGuiID s_capturing = 0;
	ImGuiID my = ImGui::GetID("keybind");

	bool changed = false;
	const bool capturing = (s_capturing == my);
	
	// Визуализация
	char buf[32]{};
	if (capturing)
		lstrcpyA(buf, "Press any key...");
	else if (vk == 0)
		lstrcpyA(buf, "[ None ]");
	else
		wsprintfA(buf, "[ %s ]", VkToStringA(vk));

	ImVec4 col = capturing ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	
	if (ImGui::Button(buf, ImVec2(110.0f, 0.0f)))
	{
		s_capturing = my; // Начинаем слушать ввод по клику
	}
	ImGui::PopStyleColor();

	// Обработка ввода если виджет активен
	if (capturing)
	{
		// Проверяем ESC для сброса
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			vk = 0;
			s_capturing = 0;
			changed = true;
			config::SaveConfig(); // Автосохранение
			
			// Ждем пока отпустят кнопку чтобы не мигало
			while(GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(1);
		}
		// Проверяем Shift для сброса
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			vk = 0;
			s_capturing = 0;
			changed = true;
			config::SaveConfig();
		}
		else
		{
			// Перебор всех клавиш
			for (int k = 1; k < 255; ++k)
			{
				if (k == VK_ESCAPE) continue;
				
				// Пропускаем клик ЛКМ, которым мы активировали кнопку
				if (k == VK_LBUTTON && ImGui::IsMouseDown(0)) continue;

				// Проверка нажатия
				if (GetAsyncKeyState(k) & 0x8000)
				{
					vk = k;
					s_capturing = 0;
					changed = true;
					config::SaveConfig();
					break;
				}
			}
		}
	}

	ImGui::PopID();
	return changed;
}

const char* VkToStringA(int vk)
{
	static char buf[64];
	buf[0] = '\0';
	if (vk == 0) return "[none]";

	UINT scan = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
	LONG lParam = (LONG)(scan << 16);
	if (vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN || 
	    vk == VK_PRIOR || vk == VK_NEXT || vk == VK_END || vk == VK_HOME || 
	    vk == VK_INSERT || vk == VK_DELETE)
		lParam |= 1 << 24;

	int n = GetKeyNameTextA(lParam, buf, (int)sizeof(buf));
	if (n > 0) return buf;
	wsprintfA(buf, "VK_%d", vk);
	return buf;
}

} // namespace ui
