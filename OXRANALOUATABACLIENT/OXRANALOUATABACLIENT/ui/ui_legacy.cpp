#include <Windows.h>

#include <d3d11.h>
#include <wincodec.h>
#include <string.h>
#include "../core/core_d3d.hpp"


#define _USE_MATH_DEFINES
#include <cmath>

#include "../config/config_vars.hpp"
#include "../config/config_manager.hpp"
#include "../esp/esp_common.hpp"
#include "../esp/esp_oof_arrows.hpp"
#include "../esp/esp_bones.hpp"
#include "../esp/esp_snaplines.hpp"
#include "../esp/esp_distance.hpp"
#include "../esp/esp_spectators.hpp"
#include "../esp/esp_damage.hpp"
#include "../esp/esp_bomb.hpp"
#include "../esp/esp_box.hpp"
#include "../esp/esp_health.hpp"
#include "../esp/esp_name.hpp"
#include "../esp/esp_weapon.hpp"

#include "../combat/combat_aimbot.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

#include "../audio/audio_hitsound.hpp"

#include "ui_theme.hpp"
#include "ui_cyber_widgets.hpp"

// Use modular offsets parsed from output/ headers (same pattern as esp_common.cpp)
#define g_offsetsRuntime memory::GetOffsets()
#include "../../../imgui-1.92.5/imgui.h"
#include "../../../imgui-1.92.5/imgui_internal.h"
#include "../../../output/offsets.hpp"
#include "../../../output/client_dll.hpp"

// external vars
extern float g_hitmarkerAlpha;
extern ULONGLONG g_hitmarkerTime;
extern LPVOID g_hitSoundBuffer;

template <typename T>
static bool TryRead(uintptr_t address, T& out) { return memory::TryRead(address, out); }
template <typename T>
static bool TryWrite(uintptr_t address, const T& val) { return memory::TryWrite(address, val); }

// Missing globals from legacy
struct StringBuf64 { char data[64]; };

static int spectatorCount = 0;
static StringBuf64 tempSpectators[64];

static int targetCount = 0;
static esp::EspTargetWorld tempTargets[256];

static ULONGLONG g_gameTimeBaseTickMs = 0;
static float g_gameTimeBaseSeconds = 0.0f;
static bool g_isLocalSniperScoped = false;

// RuntimeOffsets now comes from memory::GetOffsets() via #define g_offsetsRuntime above
// Offsets are auto-parsed from output/ headers (offsets.hpp, client_dll.hpp)


namespace esp {
    struct QAngle { float x, y, z; };
    struct vpPtr { float m[4][4]; };
    
}

extern HWND g_hOverlayWnd;  // defined in core_main.cpp
static bool g_skinChangerEnabled = false;
static int g_skinUpdateCounter = 0;
extern HMODULE g_hModule;   // defined in core_main.cpp



static bool GetDllDirA(std::string& out) {
    extern HMODULE g_hModule;
    char path[MAX_PATH];
    if (!GetModuleFileNameA(g_hModule, path, MAX_PATH)) { out = ""; return false; }
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos == std::string::npos) { out = ""; return false; }
    out = fullPath.substr(0, pos);
    return true;
}


struct MockSkinConfig {
    int paint_kit_index;
    int seed;
    float wear;
};
#include <map>
static std::map<int, MockSkinConfig> g_skinConfig;

// g_offsetsRuntime is now a #define to memory::GetOffsets() (see top of file)


void DrawMenuImGui();
// Menu state variables now in config namespace

void ApplyClientStyle();
static bool KeybindWidget(const char* id, int& vk);
static const char* VkToStringA(int vk);

// Stream Proof / Anti-Capture
// (now using config::g_antiCaptureEnabled)

// External features
// (now using config namespace variables)

// Use config namespace variables
using namespace config;

// Use ESP accessor functions
static auto& g_espBoxes = esp::GetEspBoxes();
static auto& g_espTargetsWorld = esp::GetEspTargetsWorld();
static auto& g_boneCache = esp::GetBoneCache();
static auto& g_spectators = esp::GetSpectators();

// HitSound & Hitmarker
bool g_hitSoundEnabled = false;
bool g_hitmarkerEnabled = false;
static float g_hitmarkerAlpha = 0.0f;
static ULONGLONG g_hitmarkerTime = 0;
static std::map<uintptr_t, int> g_lastEnemyHp; // pawn -> last known hp

// Кэш звука для моментального воспроизведения
LPVOID g_hitSoundBuffer = nullptr;
DWORD  g_hitSoundSize = 0;
static ULONGLONG g_lastLocalShotTime = 0;
static int g_prevTotalHitsOnServer = 0;
static int g_prevShotsFired = 0;
static ULONGLONG g_lastConfirmedHitTime = 0;
// Pending bullet-hit waiting for enemy HP confirmation (to suppress teammate FF hits).
static bool g_pendingBulletHit = false;
static ULONGLONG g_pendingBulletHitTime = 0;

// g_autoFireActive moved to combat/combat_aimbot.cpp

// Keybinds List
// (now using config::g_keybindsListEnabled)

// Gun ESP text offset (X/Y)
// (now using config::g_gunOffsetX)

// UpdateFovOverride moved to player/player_fov.cpp



// InitHooks and RemoveHooks moved to hooks/hooks_manager.cpp

// UpdateSkinChanger - УДАЛЕНА (устаревшая, заменена на UpdateSkinChangerHooked)

static bool IsCs2Active()
{
	HWND hwnd = GetForegroundWindow();
	if (!hwnd) return false;

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	return pid == GetCurrentProcessId();
}

// WorldToScreen - made non-static for use by combat modules
bool esp::WorldToScreen(const float m[16], const Vec3& pos, int width, int height, Vec2& out)
{
	float w = m[12] * pos.x + m[13] * pos.y + m[14] * pos.z + m[15];
	if (w < 0.001f) return false;

	float x = m[0] * pos.x + m[1] * pos.y + m[2] * pos.z + m[3];
	float y = m[4] * pos.x + m[5] * pos.y + m[6] * pos.z + m[7];

	float invW = 1.0f / w;
	x *= invW;
	y *= invW;

	float cx = width / 2.0f;
	float cy = height / 2.0f;

	out.x = cx + (cx * x);
	out.y = cy - (cy * y);
	return true;
}

// Получение имени оружия по defIndex
static const char* GetWeaponName(int defIndex)
{
	switch(defIndex) {
		case 1: return "DEAGLE";
		case 2: return "ELITE";
		case 3: return "FIVESEVEN";
		case 4: return "GLOCK";
		case 7: return "AK-47";
		case 8: return "AUG";
		case 9: return "AWP";
		case 10: return "FAMAS";
		case 11: return "G3SG1";
		case 13: return "GALIL";
		case 14: return "M249";
		case 16: return "M4A4";
		case 17: return "MAC10";
		case 19: return "P90";
		case 23: return "MP5";
		case 24: return "UMP45";
		case 25: return "XM1014";
		case 26: return "BIZON";
		case 27: return "MAG7";
		case 28: return "NEGEV";
		case 29: return "SAWEDOFF";
		case 30: return "TEC9";
		case 31: return "ZEUS";
		case 32: return "P2000";
		case 33: return "MP7";
		case 34: return "MP9";
		case 35: return "NOVA";
		case 36: return "P250";
		case 38: return "SCAR20";
		case 39: return "SG556";
		case 40: return "SCOUT";
		case 41: return "KNIFE";
		case 42: return "KNIFE";
		case 43: return "FLASH";
		case 44: return "HE";
		case 45: return "SMOKE";
		case 46: return "MOLOTOV";
		case 47: return "DECOY";
		case 48: return "INCENDIARY";
		case 49: return "C4";
		case 59: return "KNIFE";
		case 60: return "M4A1-S";
		case 61: return "USP-S";
		case 63: return "CZ75";
		case 64: return "REVOLVER";
		case 500: return "BAYONET";
		case 503: return "CLASSIC";
		case 505: return "FLIP";
		case 506: return "GUT";
		case 507: return "KARAMBIT";
		case 508: return "M9";
		case 509: return "HUNTSMAN";
		case 512: return "FALCHION";
		case 514: return "BOWIE";
		case 515: return "BUTTERFLY";
		case 516: return "DAGGERS";
		default: return "WEAPON";
	}
}

// ИСПРАВЛЕНИЕ C2712: Структура для временного хранения зрителей
struct SpectatorTemp {
	char name[64];
	bool valid;
};

