#include "dll_injection.h"

#include <stdio.h>
#include <string.h>

static void LogLastError(bool enabled, const char* message)
{
    if (enabled)
    {
        fprintf(stderr, "ERROR: %s (%lu)\n", message, GetLastError());
    }
}

bool InjectDllIntoProcess(HANDLE process, const char* dllPath, bool logErrors)
{
    SIZE_T pathLen = strlen(dllPath) + 1;

    // Allocate memory in target process for the DLL path.
    LPVOID remoteMem = VirtualAllocEx(process, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem)
    {
        LogLastError(logErrors, "VirtualAllocEx failed");
        return false;
    }

    // Write DLL path into target process.
    if (!WriteProcessMemory(process, remoteMem, dllPath, pathLen, NULL))
    {
        LogLastError(logErrors, "WriteProcessMemory failed");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    FARPROC loadLibAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibAddr)
    {
        LogLastError(logErrors, "Could not find LoadLibraryA");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, NULL);
    if (!thread)
    {
        LogLastError(logErrors, "CreateRemoteThread failed");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    // Wait for LoadLibrary to complete before resuming the target process.
    DWORD waitResult = WaitForSingleObject(thread, 10000);
    if (waitResult != WAIT_OBJECT_0)
    {
        LogLastError(logErrors, "LoadLibrary remote thread did not finish");
        CloseHandle(thread);
        return false;
    }

    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);
    CloseHandle(thread);
    VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);

    if (exitCode == 0)
    {
        if (logErrors)
        {
            fprintf(stderr, "ERROR: LoadLibrary returned NULL in target process. DLL injection failed.\n");
        }
        return false;
    }

    return true;
}
