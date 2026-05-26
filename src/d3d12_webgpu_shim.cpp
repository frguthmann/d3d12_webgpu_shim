#include "d3d12_webgpu_shim_classes.h"

// ============================================================================
// Function pointers (loaded from the real d3d12.dll)
// ============================================================================

PFN_D3D12_CREATE_DEVICE g_D3D12CreateDevice;
PFN_D3D12_GET_DEBUG_INTERFACE g_D3D12GetDebugInterface;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE g_D3D12SerializeRootSignature;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER g_D3D12CreateRootSignatureDeserializer;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE g_D3D12SerializeVersionedRootSignature;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER g_D3D12CreateVersionedRootSignatureDeserializer;
PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES g_D3D12EnableExperimentalFeatures;
PFN_CREATE_DXGI_FACTORY g_DXGICreateFactory;

// ============================================================================
// Exported shim functions
// ============================================================================

HRESULT WINAPI D3D12CreateDevice_webgpu_shim(
    _In_opt_ IUnknown* pAdapter,
    D3D_FEATURE_LEVEL MinimumFeatureLevel,
    _In_ REFIID riid,
    _COM_Outptr_opt_ void** ppDevice)
{
    HRESULT res = g_D3D12CreateDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);

    if (res == S_OK)
    {
        ID3D12Device_webgpu_shim* device = new ID3D12Device_webgpu_shim(reinterpret_cast<IUnknown*>(*ppDevice));
        *ppDevice = device;
    }

    return res;
}

HRESULT WINAPI D3D12GetDebugInterface_webgpu_shim(_In_ REFIID riid, _COM_Outptr_opt_ void** ppvDebug)
{
    return g_D3D12GetDebugInterface(riid, ppvDebug);
}

HRESULT WINAPI D3D12SerializeRootSignature_webgpu_shim(
    _In_ const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
    _In_ D3D_ROOT_SIGNATURE_VERSION Version,
    _Out_ ID3DBlob** ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob)
{
    return g_D3D12SerializeRootSignature(pRootSignature, Version, ppBlob, ppErrorBlob);
}

HRESULT WINAPI D3D12CreateRootSignatureDeserializer_webgpu_shim(
    _In_reads_bytes_(SrcDataSizeInBytes) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSizeInBytes,
    _In_ REFIID pRootSignatureDeserializerInterface,
    _Out_ void** ppRootSignatureDeserializer)
{
    return g_D3D12CreateRootSignatureDeserializer(pSrcData, SrcDataSizeInBytes, pRootSignatureDeserializerInterface, ppRootSignatureDeserializer);
}

HRESULT WINAPI D3D12SerializeVersionedRootSignature_webgpu_shim(
    _In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
    _Out_ ID3DBlob** ppBlob,
    _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob)
{
    return g_D3D12SerializeVersionedRootSignature(pRootSignature, ppBlob, ppErrorBlob);
}

HRESULT WINAPI D3D12CreateVersionedRootSignatureDeserializer_webgpu_shim(
    _In_reads_bytes_(SrcDataSizeInBytes) LPCVOID pSrcData,
    _In_ SIZE_T SrcDataSizeInBytes,
    _In_ REFIID pRootSignatureDeserializerInterface,
    _Out_ void** ppRootSignatureDeserializer)
{
    return g_D3D12CreateVersionedRootSignatureDeserializer(pSrcData, SrcDataSizeInBytes, pRootSignatureDeserializerInterface, ppRootSignatureDeserializer);
}

HRESULT WINAPI D3D12EnableExperimentalFeatures_webgpu_shim(
    UINT NumFeatures,
    _In_count_(NumFeatures) const IID* pIIDs,
    _In_opt_count_(NumFeatures) void* pConfigurationStructs,
    _In_opt_count_(NumFeatures) UINT* pConfigurationStructSizes)
{
    return g_D3D12EnableExperimentalFeatures(NumFeatures, pIIDs, pConfigurationStructs, pConfigurationStructSizes);
}

// ============================================================================
// DLL entry point
// ============================================================================

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    static HINSTANCE d3d12Handle, dxgiHandle;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        char systemDirectoryChar[MAX_PATH];
        UINT result = GetSystemDirectory(systemDirectoryChar, MAX_PATH);
        if (result == 0) {
            return FALSE;
        }

        std::string coreLibDirectory(systemDirectoryChar);
        d3d12Handle = LoadLibraryA((coreLibDirectory + "\\d3d12.dll").c_str());
        if (d3d12Handle == NULL)
        {
            return FALSE;
        }

        g_D3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(d3d12Handle, "D3D12CreateDevice"));
        g_D3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(GetProcAddress(d3d12Handle, "D3D12GetDebugInterface"));
        g_D3D12SerializeRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(d3d12Handle, "D3D12SerializeRootSignature"));
        g_D3D12CreateRootSignatureDeserializer = reinterpret_cast<PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER>(GetProcAddress(d3d12Handle, "D3D12CreateRootSignatureDeserializer"));
        g_D3D12SerializeVersionedRootSignature = reinterpret_cast<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>(GetProcAddress(d3d12Handle, "D3D12SerializeVersionedRootSignature"));
        g_D3D12CreateVersionedRootSignatureDeserializer = reinterpret_cast<PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER>(GetProcAddress(d3d12Handle, "D3D12CreateVersionedRootSignatureDeserializer"));
        g_D3D12EnableExperimentalFeatures = reinterpret_cast<PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES>(GetProcAddress(d3d12Handle, "D3D12EnableExperimentalFeatures"));

        dxgiHandle = LoadLibraryA((coreLibDirectory + "\\dxgi.dll").c_str());
        if (dxgiHandle == NULL)
        {
            return FALSE;
        }
        g_DXGICreateFactory = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(GetProcAddress(dxgiHandle, "CreateDXGIFactory"));
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        FreeLibrary(d3d12Handle);
        FreeLibrary(dxgiHandle);
    }

    return TRUE;
}