// ИСПРАВЛЕНИЕ C2712: Helper-функция без C++ объектов для использования __try/__except
static void UpdateESPInternal_TryBlock(
	uintptr_t client,
	esp::EspTargetWorld* tempTargets,
	int* targetCount,
	SpectatorTemp* tempSpectators,
	int* spectatorCount
)
{
	__try
	{
		using namespace cs2_dumper;
		using namespace cs2_dumper::schemas::client_dll;

		// ИСПРАВЛЕНИЕ: Строгая проверка валидности локального игрока
		uintptr_t localPawn = 0;
		if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerPawn, localPawn))
			return;
		
		// 1. Проверка на nullptr (мы в меню)
		if (!localPawn) {
			return;
		}
		
		// 2. ПРОВЕРКА ОТ КРАША: Проверяем GameSceneNode у себя. Если его нет - мир не загружен.
		uintptr_t localScene = 0;
		if (!TryRead<uintptr_t>(localPawn + C_BaseEntity::m_pGameSceneNode, localScene))
			return;
		if (!localScene) {
			return;
		}
		
		// 3. ИСПРАВЛЕНИЕ: НЕ проверяем HP локального игрока!
		// При слежке за другим игроком (spectator mode) наш HP = 0, но ESP должен работать.
		// Проверка здоровья нужна только для защиты от мусорных значений при смене карты.
		int localHp = 0;
		if (!TryRead<int>(localPawn + C_BaseEntity::m_iHealth, localHp))
			return;
		// Разрешаем HP от -100 до 10000 (включая 0 для режима наблюдателя)
		if (localHp < -100 || localHp > 10000) {
			return;
		}
		
		// ОПТИМИЗАЦИЯ: Проверяем снайперку здесь, а не в рендере
		g_isLocalSniperScoped = false;
		uintptr_t weaponServices = 0;
		if (TryRead<uintptr_t>(localPawn + C_BasePlayerPawn::m_pWeaponServices, weaponServices) && weaponServices)
		{
			uint32_t hActive = 0;
			if (TryRead<uint32_t>(weaponServices + CPlayer_WeaponServices::m_hActiveWeapon, hActive) && hActive)
			{
				uintptr_t entityList = 0;
				if (TryRead<uintptr_t>(client + g_offsetsRuntime.dwEntityList, entityList) && entityList)
				{
					int weIndex = hActive & 0x1FF;
					int weEntryIndex = (hActive & 0x7FFF) >> 9;
					uintptr_t weList = 0;
					if (TryRead<uintptr_t>(entityList + 0x8 * weEntryIndex + 0x10, weList) && weList)
					{
						uintptr_t weaponEnt = 0;
						if (TryRead<uintptr_t>(weList + 0x70 * weIndex, weaponEnt) && weaponEnt)
						{
							uintptr_t itemView = weaponEnt + C_EconEntity::m_AttributeManager + C_AttributeContainer::m_Item;
							uint16_t defIndex = 0;
							if (TryRead<uint16_t>(itemView + C_EconItemView::m_iItemDefinitionIndex, defIndex))
							{
								// 9 = AWP, 40 = SSG08 (Scout)
								g_isLocalSniperScoped = (defIndex == 9 || defIndex == 40);
							}
						}
					}
				}
			}
		}

		uintptr_t entityList = 0;
		if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwEntityList, entityList))
			return;
		if (!entityList) return;
		
		// ИСПРАВЛЕНИЕ: При слежке (HP <= 0) не проверяем команду, показываем всех
		bool isSpectating = (localHp <= 0);
		
		int localTeam = 0;
		bool hasLocalTeam = false;
		if (localPawn && !isSpectating)
		{
			if (!TryRead<int>(localPawn + C_BaseEntity::m_iTeamNum, localTeam))
				localTeam = 0;
			hasLocalTeam = (localTeam >= 2 && localTeam <= 3);
		}

		// Импульс отключения радара (одиночный сброс флага при выключении)
		static bool s_wasRadarHackEnabled = false;
		bool radarDisablePulse = false;
		if (s_wasRadarHackEnabled && !config::g_radarHackEnabled) {
			radarDisablePulse = true;
		}
		s_wasRadarHackEnabled = config::g_radarHackEnabled;

		// Находим наш локальный Pawn Handle для Spectator List
		uint32_t localPawnHandle = 0;
		uintptr_t localCtrl = 0;
		if (TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerController, localCtrl) && localCtrl) {
			TryRead<uint32_t>(localCtrl + g_offsetsRuntime.m_hPlayerPawn, localPawnHandle);
		}

		for (int id = 1; id <= 64; ++id)
		{
			uintptr_t listEntry = 0;
			if (!TryRead<uintptr_t>(entityList + 0x8 * ((id & 0x7FFF) >> 9) + 0x10, listEntry) || !listEntry) continue;

			uintptr_t controller = 0;
			if (!TryRead<uintptr_t>(listEntry + 0x70 * (id & 0x1FF), controller) || !controller) continue;

			// --- ИДЕАЛЬНЫЙ SPECTATOR CHECK (работает даже когда мертвы) ---
			uint32_t pawnHandle = 0, obsPawnHandle = 0;
			TryRead<uint32_t>(controller + g_offsetsRuntime.m_hPlayerPawn, pawnHandle);
			TryRead<uint32_t>(controller + g_offsetsRuntime.m_hObserverPawn, obsPawnHandle);

			bool isAlive = false;
			uintptr_t activePawnToCheck = 0;

			// 1. Проверяем основную пешку игрока (жив ли он?)
			if (pawnHandle) {
				int pIdx = pawnHandle & 0x1FF;
				int pEntry = (pawnHandle & 0x7FFF) >> 9;
				uintptr_t pList = 0;
				if (TryRead<uintptr_t>(entityList + 0x8 * pEntry + 0x10, pList) && pList) {
					uintptr_t actualPawn = 0;
					if (TryRead<uintptr_t>(pList + 0x70 * pIdx, actualPawn) && actualPawn) {
						int actualHp = 0;
						TryRead<int>(actualPawn + g_offsetsRuntime.m_iHealth, actualHp);
						if (actualHp > 0) {
							isAlive = true; // Игрок возродился, пропускаем проверку зрителя
						} else {
							activePawnToCheck = actualPawn; // Игрок мертв, запоминаем его пешку
						}
					}
				}
			}

			// 2. Если игрок мертв, но у него есть активная пешка обсервера, отдаем ей приоритет
			if (!isAlive && obsPawnHandle) {
				int pIdx = obsPawnHandle & 0x1FF;
				int pEntry = (obsPawnHandle & 0x7FFF) >> 9;
				uintptr_t pList = 0;
				if (TryRead<uintptr_t>(entityList + 0x8 * pEntry + 0x10, pList) && pList) {
					uintptr_t obsPawn = 0;
					if (TryRead<uintptr_t>(pList + 0x70 * pIdx, obsPawn) && obsPawn) {
						activePawnToCheck = obsPawn;
					}
				}
			}

			// 3. Только если игрок мертв (isAlive == false), проверяем, за кем он следит
			if (!isAlive && activePawnToCheck && localPawnHandle && *spectatorCount < 64 && controller != localCtrl) {
				uintptr_t obsServices = 0;
				if (TryRead<uintptr_t>(activePawnToCheck + g_offsetsRuntime.m_pObserverServices, obsServices) && obsServices) {
					uint32_t obsTarget = 0;
					if (TryRead<uint32_t>(obsServices + g_offsetsRuntime.m_hObserverTarget, obsTarget) && obsTarget) {
						// Если цель обсервера - МЫ, добавляем в список
						if (obsTarget == localPawnHandle && localHp > 0) {
							StringBuf64 specName = {0};
							if (TryRead<StringBuf64>(controller + g_offsetsRuntime.m_iszPlayerName, specName)) {
								specName.data[63] = '\0';
								// Исключаем пустые имена
								if (specName.data[0] != '\0') {
									memcpy(tempSpectators[*spectatorCount].name, specName.data, 64);
									tempSpectators[*spectatorCount].valid = true;
									(*spectatorCount)++;
								}
							}
						}
					}
				}
			}
			// --- КОНЕЦ SPECTATOR CHECK ---

			// --- PAWN LOGIC ДЛЯ ESP И HITSOUND ---
			if (!pawnHandle) continue;
			int pawnIndex = pawnHandle & 0x1FF;
			int pawnEntryIndex = (pawnHandle & 0x7FFF) >> 9;
			uintptr_t pawnListEntry = 0;
			if (!TryRead<uintptr_t>(entityList + 0x8 * pawnEntryIndex + 0x10, pawnListEntry) || !pawnListEntry) continue;
			uintptr_t pawn = 0;
			if (!TryRead<uintptr_t>(pawnListEntry + 0x70 * pawnIndex, pawn) || !pawn) continue;
			if (localPawn && pawn == localPawn) continue;

			// СНАЧАЛА проверяем команду (чтобы не реагировать на урон по своим)
			int team = 0;
			if (!TryRead<int>(pawn + g_offsetsRuntime.m_iTeamNum, team)) continue;
			if (team < 2 || team > 3) continue;
			if (!isSpectating && hasLocalTeam && team == localTeam) continue;

			int hp = 0;
			if (!TryRead<int>(pawn + g_offsetsRuntime.m_iHealth, hp)) continue;
			if (hp <= 0 || hp > 200) continue; // Мертвецов не рисуем в ESP

			// --- GLOW (свечение сквозь стены) ---
				if (config::g_chamsEnabled)
				{
					ImVec4 c = (team == 2) ? config::g_chamsColorT : config::g_chamsColorCT;
					uintptr_t pGlow = pawn + C_BaseModelEntity::m_Glow;

					(void)TryWrite<bool>(pGlow + 0x51, true);   // m_bGlowing
					(void)TryWrite<int> (pGlow + 0x30, 3);      // Type 3 = только контур (outline)
					
					struct ColorRGBA { uint8_t r, g, b, a; };
					ColorRGBA glowColor = {
						(uint8_t)(c.x * 255.0f), (uint8_t)(c.y * 255.0f), (uint8_t)(c.z * 255.0f), (uint8_t)(c.w * 255.0f)
					};
					(void)TryWrite<ColorRGBA>(pGlow + 0x40, glowColor);

					// ИСПРАВЛЕНИЕ: true = рендерить сквозь стены!
					(void)TryWrite<bool>(pGlow + 0x52, true);   // m_bRenderWhenOccluded
					(void)TryWrite<bool>(pGlow + 0x53, true);   // m_bRenderWhenUnoccluded
				}
				else
				{
					// Сброс при выключении
					uintptr_t pGlow = pawn + C_BaseModelEntity::m_Glow;
					(void)TryWrite<bool>(pGlow + 0x51, false);
				}

				// Читаем origin, mins, maxs для ESP бокса
				Vec3 origin{}, mins{}, maxs{};
				uintptr_t gameScene = 0;
				if (TryRead<uintptr_t>(pawn + C_BaseEntity::m_pGameSceneNode, gameScene) && gameScene) {
					(void)TryRead<float>(gameScene + CGameSceneNode::m_vecAbsOrigin + 0x0, origin.x);
					(void)TryRead<float>(gameScene + CGameSceneNode::m_vecAbsOrigin + 0x4, origin.y);
					(void)TryRead<float>(gameScene + CGameSceneNode::m_vecAbsOrigin + 0x8, origin.z);
					
					// Collision bounds для 2D бокса
					uintptr_t collision = 0;
					if (TryRead<uintptr_t>(pawn + C_BaseEntity::m_pCollision, collision) && collision) {
						(void)TryRead<float>(collision + CCollisionProperty::m_vecMins + 0x0, mins.x);
						(void)TryRead<float>(collision + CCollisionProperty::m_vecMins + 0x4, mins.y);
						(void)TryRead<float>(collision + CCollisionProperty::m_vecMins + 0x8, mins.z);
						(void)TryRead<float>(collision + CCollisionProperty::m_vecMaxs + 0x0, maxs.x);
						(void)TryRead<float>(collision + CCollisionProperty::m_vecMaxs + 0x4, maxs.y);
						(void)TryRead<float>(collision + CCollisionProperty::m_vecMaxs + 0x8, maxs.z);
					}
				}

				esp::EspTargetWorld tgt{};
				tgt.pawn = pawn;
				tgt.hp = hp;
				tgt.origin = origin;
				tgt.mins = mins;
				tgt.maxs = maxs;
				tgt.weaponIconIndex = -1;
				tgt.flashed = false;
				{
					// ОПТИМИЗАЦИЯ: Читаем имя блоком за 1 вызов TryRead
					StringBuf64 nameRaw = {0};
					if (TryRead<StringBuf64>(controller + CBasePlayerController::m_iszPlayerName, nameRaw)) {
						memcpy(tgt.name, nameRaw.data, 64);
						tgt.name[63] = '\0'; // Гарантируем закрытие строки
					} else {
						tgt.name[0] = '\0';
					}
				}
				{
					uintptr_t weaponServices = 0;
					if (!TryRead<uintptr_t>(pawn + C_BasePlayerPawn::m_pWeaponServices, weaponServices))
						weaponServices = 0;
					if (weaponServices)
					{
						uint32_t hActive = 0;
						if (!TryRead<uint32_t>(weaponServices + CPlayer_WeaponServices::m_hActiveWeapon, hActive))
							hActive = 0;
						if (hActive)
						{
							int weIndex = hActive & 0x1FF;
							int weEntryIndex = (hActive & 0x7FFF) >> 9;
							uintptr_t weListEntry = 0;
							if (!TryRead<uintptr_t>(entityList + 0x8 * weEntryIndex + 0x10, weListEntry))
								weListEntry = 0;
							if (weListEntry)
							{
								uintptr_t weaponEnt = 0;
								if (!TryRead<uintptr_t>(weListEntry + 0x70 * weIndex, weaponEnt))
									weaponEnt = 0;
								if (weaponEnt)
								{
									uintptr_t econEntity = weaponEnt;
									uintptr_t itemView = econEntity + C_EconEntity::m_AttributeManager + C_AttributeContainer::m_Item;
									uint16_t defIndex = 0;
									if (!TryRead<uint16_t>(itemView + C_EconItemView::m_iItemDefinitionIndex, defIndex))
										defIndex = 0;
									if (defIndex != 0)
									{
										// ИСПРАВЛЕНИЕ: Используем текстовое имя вместо иконок
										const char* weaponName = GetWeaponName((int)defIndex);
										strncpy_s(tgt.weapon, sizeof(tgt.weapon), weaponName, _TRUNCATE);
									}
								}
							}
						}
					}
				}

				float flashDur = 0.0f;
				if (!TryRead<float>(pawn + C_CSPlayerPawnBase::m_flFlashDuration, flashDur))
					flashDur = 0.0f;
				float flashEndTime = 0.0f;
				(void)TryRead<float>(pawn + C_CSPlayerPawnBase::m_flFlashBangTime, flashEndTime);

				// Синхронизация локального времени с "game time" по первому валидному значению.
				if (flashEndTime > 0.0f && g_gameTimeBaseTickMs <= 0.0) {
					g_gameTimeBaseTickMs = (double)GetTickCount64();
					// flashBangTime в игре обычно является "absolute game time" конца ослепления.
					// Поэтому базу ставим чуть раньше на flashDur.
					g_gameTimeBaseSeconds = (double)flashEndTime - (double)flashDur;
				}

				tgt.flashEndTime = flashEndTime;
				tgt.flashed = (flashDur > 0.0f);
				tgt.flashDur = flashDur; // Добавляем длительность флешки
				
				// RADAR HACK (Native) - Раздельная независимая проверка (использует 0x8)
				if (config::g_radarHackEnabled) {
					(void)TryWrite<bool>(pawn + C_CSPlayerPawn::m_entitySpottedState + 0x8, true);
				} else if (radarDisablePulse) {
					(void)TryWrite<bool>(pawn + C_CSPlayerPawn::m_entitySpottedState + 0x8, false);
				}
				
				// ИСПРАВЛЕНИЕ C2712: Записываем в POD-буфер вместо vector::push_back
				if (*targetCount < 64) {
					tempTargets[*targetCount] = tgt;
					(*targetCount)++;
				}
			}
		
		// --- Поиск установленной C4 (работает даже если игрок мертв) ---
		esp::GetBombData().found = false;
		if (g_offsetsRuntime.dwPlantedC4)
		{
			uintptr_t c4Ptr = 0;
			if (TryRead<uintptr_t>(client + g_offsetsRuntime.dwPlantedC4, c4Ptr) && c4Ptr)
			{
				uintptr_t c4Node = 0;
				if (TryRead<uintptr_t>(c4Ptr, c4Node) && c4Node)
				{
					bool isTicking = false;
					bool isDefused = false;
					(void)TryRead<bool>(c4Node + g_offsetsRuntime.m_bBombTicking, isTicking);
					(void)TryRead<bool>(c4Node + g_offsetsRuntime.m_bBombDefused, isDefused);
					
					if (isTicking && !isDefused) {
						uintptr_t gameScene = 0;
						if (TryRead<uintptr_t>(c4Node + g_offsetsRuntime.m_pGameSceneNode, gameScene) && gameScene)
						{
							auto& bombData = esp::GetBombData();
							TryRead<float>(gameScene + g_offsetsRuntime.m_vecAbsOrigin + 0x0, bombData.pos.x);
							TryRead<float>(gameScene + g_offsetsRuntime.m_vecAbsOrigin + 0x4, bombData.pos.y);
							TryRead<float>(gameScene + g_offsetsRuntime.m_vecAbsOrigin + 0x8, bombData.pos.z);
							
							if (bombData.pos.x != 0.0f || bombData.pos.y != 0.0f) {
								TryRead<float>(c4Node + g_offsetsRuntime.m_flC4Blow, bombData.blowTime);
								TryRead<bool>(c4Node + g_offsetsRuntime.m_bBeingDefused, bombData.isDefusing);
								bombData.found = true;
							}
						}
					}
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Вектор уже очищен в начале UpdateESPInternal
	}
}

// Основная функция UpdateESPInternal
void UpdateESPInternal(int width, int height)
{
	// ИСПРАВЛЕНИЕ C2712: Операции с std::vector ПЕРЕД вызовом helper-функции
	g_espTargetsWorld.clear();
	g_espBoxes.clear();
	g_spectators.clear(); // Принудительно очищаем в начале каждого тика
	
	// Сброс данных бомбы каждый кадр
	esp::GetBombData().found = false;

	uintptr_t client = memory::GetClientBase();
	if (!client) return;

	// ИСПРАВЛЕНИЕ C2712: Временные POD-буферы
	SpectatorTemp tempSpectators[64] = {};
	int spectatorCount = 0;
	
	esp::EspTargetWorld tempTargets[64] = {};
	int targetCount = 0;

	// Вызываем helper-функцию с __try/__except
	UpdateESPInternal_TryBlock(client, tempTargets, &targetCount, tempSpectators, &spectatorCount);

	// ИСПРАВЛЕНИЕ C2712: Переносим ESP цели из POD-буфера в вектор (вне __try блока)
	g_espTargetsWorld.reserve(targetCount);
	for (int i = 0; i < targetCount; ++i) {
		g_espTargetsWorld.push_back(tempTargets[i]);
	}

	// Переносим зрителей из временного буфера в вектор (вне __try блока)
	g_spectators.clear();
	for (int i = 0; i < spectatorCount; ++i) {
		if (tempSpectators[i].valid) {
			g_spectators.push_back(tempSpectators[i].name);
		}
	}
}

// RunBhop moved to movement/movement_bhop.cpp

// Нормализация углов (обязательно, чтобы не крашнуло и не крутило)
// ClampAngles and GetBonePos moved to combat/combat_common.cpp

// Aimbot moved to combat/combat_aimbot.cpp

// Функция для правильной записи углов через CInput (для Internal чита)
// Triggerbot moved to combat/combat_triggerbot.cpp

// Premium outlined text for ESP (black outline for readability on bright maps)
static void DrawOutlinedText(ImDrawList* dl, ImFont* font, float size, const ImVec2& pos, ImU32 color, const char* text)
{
	ImU32 outlineCol = IM_COL32(0, 0, 0, 255);
	dl->AddText(font, size, ImVec2(pos.x - 1, pos.y - 1), outlineCol, text);
	dl->AddText(font, size, ImVec2(pos.x + 1, pos.y - 1), outlineCol, text);
	dl->AddText(font, size, ImVec2(pos.x - 1, pos.y + 1), outlineCol, text);
	dl->AddText(font, size, ImVec2(pos.x + 1, pos.y + 1), outlineCol, text);
	dl->AddText(font, size, pos, color, text);
}

// Shot/Hit tracking via BulletServices::m_totalHitsOnServer (server-confirmed bullet hits only).
// Runs BEFORE UpdateHitInfo each frame; sets g_lastConfirmedHitTime which gates damage indicators.
void UpdateShotTracking(uintptr_t localPawn)
{
	if (!g_hitSoundEnabled && !g_hitmarkerEnabled && !config::g_damageIndicatorsEnabled) return;
	if (!localPawn) return;

	// 1) Track local shots (keeps g_lastLocalShotTime accurate for any legacy consumers).
	int shotsFired = 0;
	if (TryRead<int>(localPawn + g_offsetsRuntime.m_iShotsFired, shotsFired))
	{
		if (shotsFired > g_prevShotsFired)
			g_lastLocalShotTime = GetTickCount64();
		g_prevShotsFired = shotsFired;
	}

	// 2) Read BulletServices pointer.
	uintptr_t bulletServices = 0;
	if (!TryRead<uintptr_t>(localPawn + g_offsetsRuntime.m_pBulletServices, bulletServices) || !bulletServices)
		return;

	// 3) Read confirmed-hits counter.
	int totalHits = 0;
	if (!TryRead<int>(bulletServices + g_offsetsRuntime.m_totalHitsOnServer, totalHits))
		return;

	// 4) Counter grew => the local player just landed a bullet on the server.
	//    DON'T play sound yet: we must confirm an enemy (not a teammate / FF) actually lost HP.
	//    UpdateHitInfo runs right after this and will confirm the pending hit if enemy HP dropped.
	if (totalHits > g_prevTotalHitsOnServer)
	{
		g_lastConfirmedHitTime = GetTickCount64();
		g_pendingBulletHit = true;
		g_pendingBulletHitTime = g_lastConfirmedHitTime;
	}
	// totalHits < prev => round reset / respawn. Silently resync, no sound.

	// Expire a pending hit if no enemy HP drop was observed within 250ms
	// (e.g. teammate friendly-fire on casual/DM servers).
	if (g_pendingBulletHit && (GetTickCount64() - g_pendingBulletHitTime) > 250)
		g_pendingBulletHit = false;

	g_prevTotalHitsOnServer = totalHits;
}

// Instant HitSound detection - runs EVERY FRAME without ESP update limits
void UpdateHitInfo(uintptr_t client)
{
	if (!g_hitSoundEnabled && !g_hitmarkerEnabled && !config::g_damageIndicatorsEnabled) return;
	if (!client) return;

	uintptr_t entityList = 0;
	if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwEntityList, entityList) || !entityList) return;

	uintptr_t localPawn = 0;
	if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerPawn, localPawn) || !localPawn) return;
	
	int localTeam = 0;
	if (!TryRead<int>(localPawn + g_offsetsRuntime.m_iTeamNum, localTeam)) return;

	// Iterate all players (1-64)
	for (int id = 1; id <= 64; ++id)
	{
		uintptr_t listEntry = 0;
		if (!TryRead<uintptr_t>(entityList + 0x8 * ((id & 0x7FFF) >> 9) + 0x10, listEntry) || !listEntry) continue;

		uintptr_t controller = 0;
		if (!TryRead<uintptr_t>(listEntry + 0x70 * (id & 0x1FF), controller) || !controller) continue;

		uint32_t pawnHandle = 0;
		if (!TryRead<uint32_t>(controller + g_offsetsRuntime.m_hPlayerPawn, pawnHandle) || !pawnHandle) continue;

		int pawnIndex = pawnHandle & 0x1FF;
		int pawnEntryIndex = (pawnHandle & 0x7FFF) >> 9;
		uintptr_t pawnListEntry = 0;
		if (!TryRead<uintptr_t>(entityList + 0x8 * pawnEntryIndex + 0x10, pawnListEntry) || !pawnListEntry) continue;

		uintptr_t pawn = 0;
		if (!TryRead<uintptr_t>(pawnListEntry + 0x70 * pawnIndex, pawn) || !pawn) continue;
		if (pawn == localPawn) continue;

		int team = 0;
		if (!TryRead<int>(pawn + g_offsetsRuntime.m_iTeamNum, team)) continue;
		if (team == localTeam) continue; // Skip teammates

		int hp = 0;
		if (!TryRead<int>(pawn + g_offsetsRuntime.m_iHealth, hp)) continue;

		auto it = g_lastEnemyHp.find(pawn);
		if (it != g_lastEnemyHp.end())
		{
			int prevHp = it->second;
			if (hp < prevHp && prevHp > 0 && hp >= 0)
			{
				// Only count HP drop as our hit if the server confirmed a bullet hit in the last 150ms
				// (g_lastConfirmedHitTime is set by UpdateShotTracking when m_totalHitsOnServer increments).
				bool recentConfirmedHit = (GetTickCount64() - g_lastConfirmedHitTime) < 150;

				if (recentConfirmedHit)
				{
					int damage = prevHp - hp;

					// Consume a pending bullet-hit only when an actual ENEMY lost HP
					// (teammates were filtered out by `team == localTeam` check above).
					// This suppresses sound/hitmarker for friendly-fire hits on casual/DM servers.
					if (g_pendingBulletHit)
					{
						g_pendingBulletHit = false;

						if (g_hitSoundEnabled && g_hitSoundBuffer)
							audio::PlayHitSound(config::g_hitSoundVolume);

						if (g_hitmarkerEnabled)
						{
							g_hitmarkerAlpha = 1.0f;
							g_hitmarkerTime = GetTickCount64();
						}
					}

					if (config::g_damageIndicatorsEnabled && damage > 0)
					{
						Vec3 worldPos{};
						uintptr_t gameScene = 0;
						if (TryRead<uintptr_t>(pawn + g_offsetsRuntime.m_pGameSceneNode, gameScene) && gameScene)
						{
							TryRead<float>(gameScene + g_offsetsRuntime.m_vecAbsOrigin + 0x0, worldPos.x);
							TryRead<float>(gameScene + g_offsetsRuntime.m_vecAbsOrigin + 0x4, worldPos.y);
							TryRead<float>(gameScene + g_offsetsRuntime.m_vecAbsOrigin + 0x8, worldPos.z);
							worldPos.z += 40.0f;
						}
						esp::AddDamageIndicator(pawn, worldPos, damage);
					}
				}
			}
		}
		g_lastEnemyHp[pawn] = hp;
	}
}

