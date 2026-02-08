#include "pch-il2cpp.h"

#include "DirectX.h"
#include "gui/Menu.h"
#include "gui/GUITheme.h"
#include "settings.h"
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include <mutex>
#include <atomic>
#include <Il2CppResolver.h>

D3D_PRESENT_FUNCTION oPresent = nullptr;
HWND DirectX::window = nullptr;
HANDLE DirectX::hRenderSemaphore = nullptr;
ID3D11Device* DirectX::pDevice = nullptr;
ID3D11DeviceContext* DirectX::pContext = nullptr;
static ID3D11RenderTargetView* pRenderTargetView = nullptr;
static WNDPROC oWndProc = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static MouseStateCache mouseCache;

LRESULT __stdcall dWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) {
		if (wParam == settings.KeyBinds.Toggle_Menu) {
			settings.bShowMenu = !settings.bShowMenu;
		}
	}

	if (settings.bShowMenu) {
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

		switch (uMsg) {
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEWHEEL:
			return true;
		}

		if (ImGui::GetIO().WantCaptureKeyboard) return true;
	}

	if (uMsg == WM_SIZE && pRenderTargetView) {
		pRenderTargetView->Release();
		pRenderTargetView = nullptr;
	}
	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall dPresent(IDXGISwapChain* __this, UINT SyncInterval, UINT Flags) {
	static std::once_flag init_flag;
	std::call_once(init_flag, [&]() {
		__this->GetDevice(__uuidof(ID3D11Device), (void**)&DirectX::pDevice);
		DirectX::pDevice->GetImmediateContext(&DirectX::pContext);
		DXGI_SWAP_CHAIN_DESC sd; __this->GetDesc(&sd);
		DirectX::window = sd.OutputWindow;

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(DirectX::window);
		ImGui_ImplDX11_Init(DirectX::pDevice, DirectX::pContext);

		ApplyTheme();
		oWndProc = (WNDPROC)SetWindowLongPtr(DirectX::window, GWLP_WNDPROC, (LONG_PTR)dWndProc);
		DirectX::hRenderSemaphore = CreateSemaphore(nullptr, 1, 1, nullptr);
		settings.ImGuiInitialized = true;
		});

	if (WaitForSingleObject(DirectX::hRenderSemaphore, 0) == WAIT_OBJECT_0) {
		if (!pRenderTargetView) {
			ID3D11Texture2D* pBackBuffer = nullptr;
			__this->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
			DirectX::pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
			pBackBuffer->Release();
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static bool wasMenuOpen = false;

		if (wasMenuOpen != settings.bShowMenu) {

			if (!wasMenuOpen && settings.bShowMenu) {
				DirectX::CacheCurrentMouseState();
				DirectX::ApplyMouseState(true, 0);
			}

			else if (wasMenuOpen && !settings.bShowMenu) {
				if (mouseCache.hasCached) {
					DirectX::ApplyMouseState(mouseCache.wasVisible, mouseCache.wasLockState);
				}
			}

			wasMenuOpen = settings.bShowMenu;
		}

		if (settings.bShowMenu) {
			DirectX::ApplyMouseState(true, 0);
			ImGui::GetIO().MouseDrawCursor = true;
		}
		else {
			ImGui::GetIO().MouseDrawCursor = false;
		}

		if (settings.bShowMenu) {
			Menu::Render();
		}

		ImGui::Render();
		DirectX::pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		ReleaseSemaphore(DirectX::hRenderSemaphore, 1, nullptr);
	}
	return oPresent(__this, SyncInterval, Flags);
}

void DirectX::Shutdown() {
	if (DirectX::hRenderSemaphore) {
		WaitForSingleObject(DirectX::hRenderSemaphore, INFINITE);
		SetWindowLongPtr(DirectX::window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		if (pRenderTargetView) pRenderTargetView->Release();
		CloseHandle(DirectX::hRenderSemaphore);
	}
}

void DirectX::CacheCurrentMouseState()
{
	Resolver::Protection::safe_call([&]() {
		Il2CppClass* cursorClass = Resolver::FindClass("UnityEngine", "Cursor");
		if (!cursorClass) return;

		const MethodInfo* getVis = il2cpp_class_get_method_from_name(cursorClass, "get_visible", 0);
		const MethodInfo* getLock = il2cpp_class_get_method_from_name(cursorClass, "get_lockState", 0);

		if (getVis && getLock) {
			Il2CppObject* visObj = il2cpp_runtime_invoke(getVis, nullptr, nullptr, nullptr);
			Il2CppObject* lockObj = il2cpp_runtime_invoke(getLock, nullptr, nullptr, nullptr);

			if (visObj) mouseCache.wasVisible = *static_cast<bool*>(il2cpp_object_unbox(visObj));
			if (lockObj) mouseCache.wasLockState = *static_cast<int*>(il2cpp_object_unbox(lockObj));

			mouseCache.hasCached = true;
		}
		});
}

void DirectX::ApplyMouseState(bool visible, int lockState)
{
	Resolver::Protection::safe_call([&]() {
		Il2CppClass* cursorClass = Resolver::FindClass("UnityEngine", "Cursor");
		if (!cursorClass) return;

		const MethodInfo* setVis = il2cpp_class_get_method_from_name(cursorClass, "set_visible", 1);

		const MethodInfo* setLock = il2cpp_class_get_method_from_name(cursorClass, "set_lockState", 1);

		if (setVis && setLock) {
			void* pVis[] = { &visible };
			void* pLock[] = { &lockState };

			il2cpp_runtime_invoke(setLock, nullptr, pLock, nullptr);
			il2cpp_runtime_invoke(setVis, nullptr, pVis, nullptr);
		}
		});
}

