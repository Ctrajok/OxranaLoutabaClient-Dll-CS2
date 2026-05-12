#pragma once

#include <Windows.h>

namespace core {

// Overlay window management
HWND CreateOverlayWindow();
void DestroyOverlayWindow();
HWND GetOverlayWindow();

// Window input mode
void SetOverlayInputMode(HWND hWnd, bool interactive);

// Window procedure
LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

} // namespace core