// Helper for reading view matrix safely (SEH requires no objects with destructors)
static bool TryReadViewMatrix(uintptr_t client, float* viewMatrix)
{
	if (!client) return false;
	__try {
		for (int i = 0; i < 16; ++i) {
			viewMatrix[i] = *(float*)(client + g_offsetsRuntime.dwViewMatrix + i * sizeof(float));
		}
		return true;
	} __except(1) {}
	return false;
}

void DrawEspImGui()
{
	// Если ничего не включено, не рисуем (но hitmarker должен рисоваться!)
	if (!config::g_whEnabled && !config::g_bonesEnabled && !config::g_nameEspEnabled && !config::g_gunEspEnabled && !config::g_hpBarEnabled && !(config::g_aimbotEnabled && config::g_aimbotDrawFov) && !config::g_scopeSightEnabled && config::g_bombEspEnabled == false && g_hitmarkerAlpha <= 0.0f) return;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	// --- ОТРИСОВКА ESP БОКСОВ ---
	esp::RenderBoxes(drawList);

	// --- ОТРИСОВКА ESP (ИМЕНА, ОРУЖИЕ, HP BAR) ---
	esp::RenderHealthBars(drawList);
	esp::RenderNames(drawList);
	esp::RenderWeapons(drawList);

	// --- FLASH BAR ---
	for (const esp::EspBox& e : g_espBoxes)
	{
		const esp::Box2D& box = e.box;

		// Flash Bar (вместо текста)
		if (e.flashed && (e.flashDur > 0.0f || e.flashEndTime > 0.0f))
		{
			// Максимальная длительность флешки около 5 секунд
			float flashMax = 5.0f;
			float flashPct = 0.0f;

			float nowGameTime = esp::GetEstimatedGameTimeSeconds();
			if (e.flashEndTime > 0.0f && nowGameTime > 0.0f) {
				float remaining = e.flashEndTime - nowGameTime;
				if (remaining < 0.0f) remaining = 0.0f;
				flashPct = remaining / flashMax;
			} else {
				// Fallback: если не можем оценить game time, используем duration как раньше
				flashPct = e.flashDur / flashMax;
			}
			if (flashPct > 1.0f) flashPct = 1.0f;
			if (flashPct < 0.0f) flashPct = 0.0f;

			// Рисуем бар справа от бокса
			float barW = 3.0f;
			float barH = (box.bottom - box.top) * flashPct;

			float x = box.right + 4.0f;
			float y = box.bottom - barH;

			// Фон бара (темный)
			drawList->AddRectFilled(ImVec2(x, box.top), ImVec2(x + barW, box.bottom), IM_COL32(0, 0, 0, 150));
			// Белый бар (показывает длительность)
			drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + barW, box.bottom), IM_COL32(255, 255, 255, 255));
		}
	}

	// --- ОТРИСОВКА ПРИЦЕЛА СНАЙПЕРКИ (AWP/SCOUT) ---
	if (config::g_scopeSightEnabled && g_isLocalSniperScoped)
	{
		ImVec2 ds = ImGui::GetIO().DisplaySize;
		ImVec2 c(ds.x * 0.5f, ds.y * 0.5f);
		const float len = 10.0f;
		const float gap = 6.0f;
		const float thick = 1.5f;
		const ImU32 col = IM_COL32(255, 255, 255, 220);

		drawList->AddLine(ImVec2(c.x - gap - len, c.y), ImVec2(c.x - gap, c.y), col, thick);
		drawList->AddLine(ImVec2(c.x + gap, c.y), ImVec2(c.x + gap + len, c.y), col, thick);
		drawList->AddLine(ImVec2(c.x, c.y - gap - len), ImVec2(c.x, c.y - gap), col, thick);
		drawList->AddLine(ImVec2(c.x, c.y + gap), ImVec2(c.x, c.y + gap + len), col, thick);
	}

	// --- HITMARKER (animated, configurable style) ---
	if (g_hitmarkerAlpha > 0.0f) {
		ImVec2 ds = ImGui::GetIO().DisplaySize;
		ImVec2 c(ds.x * 0.5f, ds.y * 0.5f);

		ULONGLONG now = GetTickCount64();
		float timeSinceHit = (float)(now - g_hitmarkerTime);

		// Pop-out animation: start small, grow quickly, hold, then fade.
		float popT = timeSinceHit / 80.0f;
		if (popT > 1.0f) popT = 1.0f;
		float scale = 0.6f + 0.4f * popT;

		// Smooth fade
		const float fadeTime = 400.0f;
		if (timeSinceHit > 100.0f) {
			g_hitmarkerAlpha = 1.0f - ((timeSinceHit - 100.0f) / fadeTime);
			if (g_hitmarkerAlpha < 0.0f) g_hitmarkerAlpha = 0.0f;
		}

		float size = config::g_hitmarkerSize * scale;
		float thick = config::g_hitmarkerThickness;
		int a = (int)(g_hitmarkerAlpha * 255.0f);
		if (a < 0) a = 0; if (a > 255) a = 255;
		ImU32 hitCol = IM_COL32(
			(int)(config::g_hitmarkerColor.x * 255.0f),
			(int)(config::g_hitmarkerColor.y * 255.0f),
			(int)(config::g_hitmarkerColor.z * 255.0f),
			(int)(config::g_hitmarkerColor.w * a));
		ImU32 shadowCol = IM_COL32(0, 0, 0, (int)(g_hitmarkerAlpha * 160));

		switch (config::g_hitmarkerStyle) {
			default:
			case 0: { // Cross (X) with split animation
				float gap = 2.0f + (timeSinceHit * 0.02f);
				if (gap > 6.0f) gap = 6.0f;
				// shadow
				drawList->AddLine(ImVec2(c.x - gap - size + 1, c.y - gap - size + 1), ImVec2(c.x - gap + 1, c.y - gap + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + gap + 1, c.y + gap + 1), ImVec2(c.x + gap + size + 1, c.y + gap + size + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + gap + 1, c.y - gap - size + 1), ImVec2(c.x + gap + size + 1, c.y - gap + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x - gap - size + 1, c.y + gap + 1), ImVec2(c.x - gap + 1, c.y + gap + size + 1), shadowCol, thick + 1.0f);
				// marker
				drawList->AddLine(ImVec2(c.x - gap - size, c.y - gap - size), ImVec2(c.x - gap, c.y - gap), hitCol, thick);
				drawList->AddLine(ImVec2(c.x + gap, c.y + gap), ImVec2(c.x + gap + size, c.y + gap + size), hitCol, thick);
				drawList->AddLine(ImVec2(c.x + gap, c.y - gap - size), ImVec2(c.x + gap + size, c.y - gap), hitCol, thick);
				drawList->AddLine(ImVec2(c.x - gap - size, c.y + gap), ImVec2(c.x - gap, c.y + gap + size), hitCol, thick);
				break;
			}
			case 1: { // Plus (+)
				float gap = 2.0f;
				drawList->AddLine(ImVec2(c.x - gap - size + 1, c.y + 1), ImVec2(c.x - gap + 1, c.y + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + gap + 1, c.y + 1), ImVec2(c.x + gap + size + 1, c.y + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + 1, c.y - gap - size + 1), ImVec2(c.x + 1, c.y - gap + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + 1, c.y + gap + 1), ImVec2(c.x + 1, c.y + gap + size + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x - gap - size, c.y), ImVec2(c.x - gap, c.y), hitCol, thick);
				drawList->AddLine(ImVec2(c.x + gap, c.y), ImVec2(c.x + gap + size, c.y), hitCol, thick);
				drawList->AddLine(ImVec2(c.x, c.y - gap - size), ImVec2(c.x, c.y - gap), hitCol, thick);
				drawList->AddLine(ImVec2(c.x, c.y + gap), ImVec2(c.x, c.y + gap + size), hitCol, thick);
				break;
			}
			case 2: { // T-shape (left, right, top only)
				float gap = 2.0f;
				drawList->AddLine(ImVec2(c.x - gap - size + 1, c.y + 1), ImVec2(c.x - gap + 1, c.y + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + gap + 1, c.y + 1), ImVec2(c.x + gap + size + 1, c.y + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x + 1, c.y - gap - size + 1), ImVec2(c.x + 1, c.y - gap + 1), shadowCol, thick + 1.0f);
				drawList->AddLine(ImVec2(c.x - gap - size, c.y), ImVec2(c.x - gap, c.y), hitCol, thick);
				drawList->AddLine(ImVec2(c.x + gap, c.y), ImVec2(c.x + gap + size, c.y), hitCol, thick);
				drawList->AddLine(ImVec2(c.x, c.y - gap - size), ImVec2(c.x, c.y - gap), hitCol, thick);
				break;
			}
			case 3: { // Dot
				float r = thick + 1.5f;
				drawList->AddCircleFilled(ImVec2(c.x + 1, c.y + 1), r, shadowCol, 16);
				drawList->AddCircleFilled(c, r, hitCol, 16);
				break;
			}
			case 4: { // Circle outline
				float r = size;
				drawList->AddCircle(ImVec2(c.x + 1, c.y + 1), r, shadowCol, 24, thick + 1.0f);
				drawList->AddCircle(c, r, hitCol, 24, thick);
				break;
			}
		}
	}

	// --- ОТРИСОВКА FOV (ТЕПЕРЬ ВСЕГДА В КОНЦЕ) ---
	if (config::g_aimbotEnabled && config::g_aimbotDrawFov)
	{
		ImVec2 ds = ImGui::GetIO().DisplaySize;
		ImVec2 center(ds.x * 0.5f, ds.y * 0.5f);
		
		// Конвертируем FOV в пиксели (примерная формула для 16:9)
		// Радиус = tan(FOV/2) * (ScreenHeight/2) / tan(CameraFOV/2)
		// Упрощенно:
		float fovRadius = std::tan(config::g_aimbotFov * M_PI / 180.0f / 2.0f) * (ds.y * 0.5f) / std::tan(90.0f * M_PI / 180.0f / 2.0f);
		
		ImU32 fovColor = ImGui::ColorConvertFloat4ToU32(config::g_aimbotFovColor);
		drawList->AddCircle(center, fovRadius, fovColor, 64, 1.0f);
	}

	// --- ОТРИСОВКА БОМБЫ (C4) ---
	esp::RenderBombEsp(drawList);

	// --- OUT OF FOV ARROWS ---
	esp::RenderOofArrows(drawList);

	// --- FLOATING DAMAGE INDICATORS (STACKED WITH ANIMATION) ---
	esp::RenderDamageIndicators(drawList);
}

