#include "pch-il2cpp.h"

#ifdef _VERSION 

#include "main.h"
#include "version.h"
#include <chrono>
#include <filesystem>
#include <thread>
#include <string>

HMODULE version_dll = nullptr;

#define WRAPPER_GENFUNC(name) \
      FARPROC o##name; \
      void _##name() { if (o##name) ((void(*)())o##name)(); }

WRAPPER_GENFUNC(GetFileVersionInfoA)
WRAPPER_GENFUNC(GetFileVersionInfoByHandle)
WRAPPER_GENFUNC(GetFileVersionInfoExW)
WRAPPER_GENFUNC(GetFileVersionInfoExA)
WRAPPER_GENFUNC(GetFileVersionInfoSizeA)
WRAPPER_GENFUNC(GetFileVersionInfoSizeExA)
WRAPPER_GENFUNC(GetFileVersionInfoSizeExW)
WRAPPER_GENFUNC(GetFileVersionInfoSizeW)
WRAPPER_GENFUNC(GetFileVersionInfoW)
WRAPPER_GENFUNC(VerFindFileA)
WRAPPER_GENFUNC(VerFindFileW)
WRAPPER_GENFUNC(VerInstallFileA)
WRAPPER_GENFUNC(VerInstallFileW)
WRAPPER_GENFUNC(VerLanguageNameA)
WRAPPER_GENFUNC(VerLanguageNameW)
WRAPPER_GENFUNC(VerQueryValueA)
WRAPPER_GENFUNC(VerQueryValueW)

#define WRAPPER_FUNC(name) o##name = GetProcAddress(version_dll, #name);

void load_version_lib() {
 char systemPath[MAX_PATH];
 GetSystemDirectoryA(systemPath, MAX_PATH);
 strcat_s(systemPath, "\\version.dll");
 version_dll = LoadLibraryA(systemPath);

 if (!version_dll) {
  MessageBoxA(NULL, " version.dll not found!!", "Error", MB_OK | MB_ICONERROR);
  return;
 }

 WRAPPER_FUNC(GetFileVersionInfoA);
 WRAPPER_FUNC(GetFileVersionInfoByHandle);
 WRAPPER_FUNC(GetFileVersionInfoExW);
 WRAPPER_FUNC(GetFileVersionInfoExA);
 WRAPPER_FUNC(GetFileVersionInfoSizeA);
 WRAPPER_FUNC(GetFileVersionInfoSizeExA);
 WRAPPER_FUNC(GetFileVersionInfoSizeExW);
 WRAPPER_FUNC(GetFileVersionInfoSizeW);
 WRAPPER_FUNC(GetFileVersionInfoW);
 WRAPPER_FUNC(VerFindFileA);
 WRAPPER_FUNC(VerFindFileW);
 WRAPPER_FUNC(VerInstallFileA);
 WRAPPER_FUNC(VerInstallFileW);
 WRAPPER_FUNC(VerLanguageNameA);
 WRAPPER_FUNC(VerLanguageNameW);
 WRAPPER_FUNC(VerQueryValueA);
 WRAPPER_FUNC(VerQueryValueW);
}

void FreeVersionLibrary() {
 if (version_dll) {
  FreeLibrary(version_dll);
  version_dll = nullptr;
 }
}

DWORD WINAPI Load(LPVOID lpParam) {
 const char* dependencies[] = {
  "d3d11.dll", "dxgi.dll", "vcruntime140.dll"
 };

 for (const char* dep : dependencies) {
  HMODULE mod = LoadLibraryA(dep);
  if (mod) {
   FreeLibrary(mod);
  }
  else {
   std::string msg = "Missing Library: " + std::string(dep);
   MessageBoxA(NULL, msg.c_str(), "Dependency Error", MB_OK | MB_ICONERROR);
   return 0; // Exit if there is an error
  }
 }

 load_version_lib();
 if (!version_dll) return 0;

 std::this_thread::sleep_for(std::chrono::seconds(6));

 Run(lpParam);

 return 0;
}

#endif 