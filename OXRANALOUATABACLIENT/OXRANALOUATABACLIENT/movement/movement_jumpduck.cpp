#include "movement_jumpduck.hpp"
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

namespace movement {

// ============================================================================
// Jump-Duck: по бинду одновременно jump + duck.
// Используется для пролезания в проёмы (вентиляции, boost-позиции).
// ============================================================================
void UpdateJumpDuck(bool cs2Active)
{
    if (!cs2Active || !config::g_jumpDuckEnabled) return;
    if (config::g_jumpDuckKey == 0) return;

    // Проверяем зажата ли кнопка.
    if (!(GetAsyncKeyState(config::g_jumpDuckKey) & 0x8000)) return;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    __try {
        uintptr_t localPawn = 0;
        if (!TryRead<uintptr_t>(client + OFF.dwLocalPlayerPawn, localPawn) || !localPawn) return;
        int hp = 0;
        if (!TryRead<int>(localPawn + OFF.m_iHealth, hp) || hp <= 0) return;

        // Жмём jump + duck одновременно.
        TryWrite<int>(client + OFF.dwForceJump, 65537);

        // Duck через CSGOInput. В CS2 duck = buttons bit 2 (IN_DUCK = 4).
        // Но у нас нет dwForceDuck в оффсетах. Используем keybd_event как fallback.
        // Лучший вариант: симулируем через keybd_event(VK_LCONTROL).
        static bool s_duckSent = false;
        if (!s_duckSent) {
            keybd_event(VK_LCONTROL, 0, 0, 0); // press
            s_duckSent = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

    // Release duck когда кнопка отпущена — обрабатывается в следующем кадре.
    static bool s_wasHeld = false;
    bool held = (GetAsyncKeyState(config::g_jumpDuckKey) & 0x8000) != 0;
    if (s_wasHeld && !held) {
        keybd_event(VK_LCONTROL, 0, KEYEVENTF_KEYUP, 0);
    }
    s_wasHeld = held;
}

// ============================================================================
// Jump Scout: автовыстрел в момент приземления.
//
// В CS2 при прыжке с SSG-08 (Scout) точность восстанавливается в последний
// момент перед приземлением (когда velocity.z переходит из отрицательной
// в ~0 или когда FL_ONGROUND появляется). Это "jump scout" техника.
//
// Логика:
//   1. Проверяем что в руках SSG (defIndex 40) или AWP (9).
//   2. Проверяем что мы в воздухе (FL_ONGROUND = 0).
//   3. Отслеживаем velocity.z — когда она была < -50 и стала > -30
//      (момент перед приземлением) → стреляем.
//   4. Работает только когда зажат aim key (чтобы не стрелять случайно).
// ============================================================================

namespace {
    float s_prevVelZ = 0.0f;
    bool  s_wasInAir = false;
    bool  s_shotFired = false;
}

void UpdateJumpScout(bool cs2Active)
{
    if (!cs2Active || !config::g_jumpScoutEnabled) return;

    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    using namespace cs2_dumper::schemas::client_dll;

    __try {
        uintptr_t localPawn = 0;
        if (!TryRead<uintptr_t>(client + OFF.dwLocalPlayerPawn, localPawn) || !localPawn) return;
        int hp = 0;
        if (!TryRead<int>(localPawn + OFF.m_iHealth, hp) || hp <= 0) return;

        // Проверяем aim key зажат.
        bool aimHeld = (config::g_aimbotKey != 0 && (GetAsyncKeyState(config::g_aimbotKey) & 0x8000));
        if (!aimHeld) {
            s_wasInAir = false;
            s_shotFired = false;
            return;
        }

        // Проверяем оружие — только SSG (40) и AWP (9).
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

        if (defIdx != 40 && defIdx != 9) {
            s_wasInAir = false;
            s_shotFired = false;
            return;
        }

        // Flags и velocity.
        uint32_t flags = 0;
        TryRead<uint32_t>(localPawn + OFF.m_fFlags, flags);
        bool onGround = (flags & 1) != 0; // FL_ONGROUND = bit 0

        float velZ = 0.0f;
        TryRead<float>(localPawn + OFF.m_vecVelocity + 8, velZ);

        if (!onGround) {
            s_wasInAir = true;
            s_shotFired = false;
        }

        // Момент приземления: были в воздухе, velocity.z была сильно отрицательная,
        // сейчас стала близка к 0 или стали на земле.
        if (s_wasInAir && !s_shotFired) {
            bool landingMoment = false;

            // Вариант 1: velocity.z перешла из < -50 в > -30 (замедление перед землёй).
            if (s_prevVelZ < -50.0f && velZ > -30.0f) {
                landingMoment = true;
            }
            // Вариант 2: только что приземлились (onGround стал true).
            if (onGround && s_prevVelZ < -20.0f) {
                landingMoment = true;
            }

            if (landingMoment) {
                // Стреляем!
                TryWrite<int>(client + OFF.dwForceAttack, 65537);
                s_shotFired = true;
                s_wasInAir = false;

                // Отпускаем через ~30ms (в следующих кадрах).
                // Простой подход: ставим таймер.
                static ULONGLONG s_fireTime = 0;
                s_fireTime = GetTickCount64();
            }
        }

        // Отпускаем attack после выстрела.
        if (s_shotFired) {
            static ULONGLONG s_releaseAfter = 0;
            if (s_releaseAfter == 0) s_releaseAfter = GetTickCount64();
            if (GetTickCount64() - s_releaseAfter > 30) {
                TryWrite<int>(client + OFF.dwForceAttack, 256);
                s_releaseAfter = 0;
                // Reset для следующего прыжка.
                if (onGround) {
                    s_wasInAir = false;
                    s_shotFired = false;
                }
            }
        }

        s_prevVelZ = velZ;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

} // namespace movement
