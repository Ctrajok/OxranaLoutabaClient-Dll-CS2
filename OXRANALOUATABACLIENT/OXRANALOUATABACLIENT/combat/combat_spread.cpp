#include "combat_spread.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"
#include "../config/config_vars.hpp"

#include <Windows.h>

// CS2 Dumper includes
#include "../../output/offsets.hpp"
#include "../../output/client_dll.hpp"

// Temporary aliases for compatibility
template <typename T>
static bool TryRead(uintptr_t address, T& out)
{
    return memory::TryRead(address, out);
}

template <typename T>
static bool TryWrite(uintptr_t address, const T& value)
{
    return memory::TryWrite(address, value);
}

#define g_offsetsRuntime memory::GetOffsets()

namespace combat {

void UpdateNoSpread(bool cs2Active)
{
    if (!cs2Active)
        return;
    if (!config::g_noSpreadEnabled)
        return;

    using namespace cs2_dumper;
    using namespace cs2_dumper::schemas::client_dll;

    uintptr_t client = memory::GetClientBase();
    if (!client)
        return;

    __try
    {
        uintptr_t localPawn = 0;
        if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerPawn, localPawn))
            return;
        if (!localPawn)
            return;

        int hp = 0;
        if (!TryRead<int>(localPawn + C_BaseEntity::m_iHealth, hp))
            return;
        if (hp <= 0)
            return;

        // No Spread logic
        uintptr_t weaponServices = 0;
        if (!TryRead<uintptr_t>(localPawn + C_BasePlayerPawn::m_pWeaponServices, weaponServices))
            return;
        if (!weaponServices)
            return;
        uint32_t hActive = 0;
        if (!TryRead<uint32_t>(weaponServices + CPlayer_WeaponServices::m_hActiveWeapon, hActive))
            return;
        if (!hActive)
            return;
        uintptr_t entityList = 0;
        if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwEntityList, entityList))
            return;
        if (!entityList)
            return;

        int weIndex = hActive & 0x1FF;
        int weEntryIndex = (hActive & 0x7FFF) >> 9;
        uintptr_t weListEntry = 0;
        if (!TryRead<uintptr_t>(entityList + 0x8 * weEntryIndex + 0x10, weListEntry))
            return;
        if (!weListEntry)
            return;
        uintptr_t weaponEnt = 0;
        if (!TryRead<uintptr_t>(weListEntry + 0x70 * weIndex, weaponEnt))
            return;
        if (!weaponEnt)
            return;

        // Сбрасываем все известные модификаторы точности
        (void)TryWrite<float>(weaponEnt + C_CSWeaponBase::m_fAccuracyPenalty, 0.0f);
        (void)TryWrite<float>(weaponEnt + C_CSWeaponBase::m_flRecoilIndex, 0.0f);
        (void)TryWrite<float>(weaponEnt + C_CSWeaponBase::m_flTurningInaccuracy, 0.0f);
        (void)TryWrite<float>(weaponEnt + C_CSWeaponBase::m_flTurningInaccuracyDelta, 0.0f);
        (void)TryWrite<float>(weaponEnt + C_CSWeaponBase::m_fAccuracySmoothedForZoom, 0.0f);

        // ИСПРАВЛЕНИЕ БАГА: Блок VData ЗАКОММЕНТИРОВАН!
        // Запись в VData ломает кэш движка и вызывает пропадание рук/оружия/звуков при смене раунда.
        // Для NoSpread достаточно обнуления m_fAccuracyPenalty и m_flRecoilIndex выше.
        /*
        uintptr_t vData = 0;
        if (TryRead<uintptr_t>(weaponEnt + 0x380, vData) && vData)
        {
            (void)TryWrite<float>(vData + 0x228, 0.0f); // m_flMaxInaccuracy
            (void)TryWrite<float>(vData + 0x22C, 0.0f); // m_flInaccuracyJumpInitial
        }
        */

        // Velocity compensation for better spread control while moving
        float* vecVel = reinterpret_cast<float*>(localPawn + C_BaseEntity::m_vecVelocity);
        if (vecVel) {
            // We don't zero velocity as that would break movement, 
            // but the game uses it to calculate spread. 
            // The offsets above should cover it, but some weapons have specific logic.
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Exception handler for no-spread - no cleanup needed
    }
}

} // namespace combat
