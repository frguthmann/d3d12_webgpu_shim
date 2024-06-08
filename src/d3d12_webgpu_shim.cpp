#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>  
#include <string>
#include <Windows.h>

using Microsoft::WRL::ComPtr;

PFN_D3D12_CREATE_DEVICE g_D3D12CreateDevice;
PFN_D3D12_GET_DEBUG_INTERFACE g_D3D12GetDebugInterface;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE g_D3D12SerializeRootSignature;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER g_D3D12CreateRootSignatureDeserializer;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE g_D3D12SerializeVersionedRootSignature;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER g_D3D12CreateVersionedRootSignatureDeserializer;

typedef HRESULT(WINAPI* PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES)( UINT NumFeatures,
    _In_count_(NumFeatures) const IID* pIIDs,
    _In_opt_count_(NumFeatures) void* pConfigurationStructs,
    _In_opt_count_(NumFeatures) UINT* pConfigurationStructSizes);
PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES g_D3D12EnableExperimentalFeatures;

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY)(REFIID riid, void** ppFactory);
PFN_CREATE_DXGI_FACTORY g_DXGICreateFactory;

class IUnknown_webgpu_shim : public IUnknown
{
public:
    IUnknown_webgpu_shim(IUnknown* wrapped_object) : object_(wrapped_object), refCount_(1) {}

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object)
    {
        return object_->QueryInterface(riid, object);
    }

    virtual ULONG STDMETHODCALLTYPE AddRef() override
    {
        ULONG res = ++refCount_;
        if (object_ != nullptr)
        {
            res = object_->AddRef();
        }

        return res;
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG res = --refCount_;
        if (object_ != nullptr)
        {
            res = object_->Release();
        }

        return res;
    }

    ~IUnknown_webgpu_shim()
    {
        object_ = nullptr;
        refCount_ = 0;
    }

protected:
    IUnknown* object_;
    uint32_t refCount_;
};

class ID3D12Object_webgpu_shim : public IUnknown_webgpu_shim
{
public:
    ID3D12Object_webgpu_shim(IUnknown* wrapped_object) : IUnknown_webgpu_shim(wrapped_object) {};

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        _In_  REFGUID guid,
        _Inout_  UINT* pDataSize,
        _Out_writes_bytes_opt_(*pDataSize)  void* pData)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->GetPrivateData(guid, pDataSize, pData);
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        _In_  REFGUID guid,
        _In_  UINT DataSize,
        _In_reads_bytes_opt_(DataSize)  const void* pData)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->SetPrivateData(guid, DataSize, pData);
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        _In_  REFGUID guid,
        _In_opt_  const IUnknown* pData)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->SetPrivateDataInterface(guid, pData);
    }

    virtual HRESULT STDMETHODCALLTYPE SetName(
        _In_z_  LPCWSTR Name)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->SetName(Name);
    }
};

class ID3D12DeviceChild_webgpu_shim : public ID3D12Object_webgpu_shim
{
public:
    ID3D12DeviceChild_webgpu_shim(IUnknown* wrapped_object) : ID3D12Object_webgpu_shim(wrapped_object) {};

    virtual HRESULT STDMETHODCALLTYPE GetDevice(
        REFIID riid,
        _COM_Outptr_opt_  void** ppvDevice)
    {
        return reinterpret_cast<ID3D12DeviceChild*>(object_)->GetDevice(riid, ppvDevice);
    }
};

class ID3D12Pageable_webgpu_shim : public ID3D12DeviceChild_webgpu_shim
{
public:
    ID3D12Pageable_webgpu_shim::ID3D12Pageable_webgpu_shim(IUnknown* wrapped_object) : ID3D12DeviceChild_webgpu_shim(wrapped_object) {};
};

class ID3D12SharingContract_webgpu_shim: public IUnknown_webgpu_shim
{
public:
    ID3D12SharingContract_webgpu_shim(IUnknown* wrapped_object, IDXGISwapChain1* swapChain) : swapChain_(swapChain), IUnknown_webgpu_shim(wrapped_object) {};

