#include "core_window.hpp"
#include "../config/config_vars.hpp"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace core {

static HWND g_hOverlayWnd = nullptr;
static const wchar_t* g_wndClassName = L"CS2_DX11_Overlay";

HWND CreateOverlayWindow()
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(nullptr); // Using main executable instance
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = g_wndClassName;

    ATOM classAtom = RegisterClassExW(&wc);
    if (!classAtom) { return nullptr; }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        g_wndClassName, L"", WS_POPUP, 0, 0, screenW, screenH, nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd) { UnregisterClassW(g_wndClassName, wc.hInstance); return nullptr; }
    
    g_hOverlayWnd = hwnd;
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    
    // Stream Proof: Скрываем оверлей от захвата OBS/Discord (по умолчанию выключено)
    if (config::g_antiCaptureEnabled) {
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    }
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    return hwnd;
}

void DestroyOverlayWindow()
{
    if (g_hOverlayWnd) {
        DestroyWindow(g_hOverlayWnd);
        g_hOverlayWnd = nullptr;
    }
    UnregisterClassW(g_wndClassName, GetModuleHandle(nullptr));
}

HWND GetOverlayWindow()
{
    return g_hOverlayWnd;
}

void SetOverlayInputMode(HWND hWnd, bool interactive)
{
    static bool wasInteractive = false;
    if (interactive == wasInteractive) return;
    wasInteractive = interactive;

    LONG exStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
    if (interactive)
    {
        // Меню открыто: окно принимает ввод
        exStyle &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
        SetWindowLongW(hWnd, GWL_EXSTYLE, exStyle);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
    }
    else
    {
        // Меню закрыто: окно пропускает клики
        exStyle |= (WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
        SetWindowLongW(hWnd, GWL_EXSTYLE, exStyle);
        
        // Принудительно возвращаем фокус игре
        HWND hGame = FindWindowW(L"SDL_app", L"Counter-Strike 2"); // Класс окна CS2
        if (hGame)
        {
            SetForegroundWindow(hGame);
            SetActiveWindow(hGame);
        }
    }
}

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Если меню открыто, сначала даем ImGui обработать сообщение
    if (config::g_menuOpen)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return 1;
    }

    switch (msg)
    {
    case WM_CREATE:
    {
        MARGINS margin = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(hWnd, &margin);
        return 0;
    }
    case WM_NCHITTEST:
    {
        // Если меню открыто — это обычное окно (клики работают).
        // Если закрыто — окно прозрачно для кликов (пропускает в игру).
        if (config::g_menuOpen) {
            // Важно: возвращаем HTCLIENT, чтобы ImGui получал события мыши
            return HTCLIENT;
        }
        return HTTRANSPARENT;
    }
    case WM_MOUSEACTIVATE:
        // При открытом меню разрешаем активацию окна
        return config::g_menuOpen ? MA_ACTIVATE : MA_NOACTIVATE;

    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        // Size handling is done in main loop to prevent circular dependencies
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace core
