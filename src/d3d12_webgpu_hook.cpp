#include "d3d12_webgpu_shim_classes.h"
#include <detours.h>

// ============================================================================
// Real function pointers (filled by Detours)
// ============================================================================
static PFN_D3D12_CREATE_DEVICE Real_D3D12CreateDevice = nullptr;
static PFN_D3D12_GET_DEBUG_INTERFACE Real_D3D12GetDebugInterface = nullptr;
static PFN_D3D12_SERIALIZE_ROOT_SIGNATURE Real_D3D12SerializeRootSignature = nullptr;
static PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER Real_D3D12CreateRootSignatureDeserializer = nullptr;
static PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE Real_D3D12SerializeVersionedRootSignature = nullptr;
static PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER Real_D3D12CreateVersionedRootSignatureDeserializer = nullptr;
static PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES Real_D3D12EnableExperimentalFeatures = nullptr;

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

static HRESULT WINAPI Hook_D3D12GetDebugInterface(REFIID riid, void** ppvDebug)
{
    return Real_D3D12GetDebugInterface(riid, ppvDebug);
}

static HRESULT WINAPI Hook_D3D12SerializeRootSignature(
    const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
    D3D_ROOT_SIGNATURE_VERSION Version,
    ID3DBlob** ppBlob,
    ID3DBlob** ppErrorBlob)
{
    return Real_D3D12SerializeRootSignature(pRootSignature, Version, ppBlob, ppErrorBlob);
}

static HRESULT WINAPI Hook_D3D12CreateRootSignatureDeserializer(
    LPCVOID pSrcData, SIZE_T SrcDataSizeInBytes,
    REFIID pRootSignatureDeserializerInterface, void** ppRootSignatureDeserializer)
{
    return Real_D3D12CreateRootSignatureDeserializer(pSrcData, SrcDataSizeInBytes, pRootSignatureDeserializerInterface, ppRootSignatureDeserializer);
}

static HRESULT WINAPI Hook_D3D12SerializeVersionedRootSignature(
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
    ID3DBlob** ppBlob, ID3DBlob** ppErrorBlob)
{
    return Real_D3D12SerializeVersionedRootSignature(pRootSignature, ppBlob, ppErrorBlob);
}

static HRESULT WINAPI Hook_D3D12CreateVersionedRootSignatureDeserializer(
    LPCVOID pSrcData, SIZE_T SrcDataSizeInBytes,
    REFIID pRootSignatureDeserializerInterface, void** ppRootSignatureDeserializer)
{
    return Real_D3D12CreateVersionedRootSignatureDeserializer(pSrcData, SrcDataSizeInBytes, pRootSignatureDeserializerInterface, ppRootSignatureDeserializer);
}

static HRESULT WINAPI Hook_D3D12EnableExperimentalFeatures(
    UINT NumFeatures, const IID* pIIDs, void* pConfigurationStructs, UINT* pConfigurationStructSizes)
{
    return Real_D3D12EnableExperimentalFeatures(NumFeatures, pIIDs, pConfigurationStructs, pConfigurationStructSizes);
}

// ============================================================================
// CreateProcessW hook — propagate DLL into Chrome's GPU child process
// ============================================================================

// Store our own DLL path for injection into child processes
static char g_hookDllPath[MAX_PATH] = {};
static HINSTANCE g_hInstance = NULL;

typedef BOOL(WINAPI* PFN_CREATE_PROCESS_W)(
    LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles, DWORD dwCreationFlags,
    LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

static PFN_CREATE_PROCESS_W Real_CreateProcessW = nullptr;

static void InjectIntoProcess(HANDLE hProcess)
{
    SIZE_T pathLen = strlen(g_hookDllPath) + 1;

    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) return;

    if (!WriteProcessMemory(hProcess, remoteMem, g_hookDllPath, pathLen, NULL))
    {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return;
    }

    FARPROC loadLibAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibAddr)
    {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, NULL);
    if (hThread)
    {
        WaitForSingleObject(hThread, 10000);
        CloseHandle(hThread);
    }

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
}

