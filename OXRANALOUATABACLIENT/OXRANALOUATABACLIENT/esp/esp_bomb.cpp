#include "esp_bomb.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// ImGui includes
#include "../../imgui-1.92.5/imgui.h"

#include <Windows.h>

namespace esp {

// Static bomb data storage
static BombData g_bombData = { {0,0,0}, 0.0f, false, false };

BombData& GetBombData()
{
    return g_bombData;
}

void RenderBombEsp(ImDrawList* drawList)
{
    if (!config::g_bombEspEnabled || !g_bombData.found)
        return;

    uintptr_t client = memory::GetClientBase();
    if (!client)
        return;

    __try
    {
        auto& offsets = memory::GetOffsets();
        const float* viewMatrix = reinterpret_cast<const float*>(client + offsets.dwViewMatrix);
        if (viewMatrix)
        {
            ImVec2 ds = ImGui::GetIO().DisplaySize;
            int width = (int)ds.x;
            int height = (int)ds.y;

            Vec2 sPos;
            if (WorldToScreen(viewMatrix, g_bombData.pos, width, height, sPos))
            {
                // Рисуем красный квадрат вокруг бомбы
                drawList->AddRect(ImVec2(sPos.x - 15, sPos.y - 15), ImVec2(sPos.x + 15, sPos.y + 15), IM_COL32(255, 0, 0, 255), 0.0f, 0, 2.0f);

                // Таймер с fallback
                uintptr_t globalVars = 0;
                float timeLeft = -1.0f;
                bool hasTime = false;

                if (memory::TryRead<uintptr_t>(client + offsets.dwGlobalVars, globalVars) && globalVars)
                {
                    float curTime = 0.0f;
                    // ИСПРАВЛЕНИЕ: Оффсет времени изменен с 0x2C на 0x30 (на некоторых версиях игры это 0x34)
                    if (memory::TryRead<float>(globalVars + 0x30, curTime) && curTime > 0.0f) {
                        timeLeft = g_bombData.blowTime - curTime;
                        hasTime = true;
                    }
                }

                // ИСПРАВЛЕНИЕ: Отрисовываем только если время адекватное (отсекаем старые раунды)
                char buf[64];
                if (hasTime && timeLeft > 0.0f && timeLeft <= 45.0f) {
                    sprintf_s(buf, sizeof(buf), "C4: %.1f", timeLeft);
                    if (g_bombData.isDefusing) lstrcatA(buf, " [DEF]");
                    drawList->AddText(ImVec2(sPos.x - 20, sPos.y + 16), IM_COL32(255, 255, 0, 255), buf);
                } else if (hasTime && timeLeft <= 0.0f && timeLeft > -5.0f) {
                    // Бомба только что взорвалась
                    drawList->AddText(ImVec2(sPos.x - 20, sPos.y + 16), IM_COL32(255, 0, 0, 255), "BOOM");
                } else if (!hasTime) {
                    // Fallback: если время не читается
                    lstrcpyA(buf, "C4");
                    if (g_bombData.isDefusing) lstrcatA(buf, " [DEF]");
                    drawList->AddText(ImVec2(sPos.x - 20, sPos.y + 16), IM_COL32(255, 200, 0, 255), buf);
                }

                // DEBUG: Логируем данные бомбы
                #ifdef _DEBUG
                char bombDbg[256];
                sprintf_s(bombDbg, "[BOMB] found=%d pos=%.1f,%.1f,%.1f blowTime=%.2f timeLeft=%.2f\n",
                    g_bombData.found, g_bombData.pos.x, g_bombData.pos.y, g_bombData.pos.z,
                    g_bombData.blowTime, timeLeft);
                OutputDebugStringA(bombDbg);
                #endif
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

} // namespace esp
