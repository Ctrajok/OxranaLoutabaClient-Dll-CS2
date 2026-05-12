#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <Windows.h>

namespace core {

// D3D11 device management
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

// D3D11 object getters
ID3D11Device* GetD3DDevice();
ID3D11DeviceContext* GetD3DDeviceContext();
IDXGISwapChain1* GetSwapChain();
ID3D11RenderTargetView* GetRenderTargetView();

} // namespace core
