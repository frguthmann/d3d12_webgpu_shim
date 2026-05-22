#pragma once

#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <string>
#include <Windows.h>

using Microsoft::WRL::ComPtr;

// ============================================================================
// Shared typedefs
// ============================================================================

typedef HRESULT(WINAPI* PFN_D3D12_ENABLE_EXPERIMENTAL_FEATURES)(
    UINT NumFeatures,
    _In_count_(NumFeatures) const IID* pIIDs,
    _In_opt_count_(NumFeatures) void* pConfigurationStructs,
    _In_opt_count_(NumFeatures) UINT* pConfigurationStructSizes);

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY)(REFIID riid, void** ppFactory);

// Must be defined by each translation unit that includes this header
extern PFN_CREATE_DXGI_FACTORY g_DXGICreateFactory;

// ============================================================================
// Shim classes
// ============================================================================

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

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->GetPrivateData(guid, pDataSize, pData);
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->SetPrivateData(guid, DataSize, pData);
    }

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->SetPrivateDataInterface(guid, pData);
    }

    virtual HRESULT STDMETHODCALLTYPE SetName(LPCWSTR Name)
    {
        return reinterpret_cast<ID3D12Object*>(object_)->SetName(Name);
    }
};

class ID3D12DeviceChild_webgpu_shim : public ID3D12Object_webgpu_shim
{
public:
    ID3D12DeviceChild_webgpu_shim(IUnknown* wrapped_object) : ID3D12Object_webgpu_shim(wrapped_object) {};

    virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppvDevice)
    {
        return reinterpret_cast<ID3D12DeviceChild*>(object_)->GetDevice(riid, ppvDevice);
    }
};

class ID3D12Pageable_webgpu_shim : public ID3D12DeviceChild_webgpu_shim
{
public:
    ID3D12Pageable_webgpu_shim(IUnknown* wrapped_object) : ID3D12DeviceChild_webgpu_shim(wrapped_object) {};
};

class ID3D12SharingContract_webgpu_shim : public IUnknown_webgpu_shim
{
public:
    ID3D12SharingContract_webgpu_shim(IUnknown* wrapped_object, IDXGISwapChain1* swapChain)
        : swapChain_(swapChain), IUnknown_webgpu_shim(wrapped_object) {};

    virtual void STDMETHODCALLTYPE Present(ID3D12Resource* pResource, UINT Subresource, HWND window)
    {
        if (object_ != nullptr)
        {
            return reinterpret_cast<ID3D12SharingContract*>(object_)->Present(pResource, Subresource, window);
        }
        if (swapChain_ != nullptr)
        {
            swapChain_->Present(0, 0);
        }
    }

    virtual void STDMETHODCALLTYPE SharedFenceSignal(ID3D12Fence* pFence, UINT64 FenceValue)
    {
        if (object_ != nullptr)
        {
            return reinterpret_cast<ID3D12SharingContract*>(object_)->SharedFenceSignal(pFence, FenceValue);
        }
    }

    virtual void STDMETHODCALLTYPE BeginCapturableWork(REFGUID guid)
    {
        if (object_ != nullptr)
        {
            return reinterpret_cast<ID3D12SharingContract*>(object_)->BeginCapturableWork(guid);
        }
    }

    virtual void STDMETHODCALLTYPE EndCapturableWork(REFGUID guid)
    {
        if (object_ != nullptr)
        {
            return reinterpret_cast<ID3D12SharingContract*>(object_)->EndCapturableWork(guid);
        }
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG res = IUnknown_webgpu_shim::Release();
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
        swapChainDesc.Width = 1;
        swapChainDesc.Height = 1;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.SampleDesc.Count = 1;

        factory->CreateSwapChainForComposition((IUnknown*)object_, &swapChainDesc, nullptr, &swapChain_);
    };