void DrawWatermarkImGui()
{
	static int cs2Fps = 0;
	static ULONGLONG lastFpsTime = GetTickCount64();
	static int lastFrameCount = 0;
	
	uintptr_t client = memory::GetClientBase();
	int currentPing = 0;
	char playerName[64] = "Unknown";
	
	if (client) {
		uintptr_t globalVars = 0;
		if (TryRead<uintptr_t>(client + g_offsetsRuntime.dwGlobalVars, globalVars) && globalVars) {
			int frameCount = 0;
			if (TryRead<int>(globalVars + 0x4, frameCount)) { // 0x4 - это m_real_frametime/framecount в CS2
				ULONGLONG now = GetTickCount64();
				if (now - lastFpsTime >= 1000) {
					cs2Fps = frameCount - lastFrameCount;
					lastFrameCount = frameCount;
					lastFpsTime = now;
				}
			}
		}
		
		uintptr_t localCtrl = 0;
		if (TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerController, localCtrl) && localCtrl) {
			TryRead<int>(localCtrl + 0x740, currentPing); // 0x740 - это m_iPing
			
			StringBuf64 nameRaw = {0};
			if (TryRead<StringBuf64>(localCtrl + g_offsetsRuntime.m_iszPlayerName, nameRaw)) {
				memcpy(playerName, nameRaw.data, 64);
				playerName[63] = '\0';
			}
		}
	}
	
	if (cs2Fps <= 0 || cs2Fps > 2000) cs2Fps = (int)ImGui::GetIO().Framerate; // Fallback

	ImGuiWindowFlags wmFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
	if (!config::g_menuOpen) wmFlags |= ImGuiWindowFlags_NoInputs; // Пропускать клики, если меню закрыто!
	
	ImGui::SetNextWindowBgAlpha(0.7f);
	ImGui::Begin("Watermark", nullptr, wmFlags);
	ImGui::TextColored(config::g_uiAccentColor, "OXRANA LOUTABA");
	ImGui::SameLine();
	ImGui::Text("| User: %s | FPS: %d | Ping: %d ms", playerName, cs2Fps, currentPing);
	ImGui::End();
}

// ===================== RADAR HACK =====================
static void DrawRadarImGui()
{
	if (!config::g_radarHackEnabled || g_espTargetsWorld.empty()) return;

	uintptr_t client = memory::GetClientBase();
	if (!client) return;

	esp::QAngle* vp = reinterpret_cast<esp::QAngle*>(client + g_offsetsRuntime.dwViewAngles);
	esp::QAngle va{};
	if (!TryRead<esp::QAngle>((uintptr_t)vp, va)) return;

	uintptr_t localPawn = 0;
	if (!TryRead<uintptr_t>(client + g_offsetsRuntime.dwLocalPlayerPawn, localPawn) || !localPawn) return;
	uintptr_t localScene = 0;
	if (!TryRead<uintptr_t>(localPawn + g_offsetsRuntime.m_pGameSceneNode, localScene) || !localScene) return;
	Vec3 myPos{};
	(void)TryRead<float>(localScene + g_offsetsRuntime.m_vecAbsOrigin + 0x0, myPos.x);
	(void)TryRead<float>(localScene + g_offsetsRuntime.m_vecAbsOrigin + 0x4, myPos.y);
	(void)TryRead<float>(localScene + g_offsetsRuntime.m_vecAbsOrigin + 0x8, myPos.z);

	ImVec2 ds = ImGui::GetIO().DisplaySize;

	// Радар: позиция и размер
	static float radarX = 20.0f, radarY = 20.0f;
	float radarSize = config::g_radarSize;
	float radarScale = config::g_radarScale;
	ImVec2 radarCenter(radarX + radarSize * 0.5f, radarY + radarSize * 0.5f);

	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// Фон
	dl->AddCircleFilled(radarCenter, radarSize * 0.5f, IM_COL32(0, 0, 0, 160), 64);
	dl->AddCircle(radarCenter, radarSize * 0.5f, IM_COL32(80, 80, 80, 255), 64, 1.5f);
	// Кресты
	dl->AddLine(ImVec2(radarCenter.x - radarSize*0.5f, radarCenter.y), ImVec2(radarCenter.x + radarSize*0.5f, radarCenter.y), IM_COL32(60,60,60,150), 0.5f);
	dl->AddLine(ImVec2(radarCenter.x, radarCenter.y - radarSize*0.5f), ImVec2(radarCenter.x, radarCenter.y + radarSize*0.5f), IM_COL32(60,60,60,150), 0.5f);
	// Игрок (белая точка в центре)
	dl->AddCircleFilled(radarCenter, 4.0f, IM_COL32(255, 255, 255, 255));

	float yawRad = va.y * (M_PI / 180.0f);
	float cosYaw = std::cos(-yawRad);
	float sinYaw = std::sin(-yawRad);

	for (const esp::EspTargetWorld& t : g_espTargetsWorld)
	{
		float dx = t.origin.x - myPos.x;
		float dy = t.origin.y - myPos.y;

		// Поворачиваем относительно взгляда
		float rx = dx * cosYaw - dy * sinYaw;
		float ry = dx * sinYaw + dy * cosYaw;

		// Масштабируем
		float px = radarCenter.x + rx / radarScale;
		float py = radarCenter.y - ry / radarScale; // Y инвертирован

		// Ограничиваем по кругу
		float relX = px - radarCenter.x;
		float relY = py - radarCenter.y;
		float dist = std::sqrt(relX * relX + relY * relY);
		float maxR = radarSize * 0.5f - 5.0f;
		if (dist > maxR) { px = radarCenter.x + relX * maxR / dist; py = radarCenter.y + relY * maxR / dist; }

		// Цвет: красный для CT, желтый для T
		ImU32 dotCol = IM_COL32(255, 80, 80, 255);
		dl->AddCircleFilled(ImVec2(px, py), 4.0f, dotCol);
		dl->AddCircle(ImVec2(px, py), 4.5f, IM_COL32(0,0,0,180), 8, 1.0f);
	}

	// Бомба на радаре
	if (config::g_bombEspEnabled && esp::GetBombData().found)
	{
		auto& bombData = esp::GetBombData();
		float bdx = bombData.pos.x - myPos.x;
		float bdy = bombData.pos.y - myPos.y;
		float brx = bdx * cosYaw - bdy * sinYaw;
		float bry = bdx * sinYaw + bdy * cosYaw;
		float bpx = radarCenter.x + brx / radarScale;
		float bpy = radarCenter.y - bry / radarScale;
		float relBX = bpx - radarCenter.x, relBY = bpy - radarCenter.y;
		float bd = std::sqrt(relBX*relBX + relBY*relBY);
		float maxR2 = radarSize * 0.5f - 5.0f;
		if (bd > maxR2) { bpx = radarCenter.x + relBX*maxR2/bd; bpy = radarCenter.y + relBY*maxR2/bd; }
		dl->AddRectFilled(ImVec2(bpx-4, bpy-4), ImVec2(bpx+4, bpy+4), IM_COL32(255, 220, 0, 255));
	}
}

static bool PremiumToggle(const char* label, bool* v)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	
	float height = 20.0f;
	float width = 38.0f;
	float radius = height * 0.5f;

	ImGui::InvisibleButton(label, ImVec2(width, height));
	bool clicked = ImGui::IsItemClicked();
	if (clicked) *v = !*v;
	bool hovered = ImGui::IsItemHovered();

	// Анимация 
	ImGuiID id = ImGui::GetID(label);
	float* anim_ptr = ImGui::GetStateStorage()->GetFloatRef(id, *v ? 1.0f : 0.0f);
	float& anim = *anim_ptr;
	
	// Скорость анимации (12.0f = плавно, но быстро)
	anim += (*v ? 1.0f : -1.0f) * ImGui::GetIO().DeltaTime * 12.0f;
	if (anim < 0.0f) anim = 0.0f;
	if (anim > 1.0f) anim = 1.0f;

	ImVec4 col_off = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
	ImVec4 col_on = config::g_uiAccentColor;
	
	// Интерполяция цвета от серого к цвету темы
	ImVec4 current_col = ImVec4(
		col_off.x + (col_on.x - col_off.x) * anim,
		col_off.y + (col_on.y - col_off.y) * anim,
		col_off.z + (col_on.z - col_off.z) * anim,
		1.0f
	);
	
	if (hovered) {
		current_col.x = (current_col.x + 0.1f > 1.0f) ? 1.0f : current_col.x + 0.1f;
		current_col.y = (current_col.y + 0.1f > 1.0f) ? 1.0f : current_col.y + 0.1f;
		current_col.z = (current_col.z + 0.1f > 1.0f) ? 1.0f : current_col.z + 0.1f;
	}

	// Фон свитча
	draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::ColorConvertFloat4ToU32(current_col), radius);
	
	// Плавный сдвиг кружка
	float circle_x = p.x + radius + (width - radius * 2.0f) * anim;
	draw_list->AddCircleFilled(ImVec2(circle_x, p.y + radius), radius - 2.0f, IM_COL32(255, 255, 255, 255));
	
	// Мягкое свечение кружка
	if (anim > 0.0f) {
		draw_list->AddCircleFilled(ImVec2(circle_x, p.y + radius), radius + 2.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(current_col.x, current_col.y, current_col.z, 0.4f * anim)));
	}

	// Плавное изменение цвета текста
	ImVec4 text_col = ImVec4(
		0.5f + (current_col.x - 0.5f) * anim,
		0.5f + (current_col.y - 0.5f) * anim,
		0.5f + (current_col.z - 0.5f) * anim,
		1.0f
	);
	
	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - ImGui::GetTextLineHeight()) * 0.5f);
	
	ImGui::PushStyleColor(ImGuiCol_Text, text_col);
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();
	
	return clicked;
}

// Premium Panel Functions - красивые карточки с заголовком
static void BeginPremiumChild(const char* str_id, const char* title, ImVec2 size) 
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.09f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
	
	ImGui::BeginChild(str_id, size, true, ImGuiWindowFlags_NoScrollbar);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 w_pos = ImGui::GetWindowPos();
	float w_width = ImGui::GetWindowWidth();
	
	// Отрисовка красивого заголовка панели
	ImU32 headBgL = ImGui::ColorConvertFloat4ToU32(ImVec4(config::g_uiAccentColor.x * 0.2f, config::g_uiAccentColor.y * 0.2f, config::g_uiAccentColor.z * 0.3f, 0.7f));
	ImU32 headBgR = ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.08f, 0.09f, 0.0f));
	
	// Градиент в шапке
	draw_list->AddRectFilledMultiColor(w_pos, ImVec2(w_pos.x + w_width, w_pos.y + 35.0f), headBgL, headBgR, headBgR, headBgL);
	
	// Акцентная линия снизу заголовка
	draw_list->AddLine(ImVec2(w_pos.x, w_pos.y + 35.0f), ImVec2(w_pos.x + w_width, w_pos.y + 35.0f), ImGui::ColorConvertFloat4ToU32(ImVec4(config::g_uiAccentColor.x, config::g_uiAccentColor.y, config::g_uiAccentColor.z, 0.5f)), 1.0f);

	// Текст заголовка
	ImGui::SetCursorPos(ImVec2(14, 10));
	ImGui::TextColored(config::g_uiAccentColor, title);
	
	// Сдвигаем курсор для контента (чтобы не рисовалось на шапке)
	ImGui::SetCursorPos(ImVec2(14, 45)); 
}

static void EndPremiumChild() 
{
	ImGui::EndChild();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}

// Векторная шестеренка (Gear) без файлов - всегда работает
static bool PremiumSettingsButton(const char* id)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	bool clicked = ImGui::InvisibleButton(id, ImVec2(24, 24));
	bool hovered = ImGui::IsItemHovered();
	
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImU32 col = ImGui::GetColorU32(hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
	
	ImVec2 c(pos.x + 12, pos.y + 12);
	
	// Отрисовка идеальной шестеренки (Gear) математикой
	dl->AddCircle(c, 4.5f, col, 12, 2.0f); // Основное кольцо
	for (int i = 0; i < 6; i++) {
		float angle = i * (3.1415926f / 3.0f);
		dl->AddLine(
			ImVec2(c.x + cosf(angle) * 4.5f, c.y + sinf(angle) * 4.5f),
			ImVec2(c.x + cosf(angle) * 7.0f, c.y + sinf(angle) * 7.0f), 
			col, 2.5f
		); // Зубья
	}
	
	// Центральное отверстие (цвет фона)
	dl->AddCircleFilled(c, 2.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(0.06f, 0.06f, 0.07f, 1.0f))); 
	
	return clicked;
}

