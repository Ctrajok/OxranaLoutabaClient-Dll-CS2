#include "combat_recoil.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"
#include "../config/config_vars.hpp"

#include <Windows.h>
#include <cmath>

#include "../../output/offsets.hpp"
#include "../../output/client_dll.hpp"

template <typename T>
static bool TryRead(uintptr_t address, T& out) { return memory::TryRead(address, out); }

template <typename T>
static bool TryWrite(uintptr_t address, const T& value) { return memory::TryWrite(address, value); }

#define g_offsetsRuntime memory::GetOffsets()

namespace combat {

// ============================================================================
// RCS — Recoil Control System
//
// В CS2 отдача накапливается в m_aimPunchAngle при стрельбе. Она "отталкивает"
// прицел визуально вверх и в стороны. Поле обновляется КАЖДЫЙ тик пока идёт
// стрельба: новое значение = старое + импульс от выстрела.
//
// Классический no-recoil просто обнуляет его — прицел вообще не трясёт, стрелять
// как по стенду. Но это outdated и сильно палится.
//
// Честный RCS: мы отслеживаем ДЕЛЬТУ между предыдущим и текущим punch, и
// компенсируем её в viewAngles с фактором силы 0..100%. Чем выше сила, тем
// точнее компенсация. В CS2 формула игры на экране: effective = view + punch*2.
// Поэтому при компенсации тоже умножаем дельту на 2.
//
// Работает только когда идёт стрельба (shotsFired > 1 — первая пуля всегда
// точна и не даёт punch). Между сериями стрельбы state сбрасывается.
// ============================================================================

namespace {

struct RcsState {
    float prevPunchX = 0.0f;
    float prevPunchY = 0.0f;
    int   prevShots  = 0;
    float pendingX   = 0.0f;
    float pendingY   = 0.0f;
};

static RcsState g_rcs;

} // namespace

void UpdateRecoilControl(bool cs2Active)
{
    if (!cs2Active) {
        g_rcs = {};
        return;
    }

    using namespace cs2_dumper;
    using namespace cs2_dumper::schemas::client_dll;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    __try
    {
        uintptr_t localPawn = 0;
        if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerPawn, localPawn) || !localPawn) return;

        int hp = 0;
        if (!TryRead<int>(localPawn + C_BaseEntity::m_iHealth, hp) || hp <= 0) return;

        // Legacy toggle "No Recoil" — просто обнуляет punch.
        if (config::g_noRecoilEnabled) {
            uintptr_t aps = 0;
            if (TryRead<uintptr_t>(localPawn + g_offsetsRuntime.m_pAimPunchServices, aps) && aps) {
                for (int i = 0; i < 3; ++i) {
                    (void)TryWrite<float>(aps + g_offsetsRuntime.m_aimPunchAngle    + i * 4, 0.0f);
                    (void)TryWrite<float>(aps + g_offsetsRuntime.m_aimPunchAngleVel + i * 4, 0.0f);
                }
            }
            uintptr_t cams = 0;
            if (TryRead<uintptr_t>(localPawn + C_BasePlayerPawn::m_pCameraServices, cams) && cams) {
                for (int i = 0; i < 3; ++i) {
                    (void)TryWrite<float>(cams + CPlayer_CameraServices::m_vecCsViewPunchAngle + i * 4, 0.0f);
                }
            }
            g_rcs = {};
            return;
        }

        // === Настоящий RCS ===
        if (!config::g_rcsEnabled) {
            g_rcs = {};
            return;
        }

        int shots = 0;
        (void)TryRead<int>(localPawn + C_CSPlayerPawn::m_iShotsFired, shots);

        // Сброс при начале новой серии (когда счётчик пошёл от 0 или уменьшился).
        if (shots <= 1 || shots < g_rcs.prevShots) {
            g_rcs = {};
            g_rcs.prevShots = shots;
            return;
        }

