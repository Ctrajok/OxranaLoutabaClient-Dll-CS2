#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX
#include <Windows.h>
#include <dwmapi.h>
#include <dcomp.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <objbase.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dwmapi.lib")

#include "../config/config_vars.hpp"
#include "../config/config_manager.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"
#include "../esp/esp_bomb.hpp"

#include "../core/core_d3d.hpp"
#include "../core/core_window.hpp"
#include "../core/core_types.hpp"

#include "../hooks/hooks_manager.hpp"

#include "../skins/skins_apply.hpp"

#include "../ui/ui_menu.hpp"
#include "../ui/ui_widgets.hpp"

#include "../esp/esp_common.hpp"
#include "../esp/esp_bones.hpp"
#include "../esp/esp_snaplines.hpp"
#include "../esp/esp_distance.hpp"
#include "../esp/esp_spectators.hpp"

#include "../combat/combat_aimbot.hpp"
#include "../combat/combat_triggerbot.hpp"
#include "../combat/combat_recoil.hpp"
#include "../combat/combat_spread.hpp"
#include "../combat/combat_autostop.hpp"
#include "../combat/combat_autoknife.hpp"

#include "../movement/movement_bhop.hpp"
#include "../movement/movement_strafe.hpp"
#include "../movement/movement_jumpduck.hpp"

#include "../player/player_antiflash.hpp"
#include "../player/player_fov.hpp"
#include "../player/player_nosmoke.hpp"

// For ImGui
#include "../../../imgui-1.92.5/imgui.h"
#include "../../../imgui-1.92.5/backends/imgui_impl_win32.h"
#include "../../../imgui-1.92.5/backends/imgui_impl_dx11.h"
#include "../../../imgui-1.92.5/imgui_internal.h"
#include "../resource.h"


const wchar_t* g_wndClassName = L"OXRANALOUATABACLIENT";

// from core_main.cpp previously
HMODULE g_hModule = nullptr;
volatile bool g_running = true;
HANDLE g_hMainThread = nullptr;
DWORD g_mainThreadId = 0;
HWND g_hOverlayWnd = nullptr;

static bool IsCs2Active()
{
	HWND hwnd = GetForegroundWindow();
	if (!hwnd) return false;

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	return pid == GetCurrentProcessId();
}

// For missing functions that are still in ui_chunks.txt but not exposed yet
extern void DrawEspImGui();
extern void DrawKeybindsListImGui();
extern void DrawWatermarkImGui();
extern void DrawMenuImGui();
extern void ApplyClientStyle();
extern void UpdateESPInternal(int width, int height);
extern void UpdateHitInfo(uintptr_t client);
extern void UpdateShotTracking(uintptr_t localPawn);

