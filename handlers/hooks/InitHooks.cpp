#include "pch-il2cpp.h"
#include <Windows.h>
#include <iostream>
#include "detours/detours.h"
#include "InitHooks.h"
#include "DirectX.h"
#include "Dx11.h"    
#include "helpers.h"

bool HookFunction(PVOID* ppPointer, PVOID pDetour, const char* functionName) {
    if (const auto error = DetourAttach(ppPointer, pDetour); error != NO_ERROR) {
        std::cout << "[ERROR]: Failed to hook " << functionName << ", error " << error << std::endl;
        return false;
    }
    std::cout << "[HOOKED]: " << functionName << std::endl;
    return true;
}

bool UnhookFunction(PVOID* ppPointer, PVOID pDetour, const char* functionName) {
    if (const auto error = DetourDetach(ppPointer, pDetour); error != NO_ERROR) {
        std::cout << "[ERROR]: Failed to unhook " << functionName << ", error " << error << std::endl;
        return false;
    }
    std::cout << "[UNHOOKED]: " << functionName << std::endl;
    return true;
}

void DetourInitilization() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    dx11api d3d11 = dx11api();
    if (!d3d11.presentFunction) {
        std::cout << "[ERROR]: Unable to retrieve IDXGISwapChain::Present method" << std::endl;
        return;
    }

    oPresent = d3d11.presentFunction;

    if (!oPresent) {
        std::cout << "[ERROR]: oPresent is null!" << std::endl;
        return;
    }

    std::cout << "[INFO]: Attempting to hook oPresent at address: " << oPresent << std::endl;

    if (!HookFunction(&(PVOID&)oPresent, dPresent, "D3D_PRESENT_FUNCTION")) {
        DetourTransactionAbort();
        return;
    }

    if (!HookFunction(&(PVOID&)app::Debug_2_Log, dDebug_Log, "Debug_2_Log"))
    {
        DetourTransactionAbort();
        return;
    }
    if (!HookFunction(&(PVOID&)app::Debug_2_LogWarning, dDebug_LogWarning, "Debug_2_Warning"))
    {
        DetourTransactionAbort();
        return;
    }


    DetourTransactionCommit();
}

void DetourUninitialization() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (DetourDetach(&(PVOID&)oPresent, dPresent) != 0) {
        DetourTransactionAbort();
        return;
    }

    if (DetourTransactionCommit() == NO_ERROR) {
        DirectX::Shutdown();
    }

    if (DetourDetach(&(PVOID&)app::Debug_2_Log, dDebug_Log) != 0) return;
    if (DetourDetach(&(PVOID&)app::Debug_2_LogWarning, dDebug_LogWarning) != 0) return;
}

void dDebug_Log(app::Object* message, MethodInfo* method)
{
    std::cout << "[Log]: " << ToString(message) << "\n";

    app::Debug_2_Log(message, method);
}


void dDebug_LogWarning(app::Object* message, MethodInfo* method)
{
    std::cout << "[Log]: " << ToString(message) << "\n";

    app::Debug_2_LogWarning(message, method);
}