        uintptr_t aps = 0;
        if (!TryRead<uintptr_t>(localPawn + g_offsetsRuntime.m_pAimPunchServices, aps) || !aps) {
            g_rcs = {};
            return;
        }

        float punchX = 0.0f, punchY = 0.0f;
        (void)TryRead<float>(aps + g_offsetsRuntime.m_aimPunchAngle + 0, punchX);
        (void)TryRead<float>(aps + g_offsetsRuntime.m_aimPunchAngle + 4, punchY);

        // Compensation strength 0..100 → 0.0..1.0.
        float strength = (float)config::g_rcsStrength / 100.0f;
        if (strength < 0.0f) strength = 0.0f;
        if (strength > 1.0f) strength = 1.0f;

        // Дельта punch относительно предыдущего кадра, умноженная на 2
        // (CS2 отображает punch*2 в прицеле).
        float rawDeltaX = (punchX - g_rcs.prevPunchX) * 2.0f;
        float rawDeltaY = (punchY - g_rcs.prevPunchY) * 2.0f;

        // Smoothing: вместо мгновенной компенсации всей дельты за 1 кадр,
        // применяем только 60% дельты за кадр. Остаток накапливается и
        // компенсируется в следующих кадрах. Это убирает резкий рывок вниз.
        g_rcs.pendingX += rawDeltaX * strength;
        g_rcs.pendingY += rawDeltaY * strength;

        // Применяем 25% от накопленного за кадр (smooth factor).
        // Чем меньше — тем плавнее, но медленнее реагирует.
        constexpr float kSmoothFactor = 0.25f;
        float deltaX = g_rcs.pendingX * kSmoothFactor;
        float deltaY = g_rcs.pendingY * kSmoothFactor;

        // Clamp максимальную коррекцию за кадр — не больше 0.4 градуса.
        // Это убирает рывки при резких импульсах отдачи (первые 2-3 пули AK).
        constexpr float kMaxDeltaPerFrame = 0.4f;
        if (deltaX > kMaxDeltaPerFrame) deltaX = kMaxDeltaPerFrame;
        if (deltaX < -kMaxDeltaPerFrame) deltaX = -kMaxDeltaPerFrame;
        if (deltaY > kMaxDeltaPerFrame) deltaY = kMaxDeltaPerFrame;
        if (deltaY < -kMaxDeltaPerFrame) deltaY = -kMaxDeltaPerFrame;

        g_rcs.pendingX -= deltaX;
        g_rcs.pendingY -= deltaY;

        if (std::abs(g_rcs.pendingX) < 0.001f) g_rcs.pendingX = 0.0f;
        if (std::abs(g_rcs.pendingY) < 0.001f) g_rcs.pendingY = 0.0f;

        // Компенсируем в viewAngles.
        float* viewAngles = reinterpret_cast<float*>(client + g_offsetsRuntime.dwViewAngles);
        float va[3] = {0};
        (void)TryRead<float>((uintptr_t)&viewAngles[0], va[0]);
        (void)TryRead<float>((uintptr_t)&viewAngles[1], va[1]);
        (void)TryRead<float>((uintptr_t)&viewAngles[2], va[2]);

        // Pitch (X) — отдача ВВЕРХ (punch.x отрицательный в CS2), компенсируем ВНИЗ.
        // Yaw (Y) — горизонтальное дрожание.
        va[0] -= deltaX * strength;
        va[1] -= deltaY * strength;

        // Clamp pitch.
        if (va[0] > 89.0f) va[0] = 89.0f;
        if (va[0] < -89.0f) va[0] = -89.0f;
        while (va[1] > 180.0f) va[1] -= 360.0f;
        while (va[1] < -180.0f) va[1] += 360.0f;

        (void)TryWrite<float>((uintptr_t)&viewAngles[0], va[0]);
        (void)TryWrite<float>((uintptr_t)&viewAngles[1], va[1]);

        g_rcs.prevPunchX = punchX;
        g_rcs.prevPunchY = punchY;
        g_rcs.prevShots = shots;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

} // namespace combat