// The main thread routine
DWORD WINAPI MainThread(LPVOID)
{
	HRESULT coInitHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool coInitOk = (coInitHr == S_OK || coInitHr == S_FALSE);

	timeBeginPeriod(1);

	// InitRuntimeOffsets();
	skins::InitSkinConfig();
	config::LoadConfig();
	
	hooks::Install();
	memory::InitializeOffsets();

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = core::OverlayWndProc;
	wc.hInstance = g_hModule;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = g_wndClassName;

	ATOM classAtom = RegisterClassExW(&wc);
	if (!classAtom) { FreeLibraryAndExitThread(g_hModule, 0); return 0; }

	int screenW = GetSystemMetrics(SM_CXSCREEN);
	int screenH = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		g_wndClassName, L"", WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, wc.hInstance, nullptr);

	if (!hwnd) { UnregisterClassW(g_wndClassName, wc.hInstance); FreeLibraryAndExitThread(g_hModule, 0); return 0; }
	g_hOverlayWnd = hwnd;
	SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
	
	if (config::g_antiCaptureEnabled) {
		SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
	}
	MARGINS margins = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	if (!core::CreateDeviceD3D(hwnd)) {
		core::CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClassW(g_wndClassName, wc.hInstance); FreeLibraryAndExitThread(g_hModule, 0); return 0;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// --- Загрузка кастомного шрифта из ресурсов DLL ---
	{
		HRSRC hFontRes = FindResourceW(g_hModule, MAKEINTRESOURCEW(IDR_FONT_MAIN), RT_RCDATA);
		if (hFontRes)
		{
			HGLOBAL hFontMem = LoadResource(g_hModule, hFontRes);
			if (hFontMem)
			{
				DWORD fontSize = SizeofResource(g_hModule, hFontRes);
				void* fontData = LockResource(hFontMem);
				if (fontData && fontSize > 0)
				{
					// ImGui берёт ownership копии, поэтому делаем memcpy
					void* fontCopy = IM_ALLOC(fontSize);
					if (fontCopy)
					{
						memcpy(fontCopy, fontData, fontSize);
						// Кириллица + латиница
						static const ImWchar ranges[] = {
							0x0020, 0x00FF, // Latin
							0x0400, 0x04FF, // Cyrillic
							0,
						};
						io.Fonts->AddFontFromMemoryTTF(fontCopy, (int)fontSize, 16.0f, nullptr, ranges);
					}
				}
			}
		}
		// Если шрифт не загрузился — ImGui использует встроенный дефолт
	}

	ApplyClientStyle();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(core::GetD3DDevice(), core::GetD3DDeviceContext());

	MSG msg{};
	static ULONGLONG s_lastFrameTick = 0;

	while (g_running)
	{
		while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT) g_running = false;
		}
		if (!g_running) break;

		bool cs2Active = IsCs2Active();
		if (!cs2Active) config::g_menuOpen = false;

		if (cs2Active) {
			if (GetAsyncKeyState(VK_F6) & 1) config::g_bhopEnabled = !config::g_bhopEnabled;
			if (config::g_whKey != 0 && (GetAsyncKeyState(config::g_whKey) & 1)) config::g_whEnabled = !config::g_whEnabled;
			if (config::g_bonesKey != 0 && (GetAsyncKeyState(config::g_bonesKey) & 1)) config::g_bonesEnabled = !config::g_bonesEnabled;
			if (config::g_triggerToggleKey != 0 && (GetAsyncKeyState(config::g_triggerToggleKey) & 1)) config::g_triggerEnabled = !config::g_triggerEnabled;
			
			if (GetAsyncKeyState(VK_RSHIFT) & 1) config::g_menuOpen = !config::g_menuOpen;
			if (config::g_menuOpen && (GetAsyncKeyState(VK_ESCAPE) & 1)) config::g_menuOpen = false;
			
			if (GetAsyncKeyState(VK_END) & 1) { g_running = false; break; }
		}

		uintptr_t client = memory::GetClientBase();
		uintptr_t localPawn = 0;
		uintptr_t localCtrl = 0; 
		if (client) {
			memory::TryRead<uintptr_t>(client + memory::GetOffsets().dwLocalPlayerPawn, localPawn);
			memory::TryRead<uintptr_t>(client + memory::GetOffsets().dwLocalPlayerController, localCtrl);
		}
		uintptr_t localScene = 0;
		int localHp = 0;
		if (localPawn && localCtrl) {
			memory::TryRead<uintptr_t>(localPawn + memory::GetOffsets().m_pGameSceneNode, localScene);
			memory::TryRead<int>(localPawn + memory::GetOffsets().m_iHealth, localHp);
		}
		
		bool validLocalPlayer = (client && localPawn && localCtrl && localScene && localHp >= -100 && localHp < 10000);
		static bool s_prevValid = false;
		if (s_prevValid && !validLocalPlayer)
		{
			esp::GetEspBoxes().clear();
			esp::GetEspTargetsWorld().clear();
			esp::GetBoneCache().clear();
			esp::GetBoneSkeletons().clear();
			esp::GetSpectators().clear();
		}
		s_prevValid = validLocalPlayer;
		
		if (validLocalPlayer && cs2Active) {
			UpdateShotTracking(localPawn);
			UpdateHitInfo(client);
			combat::UpdateAimbot(true);
			combat::UpdateTriggerbot(true);
			combat::UpdateAutoStop(true);
			combat::UpdateAutoKnife(true);
			movement::UpdateBunnyHop(true);
			movement::UpdateJumpDuck(true);
			movement::UpdateJumpScout(true);
			player::UpdateFovOverride(true);
			combat::UpdateRecoilControl(true);
			combat::UpdateNoSpread(true);
			player::UpdateAntiFlash(true);
			player::UpdateNoSmoke(true);
		} else {
			if (!esp::GetEspBoxes().empty()) esp::GetEspBoxes().clear();
			if (!esp::GetEspTargetsWorld().empty()) esp::GetEspTargetsWorld().clear();
			if (!esp::GetBoneCache().empty()) esp::GetBoneCache().clear();
			if (!esp::GetBoneSkeletons().empty()) esp::GetBoneSkeletons().clear();
			if (!esp::GetSpectators().empty()) esp::GetSpectators().clear();
			esp::GetBombData().found = false;
			esp::GetBombData().pos = { 0,0,0 };
		}

		bool needRender = config::g_menuOpen || (validLocalPlayer && cs2Active && (config::g_whEnabled || config::g_bonesEnabled || config::g_aimbotDrawFov || config::g_bombEspEnabled || config::g_spectatorListEnabled));
		
		core::SetOverlayInputMode(hwnd, config::g_menuOpen);

		static bool s_prevShow = false;
		if (needRender != s_prevShow) {
			ShowWindow(hwnd, needRender ? SW_SHOWNA : SW_HIDE);
			s_prevShow = needRender;
		}

		if (!needRender) {
			Sleep(16);
			continue;
		}

		ULONGLONG frameNow = GetTickCount64();
		if (s_lastFrameTick != 0 && (frameNow - s_lastFrameTick) < 8) {
			Sleep(1);
			continue;
		}
		s_lastFrameTick = frameNow;

		RECT rc{}; GetClientRect(hwnd, &rc);
		int width = rc.right - rc.left;
		int height = rc.bottom - rc.top;
		float view[16]{};

		if (validLocalPlayer && cs2Active) {
			__try {
				for (int i = 0; i < 16; ++i)
					memory::TryRead<float>(client + memory::GetOffsets().dwViewMatrix + i * sizeof(float), view[i]);
			} __except (1) { }

			static ULONGLONG s_lastEspTick = 0;
			if (frameNow - s_lastEspTick >= 33) {
				if (config::g_whEnabled || config::g_bonesEnabled || config::g_bombEspEnabled || config::g_spectatorListEnabled || config::g_damageIndicatorsEnabled || config::g_nameEspEnabled || config::g_gunEspEnabled || config::g_hpBarEnabled || config::g_chamsEnabled || config::g_radarHackEnabled || config::g_oofArrowsEnabled) UpdateESPInternal(width, height);
				if (config::g_bonesEnabled) esp::UpdateBonesCache(width, height);
				s_lastEspTick = frameNow;
			}
			
			if (config::g_whEnabled || config::g_nameEspEnabled || config::g_gunEspEnabled || config::g_hpBarEnabled) {
				esp::ProjectEspBoxes(view, width, height);
			}
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		
		if (config::g_menuOpen) {
			ImGuiIO& curIO = ImGui::GetIO();
			curIO.MouseDrawCursor = true;
		} else {
			ImGuiIO& curIO = ImGui::GetIO();
			curIO.MouseDrawCursor = false;
			static bool s_wasMenuOpen = false;
			if (s_wasMenuOpen && !config::g_menuOpen) {
				HWND cs2Wnd = FindWindowW(nullptr, L"Counter-Strike 2");
				if (cs2Wnd) {
					SetForegroundWindow(cs2Wnd);
					SetActiveWindow(cs2Wnd);
					SetFocus(cs2Wnd);
				}
			}
			s_wasMenuOpen = config::g_menuOpen;
		}

		ImGui::NewFrame();

		if (validLocalPlayer && cs2Active) {
			DrawEspImGui();
			esp::RenderSnaplines(ImGui::GetBackgroundDrawList());
			esp::RenderDistance(ImGui::GetBackgroundDrawList());
			if (config::g_bonesEnabled) esp::RenderBones(view, width, height, ImGui::GetBackgroundDrawList());
			esp::RenderSpectatorList();
		}
		
		DrawKeybindsListImGui();
		DrawWatermarkImGui();
		DrawMenuImGui();

		ImGui::Render();

		// if (!g_mainRenderTargetView) CreateRenderTarget();
		if (core::GetRenderTargetView()) {
			const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			ID3D11RenderTargetView* rtv = core::GetRenderTargetView();
			core::GetD3DDeviceContext()->OMSetRenderTargets(1, &rtv, nullptr);
			core::GetD3DDeviceContext()->ClearRenderTargetView(rtv, clear_color);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}
		core::GetSwapChain()->Present(0, 0);
	}

	ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
	core::CleanupDeviceD3D();
	if (g_hOverlayWnd) DestroyWindow(g_hOverlayWnd);
	UnregisterClassW(g_wndClassName, wc.hInstance);
	if (coInitOk) CoUninitialize();
	
	hooks::Uninstall();
	
	timeEndPeriod(1);
	FreeLibraryAndExitThread(g_hModule, 0);
	return 0;
}

