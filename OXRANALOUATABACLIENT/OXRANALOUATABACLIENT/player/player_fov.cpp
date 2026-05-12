#include "player_fov.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// CS2 dumper includes
#include "../../../output/client_dll.hpp"

#include <Windows.h>

namespace player {

void UpdateFovOverride(bool cs2Active)
{
	if (!cs2Active || !config::g_fovEnabled)
		return;

	uintptr_t client = memory::GetClientBase();
	if (!client)
		return;

	using namespace cs2_dumper;
	using namespace cs2_dumper::schemas::client_dll;

	__try
	{
		auto& offsets = memory::GetOffsets();
		
		uintptr_t localPawn = 0;
		if (!memory::TryRead<uintptr_t>(client + offsets.dwLocalPlayerPawn, localPawn))
			return;
		if (!localPawn)
			return;

		// ИСПРАВЛЕНИЕ: Не меняем FOV в режиме наблюдателя (когда HP <= 0)
		int localHp = 0;
		if (!memory::TryRead<int>(localPawn + C_BaseEntity::m_iHealth, localHp))
			return;
		if (localHp <= 0)
			return; // Мы мертвы или наблюдаем - не трогаем FOV

		// Не меняем FOV, если в прицеле (чтобы не ломать зум AWP/Scout)
		bool isScoped = false;
		(void)memory::TryRead<bool>(localPawn + C_CSPlayerPawn::m_bIsScoped, isScoped);
		if (isScoped)
			return;

		uintptr_t camServices = 0;
		if (!memory::TryRead<uintptr_t>(localPawn + C_BasePlayerPawn::m_pCameraServices, camServices))
			return;
		if (!camServices)
			return;

		uint32_t neededFov = static_cast<uint32_t>(config::g_fovValue);
		
		// ИСПРАВЛЕНИЕ МЕРЦАНИЯ: Выключаем скорость смены FOV и пишем каждый кадр
		(void)memory::TryWrite<float>(camServices + CCSPlayerBase_CameraServices::m_flFOVTime, 0.0f);
		(void)memory::TryWrite<float>(camServices + CCSPlayerBase_CameraServices::m_flFOVRate, 0.0f);
		
		// Пишем сразу в оба значения каждую итерацию
		(void)memory::TryWrite<uint32_t>(camServices + CCSPlayerBase_CameraServices::m_iFOV, neededFov);
		(void)memory::TryWrite<uint32_t>(camServices + CCSPlayerBase_CameraServices::m_iFOVStart, neededFov);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

} // namespace player