static bool CommandLineContainsGpuType(LPCWSTR cmdLine)
{
    if (!cmdLine) return false;
    // Look for --type=gpu in the command line (identifies the GPU process)
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
        InjectIntoProcess(lpProcessInformation->hProcess);

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
    Real_D3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(
        GetProcAddress(hD3D12, "D3D12GetDebugInterface"));
    Real_D3D12SerializeRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(
        GetProcAddress(hD3D12, "D3D12SerializeRootSignature"));
    Real_D3D12CreateRootSignatureDeserializer = reinterpret_cast<PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER>(
        GetProcAddress(hD3D12, "D3D12CreateRootSignatureDeserializer"));
    Real_D3D12SerializeVersionedRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>(
        GetProcAddress(hD3D12, "D3D12SerializeVersionedRootSignature"));
    Real_D3D12CreateVersionedRootSignatureDeserializer = reinterpret_cast<PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER>(
        GetProcAddress(hD3D12, "D3D12CreateVersionedRootSignatureDeserializer"));
    Real_D3D12EnableExperimentalFeatures = reinterpret_cast<PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES>(
        GetProcAddress(hD3D12, "D3D12EnableExperimentalFeatures"));

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

        if (Real_D3D12GetDebugInterface)
            DetourAttach(&(PVOID&)Real_D3D12GetDebugInterface, Hook_D3D12GetDebugInterface);
        if (Real_D3D12SerializeRootSignature)
            DetourAttach(&(PVOID&)Real_D3D12SerializeRootSignature, Hook_D3D12SerializeRootSignature);
        if (Real_D3D12CreateRootSignatureDeserializer)
            DetourAttach(&(PVOID&)Real_D3D12CreateRootSignatureDeserializer, Hook_D3D12CreateRootSignatureDeserializer);
        if (Real_D3D12SerializeVersionedRootSignature)
            DetourAttach(&(PVOID&)Real_D3D12SerializeVersionedRootSignature, Hook_D3D12SerializeVersionedRootSignature);
        if (Real_D3D12CreateVersionedRootSignatureDeserializer)
            DetourAttach(&(PVOID&)Real_D3D12CreateVersionedRootSignatureDeserializer, Hook_D3D12CreateVersionedRootSignatureDeserializer);
        if (Real_D3D12EnableExperimentalFeatures)
            DetourAttach(&(PVOID&)Real_D3D12EnableExperimentalFeatures, Hook_D3D12EnableExperimentalFeatures);
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
    if (Real_D3D12GetDebugInterface)
        DetourDetach(&(PVOID&)Real_D3D12GetDebugInterface, Hook_D3D12GetDebugInterface);
    if (Real_D3D12SerializeRootSignature)
        DetourDetach(&(PVOID&)Real_D3D12SerializeRootSignature, Hook_D3D12SerializeRootSignature);
    if (Real_D3D12CreateRootSignatureDeserializer)
        DetourDetach(&(PVOID&)Real_D3D12CreateRootSignatureDeserializer, Hook_D3D12CreateRootSignatureDeserializer);
    if (Real_D3D12SerializeVersionedRootSignature)
        DetourDetach(&(PVOID&)Real_D3D12SerializeVersionedRootSignature, Hook_D3D12SerializeVersionedRootSignature);
    if (Real_D3D12CreateVersionedRootSignatureDeserializer)
        DetourDetach(&(PVOID&)Real_D3D12CreateVersionedRootSignatureDeserializer, Hook_D3D12CreateVersionedRootSignatureDeserializer);
    if (Real_D3D12EnableExperimentalFeatures)
        DetourDetach(&(PVOID&)Real_D3D12EnableExperimentalFeatures, Hook_D3D12EnableExperimentalFeatures);

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
        g_hInstance = hinstDLL;
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
