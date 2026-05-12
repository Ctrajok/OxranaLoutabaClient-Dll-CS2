#include "audio_hitsound.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmreg.h>
#include <dsound.h>
#include <cstring>
#include <cmath>
#include <atomic>

// Линкуется в core_main.cpp через #pragma comment(lib, "dsound.lib")
// (добавим там). User32 линкуется по умолчанию.

namespace audio {

namespace {

// --- Парсинг WAV (RIFF) ---
struct WavInfo
{
    WAVEFORMATEX   format;
    const BYTE*    samples;
    DWORD          sampleBytes;
    bool           ok;
};

// Минимальный RIFF reader. Находит 'fmt ' и 'data' чанки.
// WAV в ресурсе CS2-чита — обычный PCM, так что простой обход достаточно.
static WavInfo ParseWav(const void* data, DWORD size)
{
    WavInfo info{};
    if (!data || size < 44) return info;

    const BYTE* p = (const BYTE*)data;
    const BYTE* end = p + size;

    // RIFF header: "RIFF" <size4> "WAVE"
    if (std::memcmp(p, "RIFF", 4) != 0) return info;
    if (std::memcmp(p + 8, "WAVE", 4) != 0) return info;
    p += 12;

    bool gotFmt = false;
    while (p + 8 <= end) {
        char tag[5] = {0};
        std::memcpy(tag, p, 4);
        DWORD chunkSize = 0;
        std::memcpy(&chunkSize, p + 4, 4);
        const BYTE* chunkData = p + 8;
        if (chunkData + chunkSize > end) break;

        if (std::memcmp(tag, "fmt ", 4) == 0) {
            if (chunkSize >= sizeof(WAVEFORMATEX) - 2) {
                // Читаем как PCMWAVEFORMAT (18 байт WAVEFORMATEX с cbSize=0).
                std::memset(&info.format, 0, sizeof(info.format));
                std::memcpy(&info.format, chunkData,
                    chunkSize < sizeof(WAVEFORMATEX) ? chunkSize : sizeof(WAVEFORMATEX));
                gotFmt = true;
            }
        } else if (std::memcmp(tag, "data", 4) == 0) {
            info.samples = chunkData;
            info.sampleBytes = chunkSize;
            if (gotFmt) {
                info.ok = true;
                return info;
            }
        }

        p = chunkData + chunkSize;
        if (chunkSize & 1) ++p; // align
    }
    return info;
}

// --- DirectSound state ---
typedef HRESULT (WINAPI* DirectSoundCreate8_t)(LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN);

HMODULE              g_dsoundDll = nullptr;
IDirectSound8*       g_ds        = nullptr;
IDirectSoundBuffer*  g_buffer    = nullptr;
std::atomic<bool>    g_inited{false};

// Конвертация 0..100 громкости в логарифмические децибелы DirectSound.
// DSBVOLUME_MIN = -10000 (слышно как тишина), DSBVOLUME_MAX = 0 (original).
// Линейный процент → лог dB: vol = 2000 * log10(percent / 100).
static LONG PercentToDSVolume(int percent)
{
    if (percent <= 0)   return DSBVOLUME_MIN;
    if (percent >= 100) return DSBVOLUME_MAX;
    double linear = (double)percent / 100.0;
    double db = 2000.0 * std::log10(linear);
    if (db < DSBVOLUME_MIN) db = DSBVOLUME_MIN;
    if (db > DSBVOLUME_MAX) db = DSBVOLUME_MAX;
    return (LONG)db;
}

} // namespace

bool InitHitSound(const void* wavData, unsigned long wavSize)
{
    if (g_inited.load()) return true;
    if (!wavData || wavSize < 44) return false;

    WavInfo wav = ParseWav(wavData, wavSize);
    if (!wav.ok) return false;

    // Загружаем dsound.dll динамически (dsound.lib не нужен).
    g_dsoundDll = LoadLibraryA("dsound.dll");
    if (!g_dsoundDll) return false;

    auto pDirectSoundCreate8 =
        (DirectSoundCreate8_t)GetProcAddress(g_dsoundDll, "DirectSoundCreate8");
    if (!pDirectSoundCreate8) {
        FreeLibrary(g_dsoundDll); g_dsoundDll = nullptr;
        return false;
    }

    if (FAILED(pDirectSoundCreate8(nullptr, &g_ds, nullptr)) || !g_ds) return false;

    // Нужен active window для SetCooperativeLevel. GetForegroundWindow работает,
    // но если вдруг nullptr — GetDesktopWindow как fallback.
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) hwnd = GetDesktopWindow();
    if (FAILED(g_ds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))) {
        // Не фатально — пробуем NORMAL.
        g_ds->SetCooperativeLevel(hwnd, DSSCL_NORMAL);
    }

    // Secondary buffer с volume control.
    DSBUFFERDESC desc{};
    desc.dwSize        = sizeof(desc);
    desc.dwFlags       = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
    desc.dwBufferBytes = wav.sampleBytes;
    desc.lpwfxFormat   = &wav.format;

    if (FAILED(g_ds->CreateSoundBuffer(&desc, &g_buffer, nullptr)) || !g_buffer) {
        g_ds->Release(); g_ds = nullptr;
        return false;
    }

    // Заливаем PCM-данные в буфер.
    void* ptr1 = nullptr; DWORD size1 = 0;
    void* ptr2 = nullptr; DWORD size2 = 0;
    if (SUCCEEDED(g_buffer->Lock(0, wav.sampleBytes, &ptr1, &size1, &ptr2, &size2, 0))) {
        if (ptr1 && size1) std::memcpy(ptr1, wav.samples, size1);
        if (ptr2 && size2) std::memcpy(ptr2, wav.samples + size1, size2);
        g_buffer->Unlock(ptr1, size1, ptr2, size2);
    }

    g_inited.store(true);
    return true;
}

void PlayHitSound(int volumePercent)
{
    if (!g_inited.load() || !g_buffer) return;

    g_buffer->SetVolume(PercentToDSVolume(volumePercent));
    // Stop+SetPos позволяет триггерить подряд быстрые выстрелы без
    // "залипания" на первой не закончившейся проигрывке.
    g_buffer->Stop();
    g_buffer->SetCurrentPosition(0);
    g_buffer->Play(0, 0, 0);
}

void ShutdownHitSound()
{
    if (g_buffer) { g_buffer->Release(); g_buffer = nullptr; }
    if (g_ds)     { g_ds->Release();     g_ds = nullptr; }
    if (g_dsoundDll) { FreeLibrary(g_dsoundDll); g_dsoundDll = nullptr; }
    g_inited.store(false);
}

} // namespace audio
