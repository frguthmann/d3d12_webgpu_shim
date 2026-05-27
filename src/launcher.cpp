#include "dll_injection.h"

#include <Windows.h>
#include <stdio.h>
#include <string>

// Usage: webgpu_profiling_launcher.exe <chrome.exe path> [additional chrome args...]
//
// Injection method: CreateRemoteThread + LoadLibrary
// 1. Create Chrome process SUSPENDED
// 2. Inject hook DLL via remote thread calling LoadLibraryA
// 3. Resume Chrome's main thread

int main(int argc, char* argv[])
{
    std::string chromePath;
    std::string extraArgs;

    if (argc >= 2)
    {
        chromePath = argv[1];
        for (int i = 2; i < argc; i++)
        {
            extraArgs += " ";
            extraArgs += argv[i];
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Could not find chrome.exe. Pass the path as the first argument.\n");
        return 1;
    }

    // Verify chrome exists
    if (GetFileAttributesA(chromePath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        fprintf(stderr, "ERROR: Chrome not found at: %s\n", chromePath.c_str());
        return 1;
    }

    // Find our hook DLL
    std::string hookDll = GetHookDllPath();
    if (GetFileAttributesA(hookDll.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        fprintf(stderr, "ERROR: Hook DLL not found at: %s\n", hookDll.c_str());
        return 1;
    }

    // Build command line with recommended flags for WebGPU profiling
    std::string cmdLine = "\"" + chromePath + "\"" + extraArgs;

    printf("Launching Chrome with WebGPU profiling hook...\n");
    printf("  Chrome: %s\n", chromePath.c_str());
    printf("  Hook:   %s\n", hookDll.c_str());
    printf("  Args:   %s\n\n", cmdLine.c_str());

    // Create Chrome process SUSPENDED so we can inject before it runs
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL created = CreateProcessA(
        chromePath.c_str(),
        (LPSTR)cmdLine.c_str(),
        NULL, NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL, NULL,
        &si, &pi);

    if (!created)
    {
        fprintf(stderr, "ERROR: CreateProcess failed (%lu)\n", GetLastError());
        return 1;
    }

    printf("Chrome process created suspended (PID %lu). Injecting hook...\n", pi.dwProcessId);

    // Inject our hook DLL
    if (!InjectDllIntoProcess(pi.hProcess, hookDll.c_str(), true))
    {
        fprintf(stderr, "Injection failed. Terminating Chrome process.\n");
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }

    printf("Hook DLL injected. Resuming Chrome...\n");

    // Resume Chrome's main thread - it will now run with our hooks active
    ResumeThread(pi.hThread);

    printf("Chrome running (PID %lu). You can now use your GPU profiler.\n", pi.dwProcessId);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;
}