    virtual void STDMETHODCALLTYPE UpdateTileMappings(
        ID3D12Resource* pResource, UINT NumResourceRegions,
        const D3D12_TILED_RESOURCE_COORDINATE* pResourceRegionStartCoordinates,
        const D3D12_TILE_REGION_SIZE* pResourceRegionSizes,
        ID3D12Heap* pHeap, UINT NumRanges,
        const D3D12_TILE_RANGE_FLAGS* pRangeFlags,
        const UINT* pHeapRangeStartOffsets,
        const UINT* pRangeTileCounts,
        D3D12_TILE_MAPPING_FLAGS Flags)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->UpdateTileMappings(
            pResource, NumResourceRegions, pResourceRegionStartCoordinates,
            pResourceRegionSizes, pHeap, NumRanges, pRangeFlags,
            pHeapRangeStartOffsets, pRangeTileCounts, Flags);
    }

    virtual void STDMETHODCALLTYPE CopyTileMappings(
        ID3D12Resource* pDstResource, const D3D12_TILED_RESOURCE_COORDINATE* pDstRegionStartCoordinate,
        ID3D12Resource* pSrcResource, const D3D12_TILED_RESOURCE_COORDINATE* pSrcRegionStartCoordinate,
        const D3D12_TILE_REGION_SIZE* pRegionSize, D3D12_TILE_MAPPING_FLAGS Flags)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->CopyTileMappings(
            pDstResource, pDstRegionStartCoordinate, pSrcResource,
            pSrcRegionStartCoordinate, pRegionSize, Flags);
    }

    virtual void STDMETHODCALLTYPE ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->ExecuteCommandLists(NumCommandLists, ppCommandLists);
    }

    virtual void STDMETHODCALLTYPE SetMarker(UINT Metadata, const void* pData, UINT Size)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->SetMarker(Metadata, pData, Size);
    }

    virtual void STDMETHODCALLTYPE BeginEvent(UINT Metadata, const void* pData, UINT Size)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->BeginEvent(Metadata, pData, Size);
    }

    virtual void STDMETHODCALLTYPE EndEvent(void)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->EndEvent();
    }

    virtual HRESULT STDMETHODCALLTYPE Signal(ID3D12Fence* pFence, UINT64 Value)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->Signal(pFence, Value);
    }

    virtual HRESULT STDMETHODCALLTYPE Wait(ID3D12Fence* pFence, UINT64 Value)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->Wait(pFence, Value);
    }

    virtual HRESULT STDMETHODCALLTYPE GetTimestampFrequency(UINT64* pFrequency)
    {
        return reinterpret_cast<ID3D12CommandQueue*>(object_)->GetTimestampFrequency(pFrequency);
    }

    virtual HRESULT STDMETHODCALLTYPE GetClockCalibration(UINT64* pGpuTimestamp, UINT64* pCpuTimestamp)
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
            ID3D12SharingContract_webgpu_shim* sharingContract =
                new ID3D12SharingContract_webgpu_shim(reinterpret_cast<IUnknown*>(*object), swapChain_);
            *object = sharingContract;
        }
        return res;
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG res = IUnknown_webgpu_shim::Release();
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
        const D3D12_COMMAND_QUEUE_DESC* pDesc, REFIID riid, void** ppCommandQueue)
    {
        HRESULT res = reinterpret_cast<ID3D12Device*>(object_)->CreateCommandQueue(pDesc, riid, ppCommandQueue);
        if (res == S_OK)
        {
            ID3D12CommandQueue_webgpu_shim* commandQueue =
                new ID3D12CommandQueue_webgpu_shim(reinterpret_cast<IUnknown*>(*ppCommandQueue));
            *ppCommandQueue = commandQueue;
        }
        return res;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE type, REFIID riid, void** ppCommandAllocator)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommandAllocator(type, riid, ppCommandAllocator);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateGraphicsPipelineState(
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc, REFIID riid, void** ppPipelineState)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateGraphicsPipelineState(pDesc, riid, ppPipelineState);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateComputePipelineState(
        const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc, REFIID riid, void** ppPipelineState)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateComputePipelineState(pDesc, riid, ppPipelineState);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandList(
        UINT nodeMask, D3D12_COMMAND_LIST_TYPE type,
        ID3D12CommandAllocator* pCommandAllocator,
        ID3D12PipelineState* pInitialState, REFIID riid, void** ppCommandList)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommandList(
            nodeMask, type, pCommandAllocator, pInitialState, riid, ppCommandList);
    }

    virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
        D3D12_FEATURE Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateDescriptorHeap(
        const D3D12_DESCRIPTOR_HEAP_DESC* pDescriptorHeapDesc, REFIID riid, void** ppvHeap)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateDescriptorHeap(pDescriptorHeapDesc, riid, ppvHeap);
    }

    virtual UINT STDMETHODCALLTYPE GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetDescriptorHandleIncrementSize(DescriptorHeapType);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateRootSignature(
        UINT nodeMask, const void* pBlobWithRootSignature, SIZE_T blobLengthInBytes, REFIID riid, void** ppvRootSignature)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid, ppvRootSignature);
    }

    virtual void STDMETHODCALLTYPE CreateConstantBufferView(
        const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateConstantBufferView(pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateShaderResourceView(
        ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateShaderResourceView(pResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateUnorderedAccessView(
        ID3D12Resource* pResource, ID3D12Resource* pCounterResource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateRenderTargetView(
        ID3D12Resource* pResource, const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateRenderTargetView(pResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateDepthStencilView(
        ID3D12Resource* pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateDepthStencilView(pResource, pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CreateSampler(
        const D3D12_SAMPLER_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateSampler(pDesc, DestDescriptor);
    }

    virtual void STDMETHODCALLTYPE CopyDescriptors(
        UINT NumDestDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts,
        const UINT* pDestDescriptorRangeSizes, UINT NumSrcDescriptorRanges,
        const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts,
        const UINT* pSrcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CopyDescriptors(
            NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
            NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, DescriptorHeapsType);
    }

    virtual void STDMETHODCALLTYPE CopyDescriptorsSimple(
        UINT NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
        D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CopyDescriptorsSimple(
            NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType);
    }

    virtual D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo(
        UINT visibleMask, UINT numResourceDescs, const D3D12_RESOURCE_DESC* pResourceDescs)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetResourceAllocationInfo(visibleMask, numResourceDescs, pResourceDescs);
    }

    virtual D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE GetCustomHeapProperties(UINT nodeMask, D3D12_HEAP_TYPE heapType)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetCustomHeapProperties(nodeMask, heapType);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource(
        const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags,
        const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riidResource, void** ppvResource)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommittedResource(
            pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, riidResource, ppvResource);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateHeap(const D3D12_HEAP_DESC* pDesc, REFIID riid, void** ppvHeap)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateHeap(pDesc, riid, ppvHeap);
    }

    virtual HRESULT STDMETHODCALLTYPE CreatePlacedResource(
        ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        REFIID riid, void** ppvResource)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreatePlacedResource(
            pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, riid, ppvResource);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateReservedResource(
        const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue, REFIID riid, void** ppvResource)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateReservedResource(
            pDesc, InitialState, pOptimizedClearValue, riid, ppvResource);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle(
        ID3D12DeviceChild* pObject, const SECURITY_ATTRIBUTES* pAttributes,
        DWORD Access, LPCWSTR Name, HANDLE* pHandle)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateSharedHandle(pObject, pAttributes, Access, Name, pHandle);
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedHandle(HANDLE NTHandle, REFIID riid, void** ppvObj)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->OpenSharedHandle(NTHandle, riid, ppvObj);
    }

    virtual HRESULT STDMETHODCALLTYPE OpenSharedHandleByName(LPCWSTR Name, DWORD Access, HANDLE* pNTHandle)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->OpenSharedHandleByName(Name, Access, pNTHandle);
    }

    virtual HRESULT STDMETHODCALLTYPE MakeResident(UINT NumObjects, ID3D12Pageable* const* ppObjects)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->MakeResident(NumObjects, ppObjects);
    }

    virtual HRESULT STDMETHODCALLTYPE Evict(UINT NumObjects, ID3D12Pageable* const* ppObjects)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->Evict(NumObjects, ppObjects);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateFence(
        UINT64 InitialValue, D3D12_FENCE_FLAGS Flags, REFIID riid, void** ppFence)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateFence(InitialValue, Flags, riid, ppFence);
    }

    virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetDeviceRemovedReason();
    }

    virtual void STDMETHODCALLTYPE GetCopyableFootprints(
        const D3D12_RESOURCE_DESC* pResourceDesc, UINT FirstSubresource, UINT NumSubresources,
        UINT64 BaseOffset, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
        UINT* pNumRows, UINT64* pRowSizeInBytes, UINT64* pTotalBytes)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetCopyableFootprints(
            pResourceDesc, FirstSubresource, NumSubresources, BaseOffset, pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateQueryHeap(const D3D12_QUERY_HEAP_DESC* pDesc, REFIID riid, void** ppvHeap)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateQueryHeap(pDesc, riid, ppvHeap);
    }

    virtual HRESULT STDMETHODCALLTYPE SetStablePowerState(BOOL Enable)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->SetStablePowerState(Enable);
    }

    virtual HRESULT STDMETHODCALLTYPE CreateCommandSignature(
        const D3D12_COMMAND_SIGNATURE_DESC* pDesc, ID3D12RootSignature* pRootSignature,
        REFIID riid, void** ppvCommandSignature)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->CreateCommandSignature(pDesc, pRootSignature, riid, ppvCommandSignature);
    }

    virtual void STDMETHODCALLTYPE GetResourceTiling(
        ID3D12Resource* pTiledResource, UINT* pNumTilesForEntireResource,
        D3D12_PACKED_MIP_INFO* pPackedMipDesc, D3D12_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
        UINT* pNumSubresourceTilings, UINT FirstSubresourceTilingToGet,
        D3D12_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetResourceTiling(
            pTiledResource, pNumTilesForEntireResource, pPackedMipDesc,
            pStandardTileShapeForNonPackedMips, pNumSubresourceTilings,
            FirstSubresourceTilingToGet, pSubresourceTilingsForNonPackedMips);
    }

    virtual LUID STDMETHODCALLTYPE GetAdapterLuid(void)
    {
        return reinterpret_cast<ID3D12Device*>(object_)->GetAdapterLuid();
    }

    virtual ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG res = IUnknown_webgpu_shim::Release();
        if (refCount_ == 0)
        {
            delete this;
        }
        return res;
    }
};
