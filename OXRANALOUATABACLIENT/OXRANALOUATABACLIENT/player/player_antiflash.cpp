#include "player_antiflash.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

// CS2 dumper includes
#include "../../../output/client_dll.hpp"

#include <Windows.h>

namespace player {

void UpdateAntiFlash(bool cs2Active)
{
	if (!cs2Active || !config::g_antiFlashEnabled)
		return;

	using namespace cs2_dumper;
	using namespace cs2_dumper::schemas::client_dll;

	uintptr_t client = memory::GetClientBase();
	if (!client)
		return;

	__try
	{
		auto& offsets = memory::GetOffsets();
		
		uintptr_t localPawn = 0;
		if (!memory::TryRead<uintptr_t>(client + offsets.dwLocalPlayerPawn, localPawn))
			return;
		if (!localPawn)
			return;
		
		(void)memory::TryWrite<float>(localPawn + C_CSPlayerPawnBase::m_flFlashDuration, 0.0f);
		(void)memory::TryWrite<float>(localPawn + C_CSPlayerPawnBase::m_flFlashMaxAlpha, 0.0f);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

} // namespace player