namespace core {
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		g_hMainThread = CreateThread(nullptr, 0, MainThread, nullptr, 0, &g_mainThreadId);
		break;

	case DLL_PROCESS_DETACH:
		if (lpReserved == nullptr)
		{
			g_running = false;
			if (g_hOverlayWnd)
			{
				PostMessageW(g_hOverlayWnd, WM_QUIT, 0, 0);
			}
			if (g_hMainThread)
			{
				CloseHandle(g_hMainThread);
				g_hMainThread = nullptr;
			}
		}
		break;
	}
	return TRUE;
}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return core::DllMain(hModule, ul_reason_for_call, lpReserved);
}

// --- IMPL INCLUDES ---
// These are included directly to be compiled within the core_main translation unit
// avoiding the need to modify the MSBuild project files extensively.
#include "../config/config_parser.cpp"

#pragma warning(push)
#pragma warning(disable: 6011)
#pragma warning(disable: 6387)
#include "../../../imgui-1.92.5/imgui.cpp"
#include "../../../imgui-1.92.5/imgui_draw.cpp"
#include "../../../imgui-1.92.5/imgui_tables.cpp"
#include "../../../imgui-1.92.5/imgui_widgets.cpp"
#include "../../../imgui-1.92.5/backends/imgui_impl_dx11.cpp"
#include "../../../imgui-1.92.5/backends/imgui_impl_win32.cpp"
#pragma warning(pop)