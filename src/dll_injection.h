#pragma once

#include <Windows.h>
#include <string>

bool InjectDllIntoProcess(HANDLE process, const char* dllPath, bool logErrors);

inline std::string GetHookDllPath()
{
    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);

    std::string dir(modulePath);
    size_t lastSlash = dir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        dir = dir.substr(0, lastSlash + 1);

    return dir + "d3d12_webgpu_hook.dll";
}
