#include "combat_autoknife.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"
#include "../config/config_vars.hpp"
#include "combat_common.hpp"

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

// Knife defIndex'ы в CS2 (все ножи).
static bool IsKnifeDefIndex(int defIdx)
{
    // Стандартные ножи: 42 (CT), 59 (T), + все кастомные (500-525).
    if (defIdx == 42 || defIdx == 59) return true;
    if (defIdx >= 500 && defIdx <= 525) return true;
    return false;
}

// Проверяем смотрит ли враг от нас (backstab condition).
// Если dot(enemyForward, dirToEnemy) > 0.3 → враг смотрит от нас → backstab.
static bool IsBackstab(float localX, float localY, float enemyX, float enemyY, float enemyYaw)
{
    // Направление от нас к врагу.
    float dx = enemyX - localX;
    float dy = enemyY - localY;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01f) return false;
    dx /= len; dy /= len;

    // Forward вектор врага.
    float rad = enemyYaw * 3.14159265f / 180.0f;
    float efx = std::cos(rad);
    float efy = std::sin(rad);

    // Dot product: если > 0 → враг смотрит в ту же сторону что мы к нему → спина.
    float dot = dx * efx + dy * efy;
    return dot > 0.3f;
}

void UpdateAutoKnife(bool cs2Active)
{
    if (!cs2Active || !config::g_autoKnifeEnabled) return;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    using namespace cs2_dumper::schemas::client_dll;

    static ULONGLONG s_lastStab = 0;
    static bool s_attacking = false;

    __try {
        uintptr_t localPawn = 0;
        if (!TryRead<uintptr_t>(client + OFF.dwLocalPlayerPawn, localPawn) || !localPawn) return;

        int hp = 0;
        if (!TryRead<int>(localPawn + OFF.m_iHealth, hp) || hp <= 0) return;

        int localTeam = 0;
        TryRead<int>(localPawn + OFF.m_iTeamNum, localTeam);

        // Проверяем что в руках нож.
        uintptr_t ws = 0;
        if (!TryRead<uintptr_t>(localPawn + C_BasePlayerPawn::m_pWeaponServices, ws) || !ws) return;
        uint32_t hActive = 0;
        if (!TryRead<uint32_t>(ws + CPlayer_WeaponServices::m_hActiveWeapon, hActive) || !hActive) return;

        uintptr_t entityList = 0;
        if (!TryRead<uintptr_t>(client + OFF.dwEntityList, entityList) || !entityList) return;

        int wi = hActive & 0x1FF, we = (hActive & 0x7FFF) >> 9;
        uintptr_t wle = 0;
        if (!TryRead<uintptr_t>(entityList + 0x8 * we + 0x10, wle) || !wle) return;
        uintptr_t weaponEnt = 0;
        if (!TryRead<uintptr_t>(wle + 0x70 * wi, weaponEnt) || !weaponEnt) return;

        uint16_t defIdx = 0;
        TryRead<uint16_t>(weaponEnt + C_EconEntity::m_AttributeManager +
            C_AttributeContainer::m_Item + C_EconItemView::m_iItemDefinitionIndex, defIdx);

        if (!IsKnifeDefIndex((int)defIdx)) {
            // Не нож — отпускаем если были в атаке.
            if (s_attacking) {
                TryWrite<int>(client + OFF.dwForceAttack, 256);
                s_attacking = false;
            }
            return;
        }

        // Позиция локального игрока.
        uintptr_t localScene = 0;
        if (!TryRead<uintptr_t>(localPawn + C_BaseEntity::m_pGameSceneNode, localScene) || !localScene) return;
        float lx = 0, ly = 0, lz = 0;
        TryRead<float>(localScene + CGameSceneNode::m_vecAbsOrigin + 0, lx);
        TryRead<float>(localScene + CGameSceneNode::m_vecAbsOrigin + 4, ly);
        TryRead<float>(localScene + CGameSceneNode::m_vecAbsOrigin + 8, lz);

        // View angles для определения куда мы смотрим.
        float vaY = 0;
        TryRead<float>(client + OFF.dwViewAngles + 4, vaY);

        ULONGLONG now = GetTickCount64();

        // Кулдаун между ударами (нож имеет ~1с между ударами).
        if (now - s_lastStab < 400) {
            if (s_attacking && now - s_lastStab > 80) {
                TryWrite<int>(client + OFF.dwForceAttack, 256);
                s_attacking = false;
            }
            return;
        }

        // Ищем ближайшего врага в радиусе ножа (64 units).
        constexpr float kKnifeRange = 64.0f;
        constexpr float kKnifeRangeSq = kKnifeRange * kKnifeRange;

        float bestDistSq = kKnifeRangeSq;
        bool bestIsBack = false;
        bool foundTarget = false;

        for (int id = 1; id <= 64; ++id) {
            uintptr_t lEntry = 0;
            if (!TryRead<uintptr_t>(entityList + 0x8 * ((id & 0x7FFF) >> 9) + 0x10, lEntry) || !lEntry) continue;
            uintptr_t ctrl = 0;
            if (!TryRead<uintptr_t>(lEntry + 0x70 * (id & 0x1FF), ctrl) || !ctrl) continue;
            uint32_t ph = 0;
            if (!TryRead<uint32_t>(ctrl + CCSPlayerController::m_hPlayerPawn, ph) || !ph) continue;
            uintptr_t pList = 0;
            if (!TryRead<uintptr_t>(entityList + 0x8 * ((ph & 0x7FFF) >> 9) + 0x10, pList) || !pList) continue;
            uintptr_t pawn = 0;
            if (!TryRead<uintptr_t>(pList + 0x70 * (ph & 0x1FF), pawn) || !pawn) continue;
            if (pawn == localPawn) continue;

            int tHp = 0;
            if (!TryRead<int>(pawn + OFF.m_iHealth, tHp) || tHp <= 0) continue;
            int tTeam = 0;
            if (!TryRead<int>(pawn + OFF.m_iTeamNum, tTeam)) continue;
            if (tTeam == localTeam) continue; // не бьём своих

            uintptr_t tScene = 0;
            if (!TryRead<uintptr_t>(pawn + C_BaseEntity::m_pGameSceneNode, tScene) || !tScene) continue;
            float tx = 0, ty = 0, tz = 0;
            TryRead<float>(tScene + CGameSceneNode::m_vecAbsOrigin + 0, tx);
            TryRead<float>(tScene + CGameSceneNode::m_vecAbsOrigin + 4, ty);
            TryRead<float>(tScene + CGameSceneNode::m_vecAbsOrigin + 8, tz);

            float dx = tx - lx, dy = ty - ly, dz = tz - lz;
            float distSq = dx * dx + dy * dy + dz * dz;
            if (distSq > kKnifeRangeSq) continue;
            if (std::abs(dz) > 80.0f) continue; // слишком высоко/низко

            // Проверяем что мы смотрим примерно в сторону врага (FOV < 90°).
            float angToEnemy = std::atan2(dy, dx) * 180.0f / 3.14159265f;
            float deltaYaw = angToEnemy - vaY;
            while (deltaYaw > 180.0f) deltaYaw -= 360.0f;
            while (deltaYaw < -180.0f) deltaYaw += 360.0f;
            if (std::abs(deltaYaw) > 90.0f) continue; // не смотрим на него

            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                foundTarget = true;

                // Проверяем backstab.
                float enemyYaw = 0;
                TryRead<float>(pawn + C_CSPlayerPawn::m_entitySpottedState + 0x0, enemyYaw);
                // Fallback: используем view angles врага через EyeAngles (нет прямого оффсета).
                // Простой способ: читаем m_angEyeAngles если есть, иначе используем velocity direction.
                float evx = 0, evy = 0;
                TryRead<float>(pawn + OFF.m_vecVelocity + 0, evx);
                TryRead<float>(pawn + OFF.m_vecVelocity + 4, evy);
                float eSpeed = std::sqrt(evx * evx + evy * evy);
                if (eSpeed > 10.0f) {
                    enemyYaw = std::atan2(evy, evx) * 180.0f / 3.14159265f;
                }
                bestIsBack = IsBackstab(lx, ly, tx, ty, enemyYaw);
            }
        }

        if (foundTarget) {
            // Backstab = правая кнопка (secondary attack), обычный = левая.
            // В CS2 backstab определяется сервером по позиции, но secondary attack
            // (правая кнопка) делает stab (65 dmg front / 180 dmg back) vs slash (40 dmg).
            // Всегда лучше stab (правая кнопка) — 65 front vs 40 slash.
            // Используем secondary attack (stab) всегда — он сильнее.
            // dwForceAttack = primary, dwForceAttack2 = secondary.
            // Но dwForceAttack2 нет в текущих оффсетах. Используем primary (slash).
            // Если враг спиной — primary тоже даёт backstab (сервер решает по позиции).
            TryWrite<int>(client + OFF.dwForceAttack, 65537);
            s_attacking = true;
            s_lastStab = now;
        } else {
            if (s_attacking) {
                TryWrite<int>(client + OFF.dwForceAttack, 256);
                s_attacking = false;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (s_attacking) {
            uintptr_t c = memory::GetClientBase();
            if (c) TryWrite<int>(c + OFF.dwForceAttack, 256);
            s_attacking = false;
        }
    }
}

} // namespace combat
