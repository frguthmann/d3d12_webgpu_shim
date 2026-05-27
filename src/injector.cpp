#include "dll_injection.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <string>
#include <vector>

// Usage:
//   injector.exe <pid>            -- inject into a specific process by PID
//   injector.exe <process_name>   -- inject into all processes matching the name (e.g. chrome.exe)
//
// Intended for use with Nsight: let Nsight launch Chrome normally, then run
// this tool to inject the hook DLL into the running Chrome process(es).
// The hook installs a CreateProcessW detour that automatically propagates
// the DLL into any GPU child process Chrome spawns afterwards.

static bool InjectIntoProcessById(DWORD pid, const std::string& dllPath)
{
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (!hProcess)
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
            fprintf(stderr, "  ERROR: OpenProcess(%lu) failed — access denied. Run as Administrator.\n", pid);
        else
            fprintf(stderr, "  ERROR: OpenProcess(%lu) failed (%lu)\n", pid, err);
        return false;
    }

    bool ok = InjectDllIntoProcess(hProcess, dllPath.c_str(), true);
    CloseHandle(hProcess);
    return ok;
}

// Returns all PIDs whose image name matches the given name (case-insensitive).
static std::vector<DWORD> FindProcessesByName(const char* name)
{
    std::vector<DWORD> pids;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return pids;

    PROCESSENTRY32 entry = {};
    entry.dwSize = sizeof(entry);

    if (Process32First(snap, &entry))
    {
        do
        {
            if (_stricmp(entry.szExeFile, name) == 0)
                pids.push_back(entry.th32ProcessID);
        } while (Process32Next(snap, &entry));
    }

    CloseHandle(snap);
    return pids;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  injector.exe <pid>           -- inject into process by PID\n");
        fprintf(stderr, "  injector.exe <process.exe>   -- inject into all matching processes\n");
        return 1;
    }

    std::string hookDll = GetHookDllPath();
    if (GetFileAttributesA(hookDll.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        fprintf(stderr, "ERROR: Hook DLL not found at: %s\n", hookDll.c_str());
        return 1;
    }

    printf("Hook DLL: %s\n\n", hookDll.c_str());

    // Determine if the argument is a PID (all digits) or a process name.
    const char* arg = argv[1];
    bool isNumeric = (arg[0] != '\0');
    for (int i = 0; arg[i] != '\0'; i++)
    {
        if (arg[i] < '0' || arg[i] > '9') { isNumeric = false; break; }
    }

    std::vector<DWORD> pids;

    if (isNumeric)
    {
        DWORD pid = (DWORD)strtoul(arg, nullptr, 10);
        pids.push_back(pid);
    }
    else
    {
        pids = FindProcessesByName(arg);
        if (pids.empty())
        {
            fprintf(stderr, "ERROR: No processes found matching '%s'\n", arg);
            return 1;
        }
        printf("Found %zu process(es) matching '%s':\n", pids.size(), arg);
    }

    int successCount = 0;
    for (DWORD pid : pids)
    {
        printf("  Injecting into PID %lu... ", pid);
        fflush(stdout);
        if (InjectIntoProcessById(pid, hookDll))
        {
            printf("OK\n");
            successCount++;
        }
        else
        {
            printf("FAILED\n");
        }
    }

    printf("\nDone: %d/%zu succeeded.\n", successCount, pids.size());
    return (successCount > 0) ? 0 : 1;
}
