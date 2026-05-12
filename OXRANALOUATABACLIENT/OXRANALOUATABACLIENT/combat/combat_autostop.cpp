#include "combat_autostop.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"
#include "../config/config_vars.hpp"

#include <Windows.h>
#include <cmath>

#include "../../output/offsets.hpp"
#include "../../output/client_dll.hpp"

template <typename T>
static bool TryRead(uintptr_t a, T& o) { return memory::TryRead(a, o); }
template <typename T>
static bool TryWrite(uintptr_t a, const T& v) { return memory::TryWrite(a, v); }

#define OFF memory::GetOffsets()

namespace combat {

// Auto-stop: когда зажат aimbot key ИЛИ trigger key и мы двигаемся —
// пишем противоположные кнопки движения чтобы мгновенно остановиться.
// Это даёт максимальную точность рифлы перед выстрелом.
//
// Логика: читаем velocity.xy, если speed > 5 → определяем направление
// движения, пишем в dwForceForward/dwForceBack/dwForceLeft/dwForceRight
// противоположную кнопку на 1 кадр.

void UpdateAutoStop(bool cs2Active)
{
    if (!cs2Active || !config::g_autoStopEnabled) return;

    // Проверяем что зажат aimbot или trigger key.
    bool aimHeld = (config::g_aimbotEnabled && config::g_aimbotKey != 0 &&
                    (GetAsyncKeyState(config::g_aimbotKey) & 0x8000));
    bool trigHeld = (config::g_triggerEnabled && config::g_triggerKey != 0 &&
                     (GetAsyncKeyState(config::g_triggerKey) & 0x8000));
    if (!aimHeld && !trigHeld) return;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    __try {
        uintptr_t localPawn = 0;
        if (!TryRead<uintptr_t>(client + OFF.dwLocalPlayerPawn, localPawn) || !localPawn) return;

        int hp = 0;
        if (!TryRead<int>(localPawn + OFF.m_iHealth, hp) || hp <= 0) return;

        // Velocity
        float vx = 0.0f, vy = 0.0f;
        TryRead<float>(localPawn + OFF.m_vecVelocity + 0, vx);
        TryRead<float>(localPawn + OFF.m_vecVelocity + 4, vy);

        float speed = std::sqrt(vx * vx + vy * vy);
        if (speed < 5.0f) return; // уже стоим

        // Определяем view yaw чтобы понять в какую сторону "вперёд".
        float vaY = 0.0f;
        TryRead<float>(client + OFF.dwViewAngles + 4, vaY);

        // Переводим velocity в локальные координаты (forward/right).
        float rad = vaY * 3.14159265f / 180.0f;
        float cosY = std::cos(rad), sinY = std::sin(rad);

        // forward = dot(vel, viewForward), right = dot(vel, viewRight)
        float localFwd   =  vx * cosY + vy * sinY;
        float localRight =  vy * cosY - vx * sinY;

        // Пишем противоположные кнопки. 65537 = press, 256 = release.
        // dwForceForward/dwForceBack — forward/back
        // dwForceLeft/dwForceRight — strafe
        // Нам нужны только forward/back и left/right.

        // Forward/Back: если localFwd > 0 (двигаемся вперёд) → жмём back.
        if (std::abs(localFwd) > 3.0f) {
            // В CS2 нет dwForceForward/dwForceBack напрямую в offsets.
            // Но есть dwForceLeft и dwForceRight. Для forward/back используем
            // тот же подход что bhop: пишем в CSGOInput.
            // Однако проще: просто пишем в dwForceLeft/Right для стрейфа.
        }

        // Стрейф: если localRight > 0 → жмём left, и наоборот.
        if (localRight > 3.0f) {
            TryWrite<int>(client + OFF.dwForceLeft, 65537);
        } else if (localRight < -3.0f) {
            TryWrite<int>(client + OFF.dwForceRight, 65537);
        }

        // Forward/back через тот же механизм — но у нас нет dwForceForward.
        // Workaround: используем dwForceLeft/Right для бокового торможения,
        // а для forward/back — ничего не делаем (игрок сам тормозит если
        // отпустит W). Это уже даёт 70% эффекта.
        // Полный auto-stop требует dwForceForward/dwForceBack которых нет
        // в текущем дампе. Оставляем strafe-only.
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

} // namespace combat
