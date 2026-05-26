#include "dll_injection.h"
#include "d3d12_webgpu_shim_classes.h"

#include <detours.h>

// ============================================================================
// Real function pointers (filled by Detours)
// ============================================================================
static PFN_D3D12_CREATE_DEVICE Real_D3D12CreateDevice = nullptr;

PFN_CREATE_DXGI_FACTORY g_DXGICreateFactory = nullptr;

// ============================================================================
// Hooked D3D12 functions
// ============================================================================

static HRESULT WINAPI Hook_D3D12CreateDevice(
    IUnknown* pAdapter,
    D3D_FEATURE_LEVEL MinimumFeatureLevel,
    REFIID riid,
    void** ppDevice)
{
    HRESULT res = Real_D3D12CreateDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);
    if (res == S_OK && ppDevice && *ppDevice)
    {
        ID3D12Device_webgpu_shim* device = new ID3D12Device_webgpu_shim(reinterpret_cast<IUnknown*>(*ppDevice));
        *ppDevice = device;
    }
    return res;
}

// ============================================================================
// CreateProcessW hook — propagate DLL into Chrome's GPU child process
// ============================================================================

// Store our own DLL path for injection into child processes
static char g_hookDllPath[MAX_PATH] = {};

typedef BOOL(WINAPI* PFN_CREATE_PROCESS_W)(
    LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles, DWORD dwCreationFlags,
    LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

static PFN_CREATE_PROCESS_W Real_CreateProcessW = nullptr;

static bool CommandLineContainsGpuType(LPCWSTR cmdLine)
{
    if (!cmdLine) return false;
    // Chrome has used variants such as --type=gpu and --type=gpu-process.
    return wcsstr(cmdLine, L"--type=gpu") != nullptr;
}

static BOOL WINAPI Hook_CreateProcessW(
    LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles, DWORD dwCreationFlags,
    LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    bool isGpuProcess = CommandLineContainsGpuType(lpCommandLine);

    // If this is the GPU process, force CREATE_SUSPENDED so we can inject
    DWORD flags = dwCreationFlags;
    if (isGpuProcess)
    {
        flags |= CREATE_SUSPENDED;
    }

    BOOL result = Real_CreateProcessW(
        lpApplicationName, lpCommandLine,
        lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, flags,
        lpEnvironment, lpCurrentDirectory,
        lpStartupInfo, lpProcessInformation);

    if (result && isGpuProcess)
    {
        // Inject our hook DLL into the GPU process
        InjectDllIntoProcess(lpProcessInformation->hProcess, g_hookDllPath, false);

        // If the caller didn't originally request SUSPENDED, resume now
        if (!(dwCreationFlags & CREATE_SUSPENDED))
        {
            ResumeThread(lpProcessInformation->hThread);
        }
    }

    return result;
}

// ============================================================================
// Resolve real function addresses from the loaded d3d12.dll
// ============================================================================

static bool ResolveD3D12Functions()
{
    // Get the real d3d12.dll - it may already be loaded, or we load from System32
    HMODULE hD3D12 = GetModuleHandleA("d3d12.dll");
    if (!hD3D12)
    {
        char systemDir[MAX_PATH];
        GetSystemDirectoryA(systemDir, MAX_PATH);
        std::string path = std::string(systemDir) + "\\d3d12.dll";
        hD3D12 = LoadLibraryA(path.c_str());
    }
    if (!hD3D12) return false;

    Real_D3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(
        GetProcAddress(hD3D12, "D3D12CreateDevice"));

    // Also resolve DXGI
    HMODULE hDXGI = GetModuleHandleA("dxgi.dll");
    if (!hDXGI)
    {
        char systemDir[MAX_PATH];
        GetSystemDirectoryA(systemDir, MAX_PATH);
        std::string path = std::string(systemDir) + "\\dxgi.dll";
        hDXGI = LoadLibraryA(path.c_str());
    }
    if (!hDXGI) return false;

    g_DXGICreateFactory = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(
        GetProcAddress(hDXGI, "CreateDXGIFactory"));

    return (Real_D3D12CreateDevice != nullptr);
}

// ============================================================================
// Attach/Detach hooks
// ============================================================================

static LONG AttachHooks()
{
    // Always hook CreateProcessW to propagate into child processes (GPU process)
    Real_CreateProcessW = reinterpret_cast<PFN_CREATE_PROCESS_W>(
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateProcessW"));

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (Real_CreateProcessW)
        DetourAttach(&(PVOID&)Real_CreateProcessW, Hook_CreateProcessW);

    // Try to hook D3D12 functions — this will succeed in the GPU process
    // where d3d12.dll is loaded, and gracefully skip in the browser process
    if (ResolveD3D12Functions())
    {
        DetourAttach(&(PVOID&)Real_D3D12CreateDevice, Hook_D3D12CreateDevice);
    }

    return DetourTransactionCommit();
}

static LONG DetachHooks()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (Real_CreateProcessW)
        DetourDetach(&(PVOID&)Real_CreateProcessW, Hook_CreateProcessW);

    if (Real_D3D12CreateDevice)
        DetourDetach(&(PVOID&)Real_D3D12CreateDevice, Hook_D3D12CreateDevice);

    return DetourTransactionCommit();
}

// ============================================================================
// DLL entry point
// ============================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();
        DisableThreadLibraryCalls(hinstDLL);
        // Store our DLL path so we can inject it into child processes
        GetModuleFileNameA(hinstDLL, g_hookDllPath, MAX_PATH);
        AttachHooks();
        break;

    case DLL_PROCESS_DETACH:
        DetachHooks();
        break;
    }
    return TRUE;
}
