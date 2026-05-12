#include "hooks_manager.hpp"
#include <Windows.h>
#include "../MinHook.h"
#include "../resource.h"
#include "../audio/audio_hitsound.hpp"

// External variables from cs2_internal_dll.cpp
extern HMODULE g_hModule;
extern LPVOID g_hitSoundBuffer;
extern DWORD  g_hitSoundSize;

// Hook-related globals
void** g_pSource2ClientVTable = nullptr;

// FrameStageNotify hook types and variables
typedef void(__fastcall* FrameStageNotify_t)(void* rcx, int curStage);
namespace skins {
    FrameStageNotify_t oFrameStageNotify = nullptr;
    extern void __fastcall hkFrameStageNotify(void* rcx, int curStage);
}
const int FRAMESTAGENOTIFY_INDEX = 36;

// GetInterface helper
typedef void* (*tCreateInterface)(const char* name, int* returnCode);
void* GetInterface(const char* dllName, const char* interfaceName) {
    HMODULE hMod = GetModuleHandleA(dllName);
    if (!hMod) return nullptr;
    tCreateInterface createInterface = (tCreateInterface)GetProcAddress(hMod, "CreateInterface");
    if (!createInterface) return nullptr;
    return createInterface(interfaceName, nullptr);
}

#pragma comment(lib, "libMinHook.x64.lib")

namespace hooks {

void Install()
{
    // --- ПРЕДЗАГРУЗКА ЗВУКА В КЭШ ДЛЯ МОМЕНТАЛЬНОГО ВОСПРОИЗВЕДЕНИЯ ---
    HRSRC hRes = FindResourceW(g_hModule, MAKEINTRESOURCEW(IDR_WAV_HITSOUND), RT_RCDATA);
    if (hRes) {
        HGLOBAL hMem = LoadResource(g_hModule, hRes);
        if (hMem) {
            g_hitSoundBuffer = LockResource(hMem);
            g_hitSoundSize   = SizeofResource(g_hModule, hRes);
        }
    }

    // Инициализируем DirectSound-backed проигрыватель (volume control).
    if (g_hitSoundBuffer && g_hitSoundSize > 0) {
        audio::InitHitSound(g_hitSoundBuffer, g_hitSoundSize);
    }

    MH_Initialize(); // Инициализируем MinHook

    // Ставим VTable хук на скинченджер
    void* pSource2Client = GetInterface("client.dll", "Source2Client002");
    if (pSource2Client) {
        g_pSource2ClientVTable = *(void***)pSource2Client;
        DWORD oldProtect;
        VirtualProtect(&g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        skins::oFrameStageNotify = (FrameStageNotify_t)g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX];
        g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX] = (void*)skins::hkFrameStageNotify;
        VirtualProtect(&g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX], sizeof(void*), oldProtect, &oldProtect);
    }
}

void Uninstall()
{
    audio::ShutdownHitSound();

    if (g_pSource2ClientVTable && skins::oFrameStageNotify)
    {
        DWORD oldProtect;
        VirtualProtect(&g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        
        g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX] = (void*)skins::oFrameStageNotify;
        
        VirtualProtect(&g_pSource2ClientVTable[FRAMESTAGENOTIFY_INDEX], sizeof(void*), oldProtect, &oldProtect);
    }
}

} // namespace hooks