    virtual void STDMETHODCALLTYPE Present(
        _In_  ID3D12Resource * pResource,
        UINT Subresource,
        _In_  HWND window)
        {
            if (object_ != nullptr)
            {
                return reinterpret_cast<ID3D12SharingContract_webgpu_shim*>(object_)->Present(pResource, Subresource, window);
            }
            if (swapChain_ != nullptr)
            {
                swapChain_->Present(0, 0);
            }
        }

    virtual void STDMETHODCALLTYPE SharedFenceSignal(
        _In_  ID3D12Fence* pFence,
        UINT64 FenceValue)
        {
            if (object_ != nullptr)
            {
                return reinterpret_cast<ID3D12SharingContract_webgpu_shim*>(object_)->SharedFenceSignal(pFence, FenceValue);
            }
        }

    virtual void STDMETHODCALLTYPE BeginCapturableWork(
        _In_  REFGUID guid)
        {
            if (object_ != nullptr)
            {
                return reinterpret_cast<ID3D12SharingContract_webgpu_shim*>(object_)->BeginCapturableWork(guid);
            }
        }

    virtual void STDMETHODCALLTYPE EndCapturableWork(
        _In_  REFGUID guid)
        {
            if (object_ != nullptr)
            {
                return reinterpret_cast<ID3D12SharingContract_webgpu_shim*>(object_)->EndCapturableWork(guid);
            }
        }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        HRESULT res = IUnknown_webgpu_shim::Release();

        if (refCount_ == 0)
        {
            delete this;
        }

        return res;
    }

    ~ID3D12SharingContract_webgpu_shim()
    {
        swapChain_ = nullptr;
    }

private:
    IDXGISwapChain1* swapChain_;
};

class ID3D12CommandQueue_webgpu_shim : public ID3D12Pageable_webgpu_shim
{
public:
    ID3D12CommandQueue_webgpu_shim(IUnknown* wrapped_object) : swapChain_(nullptr), ID3D12Pageable_webgpu_shim(wrapped_object)
    {
        ComPtr<IDXGIFactory4> factory;
        HRESULT hr = g_DXGICreateFactory(IID_PPV_ARGS(&factory));
        if (hr != S_OK)
        {
            return;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = 1;
        swapChainDesc.Height = 1;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        factory->CreateSwapChainForComposition((IUnknown*)object_, &swapChainDesc, nullptr, &swapChain_);
    };

    virtual void STDMETHODCALLTYPE UpdateTileMappings(
        _In_  ID3D12Resource* pResource,
        UINT NumResourceRegions,
        _In_reads_opt_(NumResourceRegions)  const D3D12_TILED_RESOURCE_COORDINATE* pResourceRegionStartCoordinates,
        _In_reads_opt_(NumResourceRegions)  const D3D12_TILE_REGION_SIZE* pResourceRegionSizes,
        _In_opt_  ID3D12Heap* pHeap,
        UINT NumRanges,
        _In_reads_opt_(NumRanges)  const D3D12_TILE_RANGE_FLAGS* pRangeFlags,
        _In_reads_opt_(NumRanges)  const UINT* pHeapRangeStartOffsets,
        _In_reads_opt_(NumRanges)  const UINT* pRangeTileCounts,
        D3D12_TILE_MAPPING_FLAGS Flags)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->UpdateTileMappings(pResource, NumResourceRegions, pResourceRegionStartCoordinates,
            pResourceRegionSizes, pHeap, NumRanges, pRangeFlags, pHeapRangeStartOffsets, pRangeTileCounts, Flags);
    }

