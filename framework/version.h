#pragma once

#ifdef _VERSION
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern HMODULE version_dll;
DWORD WINAPI Load(LPVOID lpParam);
void FreeVersionLibrary();
#endif