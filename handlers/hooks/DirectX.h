#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <imgui/imgui.h>

using D3D_PRESENT_FUNCTION = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);

extern D3D_PRESENT_FUNCTION oPresent;
extern HRESULT __stdcall dPresent(IDXGISwapChain* __this, UINT SyncInterval, UINT Flags);

struct MouseStateCache {
    bool wasVisible;
    int wasLockState; // 0: None, 1: Locked, 2: Confined
    bool hasCached = false;
};

namespace DirectX {
    extern HWND window;
    extern HANDLE hRenderSemaphore;
    extern ID3D11Device* pDevice;
    extern ID3D11DeviceContext* pContext;

    void Shutdown();
    void CacheCurrentMouseState();
    void ApplyMouseState(bool visible, int lockState);
}