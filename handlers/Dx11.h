#pragma once
#include <d3d11.h>
#include <dxgi.h>

class dx11api {
public:
    using D3D_PRESENT_FUNCTION = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    D3D_PRESENT_FUNCTION presentFunction;

    dx11api();
};