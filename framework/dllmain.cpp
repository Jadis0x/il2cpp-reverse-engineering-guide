// dllmain.cpp
#include "pch-il2cpp.h"
#include <windows.h>
#include "main.h"

#ifdef _VERSION
#include "version.h"
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // If _VERSION is defined, call the Proxy Load function
#ifdef _VERSION
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Load, hModule, NULL, NULL);
#else
         // If not defined, directly call our own Run function (Injector Mode)
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Run, hModule, NULL, NULL);
#endif
        break;

    case DLL_PROCESS_DETACH:
#ifdef _VERSION
        FreeVersionLibrary(); // Clean
#endif
        break;
    }
    return TRUE;
}