void DrawMenuImGui()
{
	if (!config::g_menuOpen) return;

	// Auto-save: flushed 800ms after last change.
	static bool  s_cfgDirty = false;
	static ULONGLONG s_cfgDirtyTs = 0;
	auto MarkDirty = [&]() { s_cfgDirty = true; s_cfgDirtyTs = GetTickCount64(); };
	if (s_cfgDirty && (GetTickCount64() - s_cfgDirtyTs > 800)) { config::SaveConfig(); s_cfgDirty = false; }

	// ---- Cyber theme infrastructure ----
	ui::theme::RebuildFromUserAccent(config::g_uiAccentColor, config::g_uiAccentSecondary);
	const ui::theme::Palette& TP = ui::theme::Colors();

	// Open-animation (fade + micro-scale via alpha gate).
	static bool s_wasOpen = false;
	if (!s_wasOpen) ui::theme::ResetOpenAnimation();
	s_wasOpen = true;
	{
		float& oa = ui::theme::OpenAnimationProgress();
		ui::theme::AnimatedValue v{ oa, 1.0f, 9.0f };
		v.Tick(ui::theme::SafeDeltaTime());
		oa = v.current;
	}

	ImGui::SetNextWindowBgAlpha(0.0f); // we paint our own glass background
	ImGui::SetNextWindowSize(ImVec2(ui::theme::kWindowW, ui::theme::kWindowH), ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ui::theme::OpenAnimationProgress());
	ImGui::Begin("##oxrana_menu", &config::g_menuOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 p = ImGui::GetWindowPos();
	ImVec2 s_win = ImGui::GetWindowSize();
	ImVec2 winMax = ImVec2(p.x + s_win.x, p.y + s_win.y);

	// ---- Glass fill + gradient border ----
	ui::CyberDrawGlassFill(p, winMax, 8.0f);
	ui::CyberDrawGradientBorder(p, winMax, 8.0f);

	// Optional particles (off by default now). Kept behind a config flag.
	if (config::g_uiParticlesEnabled)
	{
		struct Particle { ImVec2 pos; ImVec2 vel; };
		static std::vector<Particle> particles;
		static bool initParticles = false;
		if (!initParticles) {
			for (int i = 0; i < 40; i++) {
				particles.push_back({
					ImVec2(p.x + (float)(rand() % (int)s_win.x), p.y + (float)(rand() % (int)s_win.y)),
					ImVec2((float)(rand() % 100 - 50) / 100.0f, (float)(rand() % 100 - 50) / 100.0f)
				});
			}
			initParticles = true;
		}
		ImU32 particleColor = (TP.Accent & 0x00FFFFFFu) | ((ImU32)70 << 24);
		ImU32 lineColor     = (TP.Accent & 0x00FFFFFFu) | ((ImU32)40 << 24);
		for (auto& part : particles) {
			part.pos.x += part.vel.x * ImGui::GetIO().DeltaTime * 60.0f;
			part.pos.y += part.vel.y * ImGui::GetIO().DeltaTime * 60.0f;
			if (part.pos.x < p.x || part.pos.x > p.x + s_win.x) part.vel.x *= -1.0f;
			if (part.pos.y < p.y || part.pos.y > p.y + s_win.y) part.vel.y *= -1.0f;
			drawList->AddCircleFilled(part.pos, 1.5f, particleColor);
			for (auto& other : particles) {
				float dist = std::hypot(part.pos.x - other.pos.x, part.pos.y - other.pos.y);
				if (dist < 60.0f && dist > 0.0f)
					drawList->AddLine(part.pos, other.pos, lineColor, 1.0f);
			}
		}
	}

	// ---- Header (44px) ----
	const float headerH = ui::theme::kHeaderH;
	ImVec2 headerMax = ImVec2(winMax.x, p.y + headerH);

	// Subtle gradient fill: panel -> slightly brighter at the top
	ImU32 hdrTop    = IM_COL32(22, 26, 40, 200);
	ImU32 hdrBottom = IM_COL32(14, 16, 24, 120);
	drawList->AddRectFilledMultiColor(p, headerMax, hdrTop, hdrTop, hdrBottom, hdrBottom);

	// Accent strip (gradient across header bottom edge)
	ImU32 stripA = TP.Accent;
	ImU32 stripB = TP.AccentSecondary;
	drawList->AddRectFilledMultiColor(
		ImVec2(p.x, headerMax.y - 2.0f),
		ImVec2(winMax.x, headerMax.y),
		stripA, stripB, stripB, stripA);

	ImGui::SetCursorPos(ImVec2(18.0f, 12.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, TP.TextPrimaryV);
	ImGui::SetWindowFontScale(1.15f);
	ImGui::TextUnformatted("Oxrana Loutaba Client");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	// Top-right: FPS + version
	{
		ImGuiIO& io = ImGui::GetIO();
		char hdr[64];
		sprintf_s(hdr, sizeof(hdr), "FPS %.0f \xE2\x80\xA2 v2.0", io.Framerate);
		ImVec2 ts = ImGui::CalcTextSize(hdr);
		ImGui::SetCursorPos(ImVec2(s_win.x - ts.x - 18.0f, 14.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, TP.TextMutedV);
		ImGui::TextUnformatted(hdr);
		ImGui::PopStyleColor();
	}

	// ---- Tabs enum ----
	enum Tab : int { TAB_COMBAT = 0, TAB_ESP = 1, TAB_PLAYER = 2, TAB_MOVEMENT = 3, TAB_SKINS = 4, TAB_CONFIGS = 5, TAB_OTHER = 6 };
	static int activeTab = TAB_ESP;
	static int prevTab = -1;
	static float tabFadeAlpha = 1.0f;

	// Tab fade-in (220ms)
	if (prevTab != activeTab) {
		tabFadeAlpha = 0.0f;
		prevTab = activeTab;
	}
	{
		ui::theme::AnimatedValue v{ tabFadeAlpha, 1.0f, 8.0f };
		v.Tick(ui::theme::SafeDeltaTime());
		tabFadeAlpha = v.current;
	}

	// ---- Sidebar (170px) ----
	ImGui::SetCursorPos(ImVec2(0.0f, headerH));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, TP.BgPanelV);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 12));
	ImGui::BeginChild("##sidebar", ImVec2(ui::theme::kSidebarW, s_win.y - headerH - ui::theme::kFooterH), false);

	struct CategoryDef { int id; const char* icon; const char* label; const char* sub; };
	static const CategoryDef kCats[] = {
		{ TAB_COMBAT,   "[>]", "COMBAT",   "aim / trigger" },
		{ TAB_ESP,      "[#]", "VISUALS",  "esp / colors"  },
		{ TAB_PLAYER,   "[P]", "PLAYER",   "flash / fov"   },
		{ TAB_MOVEMENT, "[M]", "MOVEMENT", "bhop / strafe" },
		{ TAB_SKINS,    "[S]", "SKINS",    "paint kits"    },
		{ TAB_CONFIGS,  "[C]", "CONFIGS",  "profiles"      },
		{ TAB_OTHER,    "[*]", "MISC",     "ui / theme"    },
	};
	for (const CategoryDef& cat : kCats) {
		char idBuf[32];
		sprintf_s(idBuf, sizeof(idBuf), "##sidecat_%d", cat.id);
		if (ui::CyberSidebarItem(idBuf, cat.icon, cat.label, cat.sub, activeTab == cat.id))
			activeTab = cat.id;
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();

	// Sidebar divider (vertical accent line separating sidebar from content)
	{
		float sx = p.x + ui::theme::kSidebarW;
		drawList->AddLine(ImVec2(sx, p.y + headerH + 4.0f),
		                  ImVec2(sx, winMax.y - ui::theme::kFooterH - 4.0f),
		                  (TP.Accent & 0x00FFFFFFu) | ((ImU32)40 << 24), 1.0f);
	}

	// ---- Content area ----
	ImGui::SetCursorPos(ImVec2(ui::theme::kSidebarW, headerH));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ui::theme::kContentPadding, ui::theme::kContentPadding));
	ImGui::BeginChild("##content",
	                  ImVec2(s_win.x - ui::theme::kSidebarW, s_win.y - headerH - ui::theme::kFooterH),
	                  false,
	                  ImGuiWindowFlags_None);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tabFadeAlpha);

	if (activeTab == TAB_COMBAT)
	{
		// === AIMBOT ===
		ui::CyberSectionHeader("AIMBOT");
		PremiumToggle("Enable Aimbot", &config::g_aimbotEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		KeybindWidget("##aimbot_bind", config::g_aimbotKey);
		ImGui::SameLine();
		if (PremiumSettingsButton("##aimbot_gear_btn")) ImGui::OpenPopup("##aimbot_ctx");

		if (ImGui::BeginPopup("##aimbot_ctx"))
		{
			ImGui::TextUnformatted("Настройки Аимбота");
			ImGui::Separator();
			PremiumToggle("Проверка команды", &config::g_aimbotTeamCheck);
			if (ImGui::IsItemClicked()) MarkDirty();
			PremiumToggle("Проверка видимости", &config::g_aimbotVisCheck);
			if (ImGui::IsItemClicked()) MarkDirty();
			PremiumToggle("Автовыстрел", &config::g_aimbotAutoFire);
			if (ImGui::IsItemClicked()) MarkDirty();
			
			ImGui::SliderFloat("FOV", &config::g_aimbotFov, 1.0f, 200.0f, "%.1f");
			if (ImGui::IsItemDeactivatedAfterEdit()) { MarkDirty(); }
			
			ImGui::SliderFloat("Smooth", &config::g_aimbotSmooth, 1.0f, 10.0f, "%.1f");
			if (ImGui::IsItemDeactivatedAfterEdit()) { MarkDirty(); }
			
			const char* boneNames[] = { "Голова", "Шея", "Грудь", "Живот" };
			const int boneIds[] = { 6, 5, 4, 2 };
			int currentBoneIdx = 0;
			for (int i = 0; i < 4; i++) {
				if (config::g_aimbotBone == boneIds[i]) {
					currentBoneIdx = i;
					break;
				}
			}
			if (ImGui::Combo("Цель", &currentBoneIdx, boneNames, 4)) {
				config::g_aimbotBone = boneIds[currentBoneIdx];
				MarkDirty();
			}
			PremiumToggle("Показать FOV", &config::g_aimbotDrawFov);
			ImGui::ColorEdit4("Цвет FOV", (float*)&config::g_aimbotFovColor, ImGuiColorEditFlags_NoInputs);
			ImGui::EndPopup();
		}

		ImGui::Spacing();

		// === TRIGGERBOT ===
		ui::CyberSectionHeader("TRIGGERBOT");
		PremiumToggle("Enable Triggerbot", &config::g_triggerEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		KeybindWidget("##trigger_bind", config::g_triggerKey);
		ImGui::SameLine();
		if (PremiumSettingsButton("##trigger_gear_btn")) ImGui::OpenPopup("##trigger_ctx");

		if (ImGui::BeginPopup("##trigger_ctx"))
		{
			ImGui::TextUnformatted("Настройки Триггербота");
			ImGui::Separator();
			PremiumToggle("Только в голову", &config::g_triggerHeadOnly);
			if (ImGui::IsItemClicked()) MarkDirty();
			PremiumToggle("Проверка команды", &config::g_triggerTeamCheck);
			if (ImGui::IsItemClicked()) MarkDirty();
			PremiumToggle("Только с прицелом", &config::g_triggerScopeOnly);
			if (ImGui::IsItemClicked()) MarkDirty();
			PremiumToggle("Проверка видимости", &config::g_triggerVisCheck);
			if (ImGui::IsItemClicked()) MarkDirty();
			ImGui::EndPopup();
		}

		ImGui::Spacing();

		// === ACCURACY ===
		ui::CyberSectionHeader("ACCURACY");
		PremiumToggle("No Recoil", &config::g_noRecoilEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(полное обнуление)");

		PremiumToggle("No Spread", &config::g_noSpreadEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();

		PremiumToggle("RCS (контроль отдачи)", &config::g_rcsEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		if (config::g_rcsEnabled) {
			ImGui::SliderInt("Сила RCS", &config::g_rcsStrength, 0, 100, "%d%%");
			if (ImGui::IsItemDeactivatedAfterEdit()) MarkDirty();
		}

		ImGui::Spacing();

		// === AUTOMATION ===
		ui::CyberSectionHeader("AUTOMATION");
		PremiumToggle("Auto-Stop", &config::g_autoStopEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(тормозит перед выстрелом)");

		PremiumToggle("Auto-Knife", &config::g_autoKnifeEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(бьёт ножом автоматически)");

		ImGui::Spacing();
	}
	else if (activeTab == TAB_ESP)
	{
		// Начинаем таблицу на 2 колонки
		if (ImGui::BeginTable("##esp_columns", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings))
		{
			ImGui::TableNextRow();
			
			// --- КОЛОНКА 1 ---
			ImGui::TableSetColumnIndex(0);
			
			PremiumToggle("Boxes", &config::g_whEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			ImGui::SameLine();
			if (PremiumSettingsButton("##box_gear_btn")) ImGui::OpenPopup("##box_ctx");

			if (ImGui::BeginPopup("##box_ctx")) {
				ImGui::TextUnformatted("Настройки боксов");
				ImGui::Separator();
				ImGui::ColorEdit4("Цвет", (float*)&config::g_boxColor, ImGuiColorEditFlags_NoInputs);
				PremiumToggle("Dynamic HP Color", &g_dynamicBoxColor);
				if (ImGui::IsItemClicked()) MarkDirty();
				ImGui::SliderFloat("Отступы", &g_boxPadding, 0.0f, 10.0f, "%.1f");
				ImGui::SliderFloat("Толщина", &g_boxThickness, 0.1f, 5.0f, "%.1f");
				ImGui::EndPopup();
			}

			ImGui::Separator();
			
			PremiumToggle("HP Bar", &config::g_hpBarEnabled);
			ImGui::SameLine();
			if (PremiumSettingsButton("##hp_gear_btn")) ImGui::OpenPopup("##hp_ctx");

			if (ImGui::BeginPopup("##hp_ctx")) {
				ImGui::TextUnformatted("Настройки HP Bar");
				ImGui::Separator();
				ImGui::ColorEdit4("Цвет", (float*)&config::g_hpBarColor, ImGuiColorEditFlags_NoInputs);
				ImGui::SliderFloat("Ширина", &config::g_hpBarWidth, 1.0f, 10.0f, "%.1f");
				ImGui::SliderFloat("Отступ", &config::g_hpBarOffset, 0.0f, 20.0f, "%.1f");
				ImGui::EndPopup();
			}

			ImGui::Separator();
			
			PremiumToggle("Skeletons", &config::g_bonesEnabled);
			ImGui::SameLine();
			if (PremiumSettingsButton("##bone_gear_btn")) ImGui::OpenPopup("##bone_ctx");

			if (ImGui::BeginPopup("##bone_ctx")) {
				ImGui::TextUnformatted("Настройки скелета");
				ImGui::Separator();
				ImGui::ColorEdit4("Цвет", (float*)&config::g_boneColor, ImGuiColorEditFlags_NoInputs);
				ImGui::EndPopup();
			}

			ImGui::Separator();

			// --- КОЛОНКА 2 ---
			ImGui::TableSetColumnIndex(1);
			
			PremiumToggle("Names", &config::g_nameEspEnabled);
			ImGui::SameLine();
			if (PremiumSettingsButton("##name_gear_btn")) ImGui::OpenPopup("##name_ctx");

			if (ImGui::BeginPopup("##name_ctx")) {
				ImGui::TextUnformatted("Настройки имён");
				ImGui::Separator();
				ImGui::ColorEdit4("Цвет", (float*)&config::g_nameColor, ImGuiColorEditFlags_NoInputs);
				ImGui::SliderFloat("Смещение Y", &config::g_nameOffsetY, 0.0f, 20.0f, "%.1f");
				ImGui::EndPopup();
			}

			ImGui::Separator();
			
			PremiumToggle("Weapons", &config::g_gunEspEnabled);
			ImGui::SameLine();
			if (PremiumSettingsButton("##weapon_gear_btn")) ImGui::OpenPopup("##weapon_ctx");

			if (ImGui::BeginPopup("##weapon_ctx")) {
				ImGui::TextUnformatted("Настройки оружия (текст)");
				ImGui::Separator();
				ImGui::ColorEdit4("Цвет текста", (float*)&config::g_weaponIconColor, ImGuiColorEditFlags_NoInputs);
				ImGui::SliderFloat("Смещение X", &config::g_gunOffsetX, -30.0f, 30.0f, "%.1f");
				ImGui::SliderFloat("Смещение Y", &config::g_gunOffsetY, 0.0f, 30.0f, "%.1f");
				ImGui::EndPopup();
			}

			ImGui::Separator();
			
			PremiumToggle("Chams (Glow)", &config::g_chamsEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			ImGui::SameLine();
			if (PremiumSettingsButton("##chams_gear_btn")) ImGui::OpenPopup("##chams_ctx");

			if (ImGui::BeginPopup("##chams_ctx")) {
				ImGui::TextUnformatted("Настройки Chams");
				ImGui::Separator();
				ImGui::ColorEdit4("Цвет T", (float*)&config::g_chamsColorT, ImGuiColorEditFlags_NoInputs);
				ImGui::ColorEdit4("Цвет CT", (float*)&config::g_chamsColorCT, ImGuiColorEditFlags_NoInputs);
				ImGui::EndPopup();
			}

			// Продолжаем ПЕРВУЮ КОЛОНКУ (добавляем туда элементы)
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			
			PremiumToggle("Snaplines", &config::g_snaplinesEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			
			ImGui::Separator();
			PremiumToggle("Distance", &config::g_distanceEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();

			ImGui::Separator();
			PremiumToggle("Scope Sight", &config::g_scopeSightEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			ImGui::SameLine();
			ImGui::TextDisabled("(прицел AWP / SSG при scope)");

			ImGui::Separator();
			PremiumToggle("Damage Indicators", &config::g_damageIndicatorsEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			ImGui::SameLine();
			if (PremiumSettingsButton("##dmg_gear_btn")) ImGui::OpenPopup("##dmg_ctx");
			if (ImGui::BeginPopup("##dmg_ctx")) {
				ImGui::ColorEdit4("Цвет урона", (float*)&config::g_damageColor, ImGuiColorEditFlags_NoInputs);
				ImGui::SliderFloat("Размер текста", &config::g_damageTextSize, 10.0f, 40.0f, "%.1f");
				ImGui::SliderFloat("Время (сек)", &config::g_damageTextLifetime, 1.0f, 10.0f, "%.1f");
				ImGui::EndPopup();
			}

			// --- ВТОРАЯ КОЛОНКА ---
			ImGui::TableSetColumnIndex(1);
			
			PremiumToggle("Radar Hack (Native)", &config::g_radarHackEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			
			ImGui::Separator();
			PremiumToggle("Out of FOV Arrows", &config::g_oofArrowsEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();
			ImGui::SameLine();
			if (PremiumSettingsButton("##oof_gear_btn")) ImGui::OpenPopup("##oof_ctx");
			if (ImGui::BeginPopup("##oof_ctx")) {
				ImGui::ColorEdit4("Цвет стрелок", (float*)&config::g_oofArrowsColor, ImGuiColorEditFlags_NoInputs);
				ImGui::SliderFloat("Радиус", &config::g_oofArrowsRadius, 50.0f, 500.0f, "%.0f");
				ImGui::SliderFloat("Размер", &config::g_oofArrowsSize, 10.0f, 30.0f, "%.1f");
				ImGui::EndPopup();
			}

			ImGui::Separator();
			PremiumToggle("Bomb ESP (Timer)", &config::g_bombEspEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();

			ImGui::Separator();
			PremiumToggle("Spectator List", &config::g_spectatorListEnabled);
			if (ImGui::IsItemClicked()) MarkDirty();

			ImGui::EndTable(); // ТЕПЕРЬ ТАБЛИЦА ЗАКРЫВАЕТСЯ ЗДЕСЬ
		}

		// Эти можно оставить под таблицей
		ImGui::Spacing();
		PremiumToggle("Keybinds List", &config::g_keybindsListEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		
		ImGui::Separator();
		ImGui::Spacing();
	}
	else if (activeTab == TAB_PLAYER)
	{
		PremiumToggle("Anti-Flash", &config::g_antiFlashEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		
		PremiumToggle("No Smoke", &config::g_noSmokeEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(убирает смок-гранаты)");
		
		PremiumToggle("Hit Sound", &g_hitSoundEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(звук при попадании)");
		if (g_hitSoundEnabled) {
			if (ImGui::SliderInt("Громкость##hitsnd", &config::g_hitSoundVolume, 0, 100, "%d%%"))
				MarkDirty();
		}

		PremiumToggle("Hitmarker", &g_hitmarkerEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		if (PremiumSettingsButton("##hitmarker_gear_btn")) ImGui::OpenPopup("##hitmarker_ctx");
		if (ImGui::BeginPopup("##hitmarker_ctx")) {
			static const char* hitmarkerStyles[] = { "Cross (X)", "Plus (+)", "T-shape", "Dot", "Circle" };
			int style = config::g_hitmarkerStyle;
			if (style < 0) style = 0;
			if (style > 4) style = 4;
			if (ImGui::Combo("Style", &style, hitmarkerStyles, IM_ARRAYSIZE(hitmarkerStyles))) {
				config::g_hitmarkerStyle = style;
				MarkDirty();
			}
			if (ImGui::ColorEdit4("Color", (float*)&config::g_hitmarkerColor, ImGuiColorEditFlags_NoInputs))
				MarkDirty();
			if (ImGui::SliderFloat("Size", &config::g_hitmarkerSize, 2.0f, 20.0f, "%.1f"))
				MarkDirty();
			if (ImGui::SliderFloat("Thickness", &config::g_hitmarkerThickness, 1.0f, 5.0f, "%.1f"))
				MarkDirty();
			if (ImGui::Button("Preview")) {
				g_hitmarkerAlpha = 1.0f;
				g_hitmarkerTime = GetTickCount64();
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(визуальный крестик при попадании)");
		
		ImGui::Separator();
		
		if (PremiumToggle("FOV Override", &config::g_fovEnabled))
		{
			MarkDirty();
		}
		ImGui::SliderFloat("FOV Value", &config::g_fovValue, 60.0f, 140.0f, "%.1f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			MarkDirty();
		}
		
		ImGui::Separator();
		
		// Stream Proof / Anti-Capture
		if (PremiumToggle("Anti-Capture (OBS/Discord)", &config::g_antiCaptureEnabled))
		{
			MarkDirty();
			// Применяем сразу
			if (g_hOverlayWnd) {
				SetWindowDisplayAffinity(g_hOverlayWnd, config::g_antiCaptureEnabled ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
			}
		}
		ImGui::TextDisabled("Скрывает меню и ESP от захвата экрана");
	}
	else if (activeTab == TAB_MOVEMENT)
	{
		ui::CyberSectionHeader("BUNNY HOP");
		PremiumToggle("Bhop", &config::g_bhopEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		KeybindWidget("##bhop_bind", config::g_bhopKey);
		ImGui::TextDisabled("Зажми клавишу для автопрыжка (A/D крути сам)");

		PremiumToggle("Auto Strafe", &config::g_autoStrafeEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();

		ImGui::Spacing();
		ui::CyberSectionHeader("JUMP TRICKS");

		PremiumToggle("Jump-Duck", &config::g_jumpDuckEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		KeybindWidget("##jumpduck_bind", config::g_jumpDuckKey);
		ImGui::SameLine();
		ImGui::TextDisabled("(jump + duck по 1 кнопке)");

		PremiumToggle("Jump Scout", &config::g_jumpScoutEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(автовыстрел при приземлении SSG/AWP)");
	}
	else if (activeTab == TAB_SKINS)
	{
		if (!config::g_skinWarningShown) {
			ImGui::OpenPopup("##skin_warning");
		}
		
		ImGui::SetNextWindowSize(ImVec2(650.0f, 420.0f));
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("##skin_warning", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
			ImDrawList* pdl = ImGui::GetWindowDrawList();
			ImVec2 wp = ImGui::GetWindowPos();
			ImVec2 ws = ImGui::GetWindowSize();
			
			// Та же градиентная линия сверху как у основного меню
			ImU32 col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(config::g_uiAccentColor.x, config::g_uiAccentColor.y, config::g_uiAccentColor.z, 1.0f));
			ImU32 col2 = ImGui::ColorConvertFloat4ToU32(ImVec4(config::g_uiAccentColor.x * 0.5f, config::g_uiAccentColor.y * 0.3f, 1.0f, 1.0f));
			pdl->AddRectFilledMultiColor(wp, ImVec2(wp.x + ws.x, wp.y + 4.0f), col1, col2, col2, col1);
			
			ImGui::SetCursorPos(ImVec2(18, 20));
			ImGui::TextColored(config::g_uiAccentColor, "OXRANALOUTABA CLIENT — Skin Changer");
			ImGui::SetCursorPosY(45);
			ImGui::Separator();
			
			ImGui::SetCursorPos(ImVec2(18, 70));
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Предупреждение");
			ImGui::Spacing();
			ImGui::SetCursorPosX(18);
			ImGui::TextWrapped("Скины применяются через FallbackPaintKit — это клиентский метод.");
			ImGui::Spacing();
			ImGui::SetCursorPosX(18);
			ImGui::TextWrapped("UV-развёртка на некоторых скинах может отображаться некорректно. Это ожидаемое поведение данного метода, а не баг.");
			ImGui::Spacing();
			ImGui::SetCursorPosX(18);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Это сообщение показывается только один раз.");
			
			ImGui::SetCursorPos(ImVec2(ws.x * 0.5f - 60.0f, ws.y - 55.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, config::g_uiAccentColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(config::g_uiAccentColor.x + 0.1f, config::g_uiAccentColor.y + 0.1f, config::g_uiAccentColor.z + 0.1f, 1.0f));
			if (ImGui::Button("Хорошо", ImVec2(120.0f, 36.0f))) {
				config::g_skinWarningShown = true;
				config::SaveConfig();
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor(2);
			
			ImGui::EndPopup();
		}
		
		PremiumToggle("Enable Skin Changer", &g_skinChangerEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		
		ImGui::Separator();
		ImGui::Text("Скины (Обновляются моментально!):");
		ImGui::Spacing();

		// === TEC-9 ===
		const char* tec9Skins[] = { "Default", "Decimator", "Fuel Injector", "Remote Control", "Isaac", "Toxic", "Avalanche", "Re-Entry", "Brother" };
		const int tec9PaintKits[] = { 0, 644, 614, 791, 303, 374, 520, 539, 1099 };
		static int tec9SkinIdx = 0;
		if (ImGui::Combo("Tec-9", &tec9SkinIdx, tec9Skins, IM_ARRAYSIZE(tec9Skins))) { g_skinConfig[30] = MockSkinConfig{tec9PaintKits[tec9SkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === USP-S ===
		const char* uspSkins[] = { "Default", "Printstream", "The Traitor", "Neo-Noir", "Kill Confirmed", "Jawbreaker", "Monster Mashup", "Caiman", "Serum", "Orion", "Whiteout", "Target Acquired", "Ticket to Hell", "Cortex" };
		const int uspPaintKits[] = { 0, 1142, 1040, 653, 504, 1173, 991, 339, 221, 313, 1065, 1027, 1146, 705 };
		static int uspSkinIdx = 0;
		if (ImGui::Combo("USP-S", &uspSkinIdx, uspSkins, IM_ARRAYSIZE(uspSkins))) { g_skinConfig[61] = MockSkinConfig{uspPaintKits[uspSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === GLOCK-18 ===
		const char* glockSkins[] = { "Default", "Twilight Galaxy", "Vogue", "Water Elemental", "Snack Attack", "Gamma Doppler Emerald", "Wasteland Rebel", "Bullet Queen", "Neo-Noir", "Fade", "Moonrise", "Nuclear Garden" };
		const int glockPaintKits[] = { 0, 437, 963, 353, 1100, 1119, 586, 957, 988, 38, 707, 536 };
		static int glockSkinIdx = 0;
		if (ImGui::Combo("Glock-18", &glockSkinIdx, glockSkins, IM_ARRAYSIZE(glockSkins))) { g_skinConfig[4] = MockSkinConfig{glockPaintKits[glockSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === AK-47 ===
		const char* akSkins[] = { "Default", "Nightwish", "Leet Museo", "Legion of Anubis", "Asiimov", "Neon Rider", "The Empress", "Bloodsport", "Neon Revolution", "Fuel Injector", "Aquamarine Revenge", "Wasteland Rebel", "Jaguar", "Vulcan", "Fire Serpent", "Gold Arabesque", "X-Ray", "Wild Lotus", "Ice Coaled", "Phantom Disruptor", "Point Disarray", "Frontside Misty", "Cartel", "Redline", "Case Hardened", "Red Laminate", "Panthera onca", "Hydroponic", "Jet Set" };
		const int akPaintKits[] = { 0, 1141, 1087, 959, 551, 433, 675, 597, 600, 524, 474, 380, 316, 302, 180, 1026, 1004, 724, 1143, 941, 506, 490, 528, 282, 44, 14, 1018, 456, 340 };
		static int akSkinIdx = 0;
		if (ImGui::Combo("AK-47", &akSkinIdx, akSkins, IM_ARRAYSIZE(akSkins))) { g_skinConfig[7] = MockSkinConfig{akPaintKits[akSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === AWP ===
		const char* awpSkins[] = { "Default", "Printstream", "Chromatic Aberration", "Containment Breach", "Wildfire", "Neo-Noir", "Oni Taiji", "Hyper Beast", "Man-o'-war", "Asiimov", "Lightning Strike", "Desert Hydra", "Fade", "The Prince", "Gungnir", "Medusa", "Dragon Lore", "Ice Coaled", "Mortis", "Fever Dream", "Elite Build", "Corticera", "Redline", "Electric Hive", "Graphite", "BOOM", "Silk Tiger" };
		const int awpPaintKits[] = { 0, 1144, 1120, 887, 917, 803, 662, 475, 395, 279, 51, 1058, 1022, 736, 756, 446, 344, 1143, 691, 640, 525, 181, 259, 227, 212, 174, 1029 };
		static int awpSkinIdx = 0;
		if (ImGui::Combo("AWP", &awpSkinIdx, awpSkins, IM_ARRAYSIZE(awpSkins))) { g_skinConfig[9] = MockSkinConfig{awpPaintKits[awpSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === FAMAS ===
		const char* famasSkins[] = { "Default", "Commemoration", "Roll Cage", "Rapid Eye Movement", "Eye of Athena", "Mecha Industries", "Djinn", "Afterimage", "Waters of Nephthys", "Meltdown", "Valence" };
		const int famasPaintKits[] = { 0, 919, 604, 1127, 723, 587, 429, 154, 1128, 1053, 529 };
		static int famasSkinIdx = 0;
		if (ImGui::Combo("FAMAS", &famasSkinIdx, famasSkins, IM_ARRAYSIZE(famasSkins))) { g_skinConfig[10] = MockSkinConfig{famasPaintKits[famasSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === GALIL-AR ===
		const char* galilSkins[] = { "Default", "Chatterbox", "Chromatic Aberration", "Sugar Rush", "Eco", "Cerberus", "Rocket Pop" };
		const int galilPaintKits[] = { 0, 398, 1144, 661, 428, 379, 478 };
		static int galilSkinIdx = 0;
		if (ImGui::Combo("Galil-AR", &galilSkinIdx, galilSkins, IM_ARRAYSIZE(galilSkins))) { g_skinConfig[13] = MockSkinConfig{galilPaintKits[galilSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === M4A1-S ===
		const char* m4a1sSkins[] = { "Default", "Printstream", "Player Two", "Mecha Industries", "Chantico's Fire", "Golden Coil", "Hyper Beast", "Cyrex", "Fade", "Imminent Danger", "Welcome to the Jungle", "Black Lotus", "Nightmare", "Leaded Glass", "Decimator", "Atomic Alloy", "Guardian", "Blue Phosphor", "Control Panel", "Hot Rod", "Master Piece", "Knight" };
		const int m4a1sPaintKits[] = { 0, 984, 946, 587, 548, 497, 430, 312, 1041, 1073, 1001, 1102, 714, 681, 644, 301, 257, 1017, 792, 445, 321, 326 };
		static int m4a1sSkinIdx = 0;
		if (ImGui::Combo("M4A1-S", &m4a1sSkinIdx, m4a1sSkins, IM_ARRAYSIZE(m4a1sSkins))) { g_skinConfig[60] = MockSkinConfig{m4a1sPaintKits[m4a1sSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === M4A4 ===
		const char* m4a4Skins[] = { "Default", "Howl", "In Living Color", "The Emperor", "Neo-Noir", "Buzz Kill", "The Battlestar", "Royal Paladin", "Bullet Rain", "Desert-Strike", "Asiimov", "X-Ray", "The Coalition", "Cyber Security", "Tooth Fairy", "Hellfire", "Desolate Space", "Dragon King", "Poseidon" };
		const int m4a4PaintKits[] = { 0, 309, 1041, 844, 695, 632, 533, 512, 155, 336, 255, 215, 1063, 985, 971, 664, 588, 400, 449 };
		static int m4a4SkinIdx = 0;
		if (ImGui::Combo("M4A4", &m4a4SkinIdx, m4a4Skins, IM_ARRAYSIZE(m4a4Skins))) { g_skinConfig[16] = MockSkinConfig{m4a4PaintKits[m4a4SkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === SSG-08 ===
		const char* ssgSkins[] = { "Default", "Dragonfire", "Blood in the Water", "Turbo Peek", "Bloodshot", "Big Iron", "Death Strike" };
		const int ssgPaintKits[] = { 0, 624, 222, 1101, 899, 503, 1052 };
		static int ssgSkinIdx = 0;
		if (ImGui::Combo("SSG-08", &ssgSkinIdx, ssgSkins, IM_ARRAYSIZE(ssgSkins))) { g_skinConfig[40] = MockSkinConfig{ssgPaintKits[ssgSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		// === DESERT EAGLE ===
		const char* deagleSkins[] = { "Default", "Ocean Drive", "Printstream", "Code Red", "Golden Koi", "Mecha Industries", "Kumicho Dragon", "Conspiracy", "Cobalt Disruption", "Hypnotic", "Fennec Fox" };
		const int deaglePaintKits[] = { 0, 1090, 984, 711, 185, 587, 527, 351, 231, 61, 1051 };
		static int deagleSkinIdx = 0;
		if (ImGui::Combo("Desert Eagle", &deagleSkinIdx, deagleSkins, IM_ARRAYSIZE(deagleSkins))) { g_skinConfig[1] = MockSkinConfig{deaglePaintKits[deagleSkinIdx], 0, 0.0f}; g_skinUpdateCounter++; MarkDirty(); }

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Агенты, Ножи и Перчатки отключены для 100% стабильности (Anti-Crash).");
	}
	else if (activeTab == TAB_CONFIGS)
	{
		ImGui::TextUnformatted("Config Manager");
		ImGui::Separator();
		
		// Get DLL directory for configs
		static std::string s_dllDir;
		static std::vector<std::string> s_configFiles;
		static int s_selectedConfig = 0;
		static ULONGLONG s_lastRefresh = 0;
		
		// Refresh config list periodically
		ULONGLONG now = GetTickCount64();
		if (s_dllDir.empty() || now - s_lastRefresh > 2000) {
			if (GetDllDirA(s_dllDir)) {
				s_configFiles.clear();
				
				std::string searchPath = s_dllDir + "\\*.ini";
				WIN32_FIND_DATAA findData;
				HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
				if (hFind != INVALID_HANDLE_VALUE) {
					do {
						if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
							s_configFiles.push_back(findData.cFileName);
						}
					} while (FindNextFileA(hFind, &findData));
					FindClose(hFind);
				}
			}
			s_lastRefresh = now;
		}
		
		// Config list
		ImGui::Text("Available Configs:");
		if (ImGui::BeginListBox("##configs_list", ImVec2(-1, 200))) {
			for (int i = 0; i < (int)s_configFiles.size(); i++) {
				bool isSelected = (s_selectedConfig == i);
				if (ImGui::Selectable(s_configFiles[i].c_str(), isSelected)) {
					s_selectedConfig = i;
				}
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
		
		ImGui::Spacing();
		
		// Action buttons
		if (ImGui::Button("Load Selected", ImVec2(120, 0))) {
			if (s_selectedConfig >= 0 && s_selectedConfig < (int)s_configFiles.size()) {
				// Copy selected config to config.ini
				std::string srcPath = s_dllDir + "\\" + s_configFiles[s_selectedConfig];
				std::string dstPath = s_dllDir + "\\config.ini";
				CopyFileA(srcPath.c_str(), dstPath.c_str(), FALSE);
				config::LoadConfig();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save As...", ImVec2(120, 0))) {
			static char newConfigName[64] = "new_config.ini";
			ImGui::OpenPopup("Save Config As");
		}
		
		// Save As popup
		if (ImGui::BeginPopupModal("Save Config As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			static char newConfigName[64] = "new_config.ini";
			ImGui::InputText("Filename", newConfigName, sizeof(newConfigName));
			ImGui::Spacing();
			if (ImGui::Button("Save", ImVec2(100, 0))) {
				std::string srcPath = s_dllDir + "\\config.ini";
				std::string dstPath = s_dllDir + "\\" + std::string(newConfigName);
				config::SaveConfig();
				CopyFileA(srcPath.c_str(), dstPath.c_str(), FALSE);
				s_lastRefresh = 0; // Force refresh
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Configs — это .ini файлы рядом с DLL.");
		ImGui::TextDisabled("Выбери конфиг и нажми Load для переключения.");
	}
	else if (activeTab == TAB_OTHER)
	{
		ui::CyberSectionHeader("THEME");

		if (ui::CyberColorEdit4("Accent Primary", &config::g_uiAccentColor)) {
			ui::theme::RebuildFromUserAccent(config::g_uiAccentColor, config::g_uiAccentSecondary);
			ApplyClientStyle();
			MarkDirty();
		}
		if (ui::CyberColorEdit4("Accent Secondary", &config::g_uiAccentSecondary)) {
			ui::theme::RebuildFromUserAccent(config::g_uiAccentColor, config::g_uiAccentSecondary);
			ApplyClientStyle();
			MarkDirty();
		}

		ImGui::Spacing();
		ui::CyberSectionHeader("EFFECTS");

		ui::CyberToggle("Scanline overlay", &config::g_uiScanlineEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		if (ui::CyberSlider("Scanline intensity", &config::g_uiScanlineIntensity, 0.0f, 1.0f, "%.2f"))
			MarkDirty();

		ui::CyberToggle("Hover glow", &config::g_uiHoverGlowEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();

		ui::CyberToggle("Animations", &config::g_uiAnimationsEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();
		ImGui::SameLine();
		ImGui::TextDisabled("(мастер-выключатель)");

		ui::CyberToggle("Background particles", &config::g_uiParticlesEnabled);
		if (ImGui::IsItemClicked()) MarkDirty();

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Конфиг сохраняется автоматически.");
	}
	ImGui::PopStyleVar();

	ImGui::EndChild();

	// PREVIEW PANEL inside content (shows only when ESP tab active) — drawn as floating overlay in the bottom-right.
	if (activeTab == TAB_ESP) {
		// Float preview in the right-bottom corner of the content area.
		const float prevW = 200.0f, prevH = 180.0f;
		ImVec2 anchor = ImVec2(winMax.x - prevW - 18.0f, winMax.y - prevH - ui::theme::kFooterH - 10.0f);
		ImDrawList* fdl = ImGui::GetWindowDrawList();
		ImVec2 pmin = anchor;
		ImVec2 pmax = ImVec2(anchor.x + prevW, anchor.y + prevH);

		fdl->AddRectFilled(pmin, pmax, IM_COL32(15, 16, 26, 230), 6.0f);
		fdl->AddRect(pmin, pmax, TP.Accent, 6.0f, 0, 1.0f);

		float cx = pmin.x + prevW * 0.5f;
		float cy = pmin.y + prevH * 0.5f + 6.0f;
		float bw = 40.0f, bh = 85.0f;
		float bl = cx - bw * 0.5f, bt = cy - bh * 0.5f;

		if (config::g_chamsEnabled) {
			ImU32 glowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(config::g_chamsColorT.x, config::g_chamsColorT.y, config::g_chamsColorT.z, 0.3f));
			for (int i = 1; i <= 4; i++) fdl->AddRect(ImVec2(bl - (float)i, bt - (float)i), ImVec2(bl + bw + (float)i, bt + bh + (float)i), glowCol, 0.0f, 0, 1.0f);
			fdl->AddRectFilled(ImVec2(bl, bt), ImVec2(bl + bw, bt + bh), ImGui::ColorConvertFloat4ToU32(ImVec4(config::g_chamsColorT.x, config::g_chamsColorT.y, config::g_chamsColorT.z, 0.15f)));
		}
		if (config::g_whEnabled) fdl->AddRect(ImVec2(bl, bt), ImVec2(bl + bw, bt + bh), ImGui::ColorConvertFloat4ToU32(config::g_boxColor), 0.0f, 0, g_boxThickness);
		if (config::g_hpBarEnabled) {
			fdl->AddRectFilled(ImVec2(bl - 8, bt), ImVec2(bl - 4, bt + bh), IM_COL32(0, 0, 0, 180));
			fdl->AddRectFilled(ImVec2(bl - 7, bt + bh * 0.25f + 1.0f), ImVec2(bl - 5, bt + bh - 1.0f), IM_COL32(80, 220, 80, 255));
		}
		if (config::g_bonesEnabled) {
			ImU32 bc = ImGui::ColorConvertFloat4ToU32(config::g_boneColor);
			fdl->AddLine(ImVec2(cx, bt + 5),  ImVec2(cx, bt + 45), bc, 1.5f);
			fdl->AddLine(ImVec2(cx, bt + 20), ImVec2(bl, bt + 40), bc, 1.5f);
			fdl->AddLine(ImVec2(cx, bt + 20), ImVec2(bl + bw, bt + 40), bc, 1.5f);
			fdl->AddLine(ImVec2(cx, bt + 45), ImVec2(bl + 5.0f, bt + bh), bc, 1.5f);
			fdl->AddLine(ImVec2(cx, bt + 45), ImVec2(bl + bw - 5.0f, bt + bh), bc, 1.5f);
			fdl->AddCircleFilled(ImVec2(cx, bt + 5), 5.0f, bc);
		}
		if (config::g_nameEspEnabled)
			fdl->AddText(ImVec2(bl - 5.0f, bt - 18.0f), ImGui::ColorConvertFloat4ToU32(config::g_nameColor), "Player");
		if (config::g_gunEspEnabled)
			fdl->AddText(ImVec2(bl, bt + bh + 4.0f), ImGui::ColorConvertFloat4ToU32(config::g_weaponIconColor), "AK-47");
	}

	ImGui::PopStyleVar(); // content window padding
	ImGui::PopStyleColor(); // content ChildBg

	// ---- Footer (status bar) ----
	{
		ImVec2 ftMin = ImVec2(p.x, winMax.y - ui::theme::kFooterH);
		ImVec2 ftMax = winMax;
		drawList->AddRectFilled(ftMin, ftMax, IM_COL32(10, 12, 18, 220), 0.0f);
		drawList->AddLine(ImVec2(ftMin.x + 12.0f, ftMin.y), ImVec2(ftMax.x - 12.0f, ftMin.y),
		                  (TP.Accent & 0x00FFFFFFu) | ((ImU32)60 << 24), 1.0f);
		const char* hint = "RShift: open/close menu  \xE2\x80\xA2  END: unload";
		ImGui::SetCursorPos(ImVec2(14.0f, s_win.y - ui::theme::kFooterH + 7.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, TP.TextMutedV);
		ImGui::TextUnformatted(hint);
		ImGui::PopStyleColor();

		if (s_cfgDirty) {
			const char* saving = "\xE2\x97\x8F saving...";
			ImVec2 ts = ImGui::CalcTextSize(saving);
			ImGui::SetCursorPos(ImVec2(s_win.x - ts.x - 18.0f, s_win.y - ui::theme::kFooterH + 7.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, TP.AccentV);
			ImGui::TextUnformatted(saving);
			ImGui::PopStyleColor();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar(4); // WindowRounding, WindowBorderSize, WindowPadding, Alpha

	// ---- Scanline overlay (drawn in ForegroundDrawList after End) ----
	ui::CyberDrawScanlines(p, winMax);
}



// UpdateAntiFlash moved to player/player_antiflash.cpp

// UpdateNoSmoke moved to player/player_nosmoke.cpp

static bool KeybindWidget(const char* id, int& vk)
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
		// Проверяем Shift для сброса (как ты просил)
		else if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			// Если нажали Shift во время бинда - считаем это сбросом? 
			// Или можно биндить Shift. Сделаем как просил: Shift очищает.
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
				
				// Пропускаем клик ЛКМ, которым мы активировали кнопку (ждем пока отпустят)
				if (k == VK_LBUTTON && ImGui::IsMouseDown(0)) continue;

				// Проверка нажатия (async state)
				if (GetAsyncKeyState(k) & 0x8000)
				{
					vk = k;
					s_capturing = 0; // Перестаем слушать
					changed = true;
					config::SaveConfig(); // Автосохранение
					break;
				}
			}
		}
	}

	ImGui::PopID();
	return changed;
}

static const char* VkToStringA(int vk)
{
	static char buf[64];
	buf[0] = '\0';
	if (vk == 0) return "[none]";

	UINT scan = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
	LONG lParam = (LONG)(scan << 16);
	if (vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN || vk == VK_PRIOR || vk == VK_NEXT || vk == VK_END || vk == VK_HOME || vk == VK_INSERT || vk == VK_DELETE)
		lParam |= 1 << 24;

	int n = GetKeyNameTextA(lParam, buf, (int)sizeof(buf));
	if (n > 0) return buf;
	wsprintfA(buf, "VK_%d", vk);
	return buf;
}

void DrawKeybindsListImGui() {
    if (!config::g_keybindsListEnabled) return;
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    if (!config::g_menuOpen) {
        flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }
    
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(config::g_uiAccentColor.x, config::g_uiAccentColor.y, config::g_uiAccentColor.z, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    
    ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("KeybindsList", nullptr, flags);
    
    ImGui::TextColored(config::g_uiAccentColor, "Keybinds");
    ImGui::Separator();
    
    // Aimbot
    if (config::g_aimbotEnabled) {
        bool active = (GetAsyncKeyState(config::g_aimbotKey) & 0x8000) != 0;
        ImGui::TextColored(active ? config::g_uiAccentColor : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
            "Aimbot [%s]", VkToStringA(config::g_aimbotKey));
        if (active) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
        }
    }
    
    // Triggerbot
    if (config::g_triggerEnabled) {
        bool active = (GetAsyncKeyState(config::g_triggerKey) & 0x8000) != 0;
        ImGui::TextColored(active ? config::g_uiAccentColor : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
            "Triggerbot [%s]", VkToStringA(config::g_triggerKey));
        if (active) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
        }
    }
    
    // Bhop
    if (config::g_bhopEnabled) {
        bool active = (GetAsyncKeyState(config::g_bhopKey) & 0x8000) != 0;
        ImGui::TextColored(active ? config::g_uiAccentColor : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
            "Bhop [%s]", VkToStringA(config::g_bhopKey));
        if (active) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
        }
    }
    
    ImGui::End();
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void ApplyClientStyle()
{
	// Ensure palette is rebuilt from latest user accents before we copy into ImGuiStyle.
	ui::theme::RebuildFromUserAccent(config::g_uiAccentColor, config::g_uiAccentSecondary);
	const ui::theme::Palette& TP = ui::theme::Colors();

	ImGuiStyle& s = ImGui::GetStyle();

	s.WindowPadding    = ImVec2(16, 16);
	s.FramePadding     = ImVec2(10, 6);
	s.ItemSpacing      = ImVec2(10, 8);
	s.ItemInnerSpacing = ImVec2(8, 6);
	s.IndentSpacing    = 16.0f;

	s.WindowRounding    = 8.0f;
	s.ChildRounding     = ui::theme::kPanelRounding;
	s.FrameRounding     = ui::theme::kWidgetRounding;
	s.PopupRounding     = ui::theme::kPanelRounding;
	s.ScrollbarRounding = 12.0f;
	s.GrabMinSize       = 10.0f;
	s.GrabRounding      = 4.0f;
	s.TabRounding       = 5.0f;

	s.WindowBorderSize = 0.0f;
	s.ChildBorderSize  = 1.0f;
	s.FrameBorderSize  = 0.0f;
	s.PopupBorderSize  = 1.0f;
	s.TabBorderSize    = 0.0f;

	ImVec4* c = s.Colors;

	// Backgrounds
	c[ImGuiCol_WindowBg]       = TP.BgBaseV;
	c[ImGuiCol_ChildBg]        = TP.BgPanelV;
	c[ImGuiCol_PopupBg]        = TP.BgOverlayV;
	c[ImGuiCol_Border]         = ImVec4(1, 1, 1, 0.08f);
	c[ImGuiCol_BorderShadow]   = ImVec4(0, 0, 0, 0);

	// Text
	c[ImGuiCol_Text]           = TP.TextPrimaryV;
	c[ImGuiCol_TextDisabled]   = TP.TextMutedV;

	// Inputs
	c[ImGuiCol_FrameBg]        = TP.BgInputFieldV;
	c[ImGuiCol_FrameBgHovered] = ImVec4(TP.BgInputFieldV.x + 0.04f, TP.BgInputFieldV.y + 0.04f, TP.BgInputFieldV.z + 0.05f, 1.0f);
	c[ImGuiCol_FrameBgActive]  = ImVec4(TP.BgInputFieldV.x + 0.08f, TP.BgInputFieldV.y + 0.08f, TP.BgInputFieldV.z + 0.10f, 1.0f);

	// Buttons
	c[ImGuiCol_Button]         = TP.BgInputFieldV;
	c[ImGuiCol_ButtonHovered]  = ImVec4(TP.AccentV.x * 0.35f, TP.AccentV.y * 0.35f, TP.AccentV.z * 0.35f, 1.0f);
	c[ImGuiCol_ButtonActive]   = TP.AccentV;

	// Accents
	c[ImGuiCol_CheckMark]         = TP.AccentV;
	c[ImGuiCol_SliderGrab]        = TP.AccentV;
	c[ImGuiCol_SliderGrabActive]  = ImVec4(TP.AccentV.x, TP.AccentV.y, TP.AccentV.z, 1.0f);

	// Headers (for collapsing headers, selectables, tree nodes)
	c[ImGuiCol_Header]         = ImVec4(TP.AccentV.x, TP.AccentV.y, TP.AccentV.z, 0.25f);
	c[ImGuiCol_HeaderHovered]  = ImVec4(TP.AccentV.x, TP.AccentV.y, TP.AccentV.z, 0.45f);
	c[ImGuiCol_HeaderActive]   = TP.AccentV;

	// Separators
	c[ImGuiCol_Separator]        = ImVec4(1, 1, 1, 0.10f);
	c[ImGuiCol_SeparatorHovered] = TP.AccentV;
	c[ImGuiCol_SeparatorActive]  = TP.AccentV;

	// Tabs (legacy tab control — we use custom sidebar now but keep styling for popups)
	c[ImGuiCol_Tab]                = TP.BgInputFieldV;
	c[ImGuiCol_TabHovered]         = ImVec4(TP.AccentV.x, TP.AccentV.y, TP.AccentV.z, 0.5f);
	c[ImGuiCol_TabActive]          = TP.AccentV;
	c[ImGuiCol_TitleBg]            = TP.BgBaseV;
	c[ImGuiCol_TitleBgActive]      = TP.BgPanelV;

	c[ImGuiCol_ModalWindowDimBg]   = ImVec4(0, 0, 0, 0.75f);
}




static bool LoadTextureFromResource(int resourceId, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	if (!out_srv || !core::GetD3DDevice() || !core::GetD3DDeviceContext())
		return false;

	*out_srv = nullptr;
	if (out_width) *out_width = 0;
	if (out_height) *out_height = 0;

	// Загружаем ресурс
	HRSRC hResource = FindResourceW(g_hModule, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
	if (!hResource)
		return false;

	HGLOBAL hMemory = LoadResource(g_hModule, hResource);
	if (!hMemory)
		return false;

	DWORD dwSize = SizeofResource(g_hModule, hResource);
	LPVOID lpAddress = LockResource(hMemory);
	if (!lpAddress || dwSize == 0)
		return false;

	// Создаем IStream из памяти
	IStream* stream = nullptr;
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);
	if (!hGlobal)
		return false;

	void* pGlobal = GlobalLock(hGlobal);
	if (!pGlobal)
	{
		GlobalFree(hGlobal);
		return false;
	}

	memcpy(pGlobal, lpAddress, dwSize);
	GlobalUnlock(hGlobal);

	HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &stream);
	if (FAILED(hr) || !stream)
	{
		GlobalFree(hGlobal);
		return false;
	}

	// Создаем WIC factory
	IWICImagingFactory* factory = nullptr;
	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
	if (FAILED(hr) || !factory)
	{
		stream->Release();
		return false;
	}

	// Декодируем из stream
	IWICBitmapDecoder* decoder = nullptr;
	hr = factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
	stream->Release();
	if (FAILED(hr) || !decoder)
	{
		factory->Release();
		return false;
	}

	IWICBitmapFrameDecode* frame = nullptr;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr) || !frame)
	{
		decoder->Release();
		factory->Release();
		return false;
	}

	IWICFormatConverter* converter = nullptr;
	hr = factory->CreateFormatConverter(&converter);
	if (FAILED(hr) || !converter)
	{
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	if (FAILED(hr))
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	UINT w = 0, h = 0;
	hr = converter->GetSize(&w, &h);
	if (FAILED(hr) || w == 0 || h == 0)
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	std::vector<BYTE> pixels;
	const UINT stride = w * 4;
	const UINT imageSize = stride * h;
	pixels.resize(imageSize);
	hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
	if (FAILED(hr))
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = w;
	texDesc.Height = h;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = pixels.data();
	initData.SysMemPitch = stride;

	ID3D11Texture2D* tex = nullptr;
	hr = core::GetD3DDevice()->CreateTexture2D(&texDesc, &initData, &tex);
	if (FAILED(hr) || !tex)
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* srv = nullptr;
	hr = core::GetD3DDevice()->CreateShaderResourceView(tex, &srvDesc, &srv);
	tex->Release();
	converter->Release();
	frame->Release();
	decoder->Release();
	factory->Release();

	if (FAILED(hr) || !srv)
		return false;

	*out_srv = srv;
	if (out_width) *out_width = (int)w;
	if (out_height) *out_height = (int)h;
	return true;
}

static bool LoadTextureFromFileW(const wchar_t* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	if (!filename || !out_srv || !core::GetD3DDevice() || !core::GetD3DDeviceContext())
		return false;
	*out_srv = nullptr;
	if (out_width) *out_width = 0;
	if (out_height) *out_height = 0;

	IWICImagingFactory* factory = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
	if (FAILED(hr) || !factory)
		return false;

	IWICBitmapDecoder* decoder = nullptr;
	hr = factory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr) || !decoder)
	{
		factory->Release();
		return false;
	}

	IWICBitmapFrameDecode* frame = nullptr;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr) || !frame)
	{
		decoder->Release();
		factory->Release();
		return false;
	}

	IWICFormatConverter* converter = nullptr;
	hr = factory->CreateFormatConverter(&converter);
	if (FAILED(hr) || !converter)
	{
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	if (FAILED(hr))
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	UINT w = 0, h = 0;
	hr = converter->GetSize(&w, &h);
	if (FAILED(hr) || w == 0 || h == 0)
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	std::vector<BYTE> pixels;
	const UINT stride = w * 4;
	const UINT imageSize = stride * h;
	pixels.resize(imageSize);
	hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
	if (FAILED(hr))
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = w;
	texDesc.Height = h;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = pixels.data();
	initData.SysMemPitch = stride;

	ID3D11Texture2D* tex = nullptr;
	hr = core::GetD3DDevice()->CreateTexture2D(&texDesc, &initData, &tex);
	if (FAILED(hr) || !tex)
	{
		converter->Release();
		frame->Release();
		decoder->Release();
		factory->Release();
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* srv = nullptr;
	hr = core::GetD3DDevice()->CreateShaderResourceView(tex, &srvDesc, &srv);
	tex->Release();
	converter->Release();
	frame->Release();
	decoder->Release();
	factory->Release();
	if (FAILED(hr) || !srv)
		return false;

	*out_srv = srv;
	if (out_width) *out_width = (int)w;
	if (out_height) *out_height = (int)h;
	return true;
}