    virtual void STDMETHODCALLTYPE CopyTileMappings(
        _In_  ID3D12Resource* pDstResource,
        _In_  const D3D12_TILED_RESOURCE_COORDINATE* pDstRegionStartCoordinate,
        _In_  ID3D12Resource* pSrcResource,
        _In_  const D3D12_TILED_RESOURCE_COORDINATE* pSrcRegionStartCoordinate,
        _In_  const D3D12_TILE_REGION_SIZE* pRegionSize,
        D3D12_TILE_MAPPING_FLAGS Flags)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->CopyTileMappings(pDstResource, pDstRegionStartCoordinate, pSrcResource,
            pSrcRegionStartCoordinate, pRegionSize, Flags);
    }

    virtual void STDMETHODCALLTYPE ExecuteCommandLists(
        _In_  UINT NumCommandLists,
        _In_reads_(NumCommandLists)  ID3D12CommandList* const* ppCommandLists)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->ExecuteCommandLists(NumCommandLists, ppCommandLists);
    }

    virtual void STDMETHODCALLTYPE SetMarker(
        UINT Metadata,
        _In_reads_bytes_opt_(Size)  const void* pData,
        UINT Size)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->SetMarker(Metadata, pData, Size);
    }

    virtual void STDMETHODCALLTYPE BeginEvent(
        UINT Metadata,
        _In_reads_bytes_opt_(Size)  const void* pData,
        UINT Size)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->BeginEvent(Metadata, pData, Size);
    }

    virtual void STDMETHODCALLTYPE EndEvent(void)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->EndEvent();
    }

    virtual HRESULT STDMETHODCALLTYPE Signal(
        ID3D12Fence* pFence,
        UINT64 Value)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->Signal(pFence, Value);
    }

    virtual HRESULT STDMETHODCALLTYPE Wait(
        ID3D12Fence* pFence,
        UINT64 Value)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->Wait(pFence, Value);
    }

    virtual HRESULT STDMETHODCALLTYPE GetTimestampFrequency(
        _Out_  UINT64* pFrequency)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->GetTimestampFrequency(pFrequency);
    }

    virtual HRESULT STDMETHODCALLTYPE GetClockCalibration(
        _Out_  UINT64* pGpuTimestamp,
        _Out_  UINT64* pCpuTimestamp)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->GetClockCalibration(pGpuTimestamp, pCpuTimestamp);
    }

    virtual D3D12_COMMAND_QUEUE_DESC STDMETHODCALLTYPE GetDesc(void)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->GetDesc();
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object)
    {
        HRESULT res = object_->QueryInterface(riid, object);

        if (riid == IID_ID3D12SharingContract)
        {
            ID3D12SharingContract_webgpu_shim* sharingContract = new ID3D12SharingContract_webgpu_shim(reinterpret_cast<IUnknown*>(*object), swapChain_);
            *object = sharingContract;
        }
        return res;
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        HRESULT res = IUnknown_webgpu_shim::Release();

        if (refCount_ == 0)
        {
            delete this;
        }

        return res;
    }

    ~ID3D12CommandQueue_webgpu_shim()
    {
        if (swapChain_ != nullptr)
        {
            swapChain_->Release();
            swapChain_ = nullptr;
        }
    }

private:
    IDXGISwapChain1* swapChain_;
};

class ID3D12Device_webgpu_shim : public ID3D12Object_webgpu_shim
{
public:

    ID3D12Device_webgpu_shim(IUnknown* wrapped_object) : ID3D12Object_webgpu_shim(wrapped_object) {};

