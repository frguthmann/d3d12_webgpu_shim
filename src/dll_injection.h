#pragma once

#include <Windows.h>

bool InjectDllIntoProcess(HANDLE process, const char* dllPath, bool logErrors);