    virtual UINT STDMETHODCALLTYPE GetNodeCount(void)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetNodeCount();
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandQueue(
        _In_  const D3D12_COMMAND_QUEUE_DESC* pDesc,
        REFIID riid,
        _COM_Outptr_  void** ppCommandQueue)
    {
        HRESULT res = reinterpret_cast<ID3D12Device*>(object_)->CreateCommandQueue(pDesc, riid, ppCommandQueue);

        if (res == S_OK)
        {
            ID3D12CommandQueue_webgpu_shim* commandQueue = new ID3D12CommandQueue_webgpu_shim(reinterpret_cast<IUnknown*>(*ppCommandQueue));
            *ppCommandQueue = commandQueue;
        }

        return res;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandAllocator(
        _In_  D3D12_COMMAND_LIST_TYPE type,
        REFIID riid,
        _COM_Outptr_  void** ppCommandAllocator)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommandAllocator(type, riid, ppCommandAllocator);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateGraphicsPipelineState(
        _In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
        REFIID riid,
        _COM_Outptr_  void** ppPipelineState)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateGraphicsPipelineState(pDesc, riid, ppPipelineState);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateComputePipelineState(
        _In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
        REFIID riid,
        _COM_Outptr_  void** ppPipelineState)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateComputePipelineState(pDesc, riid, ppPipelineState);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandList(
        _In_  UINT nodeMask,
        _In_  D3D12_COMMAND_LIST_TYPE type,
        _In_  ID3D12CommandAllocator* pCommandAllocator,
        _In_opt_  ID3D12PipelineState* pInitialState,
        REFIID riid,
        _COM_Outptr_  void** ppCommandList)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid, ppCommandList);
    }

    virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
        D3D12_FEATURE Feature,
        _Inout_updates_bytes_(FeatureSupportDataSize)  void* pFeatureSupportData,
        UINT FeatureSupportDataSize)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDescriptorHeap(
        _In_  const D3D12_DESCRIPTOR_HEAP_DESC* pDescriptorHeapDesc,
        REFIID riid,
        _COM_Outptr_  void** ppvHeap)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateDescriptorHeap(pDescriptorHeapDesc, riid, ppvHeap);
    }

    virtual UINT STDMETHODCALLTYPE GetDescriptorHandleIncrementSize(
        _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetDescriptorHandleIncrementSize(DescriptorHeapType);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateRootSignature(
        _In_  UINT nodeMask,
        _In_reads_(blobLengthInBytes)  const void* pBlobWithRootSignature,
        _In_  SIZE_T blobLengthInBytes,
        REFIID riid,
        _COM_Outptr_  void** ppvRootSignature)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid, ppvRootSignature);
    }

    virtual void STDMETHODCALLTYPE CreateConstantBufferView(
        _In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateConstantBufferView(pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateShaderResourceView(
        _In_opt_  ID3D12Resource* pResource,
        _In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateShaderResourceView(pResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateUnorderedAccessView(
        _In_opt_  ID3D12Resource* pResource,
        _In_opt_  ID3D12Resource* pCounterResource,
        _In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateRenderTargetView(
        _In_opt_  ID3D12Resource* pResource,
        _In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateRenderTargetView(pResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateDepthStencilView(
        _In_opt_  ID3D12Resource* pResource,
        _In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateDepthStencilView(pResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateSampler(
        _In_  const D3D12_SAMPLER_DESC* pDesc,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateSampler(pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CopyDescriptors(
        _In_  UINT NumDestDescriptorRanges,
        _In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts,
        _In_reads_opt_(NumDestDescriptorRanges)  const UINT* pDestDescriptorRangeSizes,
        _In_  UINT NumSrcDescriptorRanges,
        _In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts,
        _In_reads_opt_(NumSrcDescriptorRanges)  const UINT* pSrcDescriptorRangeSizes,
        _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
            NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, DescriptorHeapsType);
    }

    virtual void STDMETHODCALLTYPE CopyDescriptorsSimple(
        _In_  UINT NumDescriptors,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
        _In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
        _In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CopyDescriptorsSimple(NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType);
    }

    virtual D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo(
        _In_  UINT visibleMask,
        _In_  UINT numResourceDescs,
        _In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC* pResourceDescs)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetResourceAllocationInfo(visibleMask, numResourceDescs, pResourceDescs);
    }

    virtual D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE GetCustomHeapProperties(
        _In_  UINT nodeMask,
        D3D12_HEAP_TYPE heapType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetCustomHeapProperties(nodeMask, heapType);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource(
        _In_  const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAGS HeapFlags,
        _In_  const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES InitialResourceState,
        _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        REFIID riidResource,
        _COM_Outptr_opt_  void** ppvResource)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, riidResource, ppvResource);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateHeap(
        _In_  const D3D12_HEAP_DESC* pDesc,
        REFIID riid,
        _COM_Outptr_opt_  void** ppvHeap)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateHeap(pDesc, riid, ppvHeap);
    }

    virtual HRESULT STDMETHODCALLTYPE CreatePlacedResource(
        _In_  ID3D12Heap* pHeap,
        UINT64 HeapOffset,
        _In_  const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES InitialState,
        _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        REFIID riid,
        _COM_Outptr_opt_  void** ppvResource)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreatePlacedResource(pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, riid, ppvResource);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateReservedResource(
        _In_  const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES InitialState,
        _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        REFIID riid,
        _COM_Outptr_opt_  void** ppvResource)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, riid, ppvResource);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle(
        _In_  ID3D12DeviceChild* pObject,
        _In_opt_  const SECURITY_ATTRIBUTES* pAttributes,
        DWORD Access,
        _In_opt_  LPCWSTR Name,
        _Out_  HANDLE* pHandle)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateSharedHandle(pObject, pAttributes, Access, Name, pHandle);
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedHandle(
        _In_  HANDLE NTHandle,
        REFIID riid,
        _COM_Outptr_opt_  void** ppvObj)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->OpenSharedHandle(NTHandle, riid, ppvObj);
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedHandleByName(
        _In_  LPCWSTR Name,
        DWORD Access,
        /* [annotation][out] */
        _Out_  HANDLE* pNTHandle)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->OpenSharedHandleByName(Name, Access, pNTHandle);
    }

    virtual HRESULT STDMETHODCALLTYPE MakeResident(
        UINT NumObjects,
        _In_reads_(NumObjects)  ID3D12Pageable* const* ppObjects)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->MakeResident(NumObjects, ppObjects);
    }

    virtual HRESULT STDMETHODCALLTYPE Evict(
        UINT NumObjects,
        _In_reads_(NumObjects)  ID3D12Pageable* const* ppObjects)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->Evict(NumObjects, ppObjects);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateFence(
        UINT64 InitialValue,
        D3D12_FENCE_FLAGS Flags,
        REFIID riid,
        _COM_Outptr_  void** ppFence)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateFence(InitialValue, Flags, riid, ppFence);
    }

    virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetDeviceRemovedReason();
    }

    virtual void STDMETHODCALLTYPE GetCopyableFootprints(
        _In_  const D3D12_RESOURCE_DESC* pResourceDesc,
        _In_range_(0, D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
        _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource)  UINT NumSubresources,
        UINT64 BaseOffset,
        _Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
        _Out_writes_opt_(NumSubresources)  UINT* pNumRows,
        _Out_writes_opt_(NumSubresources)  UINT64* pRowSizeInBytes,
        _Out_opt_  UINT64* pTotalBytes)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetCopyableFootprints(pResourceDesc, FirstSubresource, NumSubresources, BaseOffset, pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateQueryHeap(
        _In_  const D3D12_QUERY_HEAP_DESC* pDesc,
        REFIID riid,
        _COM_Outptr_opt_  void** ppvHeap)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateQueryHeap(pDesc, riid, ppvHeap);
    }

    virtual HRESULT STDMETHODCALLTYPE SetStablePowerState(
        BOOL Enable)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->SetStablePowerState(Enable);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandSignature(
        _In_  const D3D12_COMMAND_SIGNATURE_DESC* pDesc,
        _In_opt_  ID3D12RootSignature* pRootSignature,
        REFIID riid,
        _COM_Outptr_opt_  void** ppvCommandSignature)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommandSignature(pDesc, pRootSignature, riid, ppvCommandSignature);
    }

    virtual void STDMETHODCALLTYPE GetResourceTiling(
        _In_  ID3D12Resource* pTiledResource,
        _Out_opt_  UINT* pNumTilesForEntireResource,
        _Out_opt_  D3D12_PACKED_MIP_INFO* pPackedMipDesc,
        _Out_opt_  D3D12_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
        _Inout_opt_  UINT* pNumSubresourceTilings,
        _In_  UINT FirstSubresourceTilingToGet,
        _Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetResourceTiling(pTiledResource, pNumTilesForEntireResource, pPackedMipDesc, pStandardTileShapeForNonPackedMips, pNumSubresourceTilings, FirstSubresourceTilingToGet, pSubresourceTilingsForNonPackedMips);
    }

    virtual LUID STDMETHODCALLTYPE GetAdapterLuid(void)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetAdapterLuid();
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        HRESULT res = IUnknown_webgpu_shim::Release();

        if (refCount_ == 0)
        {
            delete this;
        }

        return res;
    }
};

HRESULT WINAPI D3D12CreateDevice_webgpu_shim(
    _In_opt_ IUnknown* pAdapter,
    D3D_FEATURE_LEVEL MinimumFeatureLevel,
    _In_ REFIID riid, // Expected: ID3D12Device
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

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    static HINSTANCE d3d12Handle, dxgiHandle;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        MessageBoxA(nullptr, "Hello", "Hello", 0);

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