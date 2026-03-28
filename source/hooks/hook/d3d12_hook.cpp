// ============================================================================

/// @brief D3D12 hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>

#include "lazy_hook.hpp"
#include "util/log.h"

export module RayBench.Hook:D3D12.Hook;

import :D3D12.Pfn;

import std;
import RayBench.Capture;
import RayBench.Util;

using namespace std::literals;
using Manager = raybench::capture::Manager;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

///< D3D12 dynamic-link library handle
static HMODULE d3d12_module = nullptr;

// ============================================================================

namespace Hooked_ID3D12Resource
{
RAYBENCH_LAZY_INIT;

D3D12_GPU_VIRTUAL_ADDRESS WINAPI GetGPUVirtualAddress (ID3D12Resource* This)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        D3D12_GPU_VIRTUAL_ADDRESS result = Original_ID3D12Resource::GetGPUVirtualAddress (This);
        manager.CallDepthDecrement ();
        return result;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Resource::GetGPUVirtualAddress'");

    D3D12_GPU_VIRTUAL_ADDRESS result = Original_ID3D12Resource::GetGPUVirtualAddress (This);
    manager.Post_ID3D12Resource_GetGPUVirtualAddress (This, result);

    manager.CallDepthDecrement ();
    return result;
}

}

// ============================================================================

namespace Hooked_ID3D12GraphicsCommandList
{
RAYBENCH_LAZY_INIT;

void WINAPI ResourceBarrier (ID3D12GraphicsCommandList* This, UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        Original_ID3D12GraphicsCommandList::ResourceBarrier (This, NumBarriers, pBarriers);
        manager.CallDepthDecrement ();
        return;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12GraphicsCommandList::ResourceBarrier'");

    std::shared_lock<Manager::APIMutex> lock = Manager::GetSharedLock ();

    Original_ID3D12GraphicsCommandList::ResourceBarrier (This, NumBarriers, pBarriers);
    manager.Post_ID3D12GraphicsCommandList_ResourceBarrier (NumBarriers, pBarriers, lock);

    manager.CallDepthDecrement ();
}

// ----------------------------------------------------------------------------

void WINAPI Barrier (ID3D12GraphicsCommandList7* This,
                     UINT32 NumBarrierGroups,
                     const D3D12_BARRIER_GROUP* pBarrierGroups)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        Original_ID3D12GraphicsCommandList::Barrier (This, NumBarrierGroups, pBarrierGroups);
        manager.CallDepthDecrement ();
        return;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12GraphicsCommandList::ResourceBarrier'");

    std::shared_lock<Manager::APIMutex> lock = Manager::GetSharedLock ();

    Original_ID3D12GraphicsCommandList::Barrier (This, NumBarrierGroups, pBarrierGroups);
    // TODO: Add post-callback.

    manager.CallDepthDecrement ();
}

// ----------------------------------------------------------------------------

void WINAPI BuildRaytracingAccelerationStructure (
    ID3D12GraphicsCommandList4* This,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    UINT NumPostbuildInfoDescs,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* pPostbuildInfoDescs)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure (This, pDesc, NumPostbuildInfoDescs,
                                                                                  pPostbuildInfoDescs);
        manager.CallDepthDecrement ();
        return;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure'");

    std::shared_lock<Manager::APIMutex> lock = Manager::GetSharedLock ();

    Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure (This, pDesc, NumPostbuildInfoDescs,
                                                                              pPostbuildInfoDescs);
    manager.Post_ID3D12GraphicsCommandList_BuildRaytracingAccelerationStructure (This, pDesc, lock);

    manager.CallDepthDecrement ();
}

}

// ============================================================================

namespace Hooked_ID3D12Device
{
RAYBENCH_LAZY_INIT;

HRESULT WINAPI CreateCommittedResource (ID3D12Device* This,
                                        const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                        D3D12_HEAP_FLAGS HeapFlags,
                                        const D3D12_RESOURCE_DESC* pDesc,
                                        D3D12_RESOURCE_STATES InitialResourceState,
                                        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                        REFIID riidResource,
                                        void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateCommittedResource (This, pHeapProperties, HeapFlags, pDesc,
                                                                     InitialResourceState, pOptimizedClearValue,
                                                                     riidResource, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommittedResource'");

    HRESULT hr = Original_ID3D12Device::CreateCommittedResource (This, pHeapProperties, HeapFlags, pDesc,
                                                                 InitialResourceState, pOptimizedClearValue,
                                                                 riidResource, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateCommittedResource1 (ID3D12Device4* This,
                                         const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                         D3D12_HEAP_FLAGS HeapFlags,
                                         const D3D12_RESOURCE_DESC* pDesc,
                                         D3D12_RESOURCE_STATES InitialResourceState,
                                         const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                         ID3D12ProtectedResourceSession* pProtectedSession,
                                         REFIID riidResource,
                                         void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateCommittedResource1 (This, pHeapProperties, HeapFlags, pDesc,
                                                                      InitialResourceState, pOptimizedClearValue,
                                                                      pProtectedSession, riidResource, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommittedResource1'");

    HRESULT hr = Original_ID3D12Device::CreateCommittedResource1 (This, pHeapProperties, HeapFlags, pDesc,
                                                                  InitialResourceState, pOptimizedClearValue,
                                                                  pProtectedSession, riidResource, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateCommittedResource2 (ID3D12Device8* This,
                                         const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                         D3D12_HEAP_FLAGS HeapFlags,
                                         const D3D12_RESOURCE_DESC1* pDesc,
                                         D3D12_RESOURCE_STATES InitialResourceState,
                                         const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                         ID3D12ProtectedResourceSession* pProtectedSession,
                                         REFIID riidResource,
                                         void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateCommittedResource2 (This, pHeapProperties, HeapFlags, pDesc,
                                                                      InitialResourceState, pOptimizedClearValue,
                                                                      pProtectedSession, riidResource, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommittedResource2'");

    HRESULT hr = Original_ID3D12Device::CreateCommittedResource2 (This, pHeapProperties, HeapFlags, pDesc,
                                                                  InitialResourceState, pOptimizedClearValue,
                                                                  pProtectedSession, riidResource, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateCommittedResource3 (ID3D12Device10* This,
                                         const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                         D3D12_HEAP_FLAGS HeapFlags,
                                         const D3D12_RESOURCE_DESC1* pDesc,
                                         D3D12_BARRIER_LAYOUT InitialLayout,
                                         const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                         ID3D12ProtectedResourceSession* pProtectedSession,
                                         UINT32 NumCastableFormats,
                                         const DXGI_FORMAT* pCastableFormats,
                                         REFIID riidResource,
                                         void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateCommittedResource3 (This, pHeapProperties, HeapFlags, pDesc,
                                                                      InitialLayout, pOptimizedClearValue,
                                                                      pProtectedSession, NumCastableFormats,
                                                                      pCastableFormats, riidResource, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommittedResource3'");

    HRESULT hr = Original_ID3D12Device::CreateCommittedResource3 (This, pHeapProperties, HeapFlags, pDesc,
                                                                  InitialLayout, pOptimizedClearValue,
                                                                  pProtectedSession, NumCastableFormats,
                                                                  pCastableFormats, riidResource, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ============================================================================

HRESULT WINAPI CreatePlacedResource (ID3D12Device* This,
                                     ID3D12Heap* pHeap,
                                     UINT64 HeapOffset,
                                     const D3D12_RESOURCE_DESC* pDesc,
                                     D3D12_RESOURCE_STATES InitialState,
                                     const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                     REFIID riid,
                                     void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreatePlacedResource (This, pHeap, HeapOffset, pDesc, InitialState,
                                                                  pOptimizedClearValue, riid, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreatePlacedResource'");

    HRESULT hr = Original_ID3D12Device::CreatePlacedResource (This, pHeap, HeapOffset, pDesc, InitialState,
                                                              pOptimizedClearValue, riid, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreatePlacedResource1 (ID3D12Device8* This,
                                      ID3D12Heap* pHeap,
                                      UINT64 HeapOffset,
                                      const D3D12_RESOURCE_DESC1* pDesc,
                                      D3D12_RESOURCE_STATES InitialState,
                                      const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                      REFIID riid,
                                      void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreatePlacedResource1 (This, pHeap, HeapOffset, pDesc, InitialState,
                                                                   pOptimizedClearValue, riid, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreatePlacedResource1'");

    HRESULT hr = Original_ID3D12Device::CreatePlacedResource1 (This, pHeap, HeapOffset, pDesc, InitialState,
                                                               pOptimizedClearValue, riid, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreatePlacedResource2 (ID3D12Device10* This,
                                      ID3D12Heap* pHeap,
                                      UINT64 HeapOffset,
                                      const D3D12_RESOURCE_DESC1* pDesc,
                                      D3D12_BARRIER_LAYOUT InitialLayout,
                                      const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                      UINT32 NumCastableFormats,
                                      const DXGI_FORMAT* pCastableFormats,
                                      REFIID riid,
                                      void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreatePlacedResource2 (This, pHeap, HeapOffset, pDesc, InitialLayout,
                                                                   pOptimizedClearValue, NumCastableFormats,
                                                                   pCastableFormats, riid, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreatePlacedResource2'");

    HRESULT hr = Original_ID3D12Device::CreatePlacedResource2 (This, pHeap, HeapOffset, pDesc, InitialLayout,
                                                               pOptimizedClearValue, NumCastableFormats,
                                                               pCastableFormats, riid, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ============================================================================

HRESULT WINAPI CreateReservedResource (ID3D12Device* This,
                                       const D3D12_RESOURCE_DESC* pDesc,
                                       D3D12_RESOURCE_STATES InitialState,
                                       const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                       REFIID riid,
                                       void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateReservedResource (This, pDesc, InitialState, pOptimizedClearValue,
                                                                    riid, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateReservedResource'");

    HRESULT hr = Original_ID3D12Device::CreateReservedResource (This, pDesc, InitialState, pOptimizedClearValue,
                                                                riid, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateReservedResource1 (ID3D12Device4* This,
                                        const D3D12_RESOURCE_DESC* pDesc,
                                        D3D12_RESOURCE_STATES InitialState,
                                        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                        ID3D12ProtectedResourceSession* pProtectedSession,
                                        REFIID riid,
                                        void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateReservedResource1 (This, pDesc, InitialState, pOptimizedClearValue,
                                                                     pProtectedSession, riid, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateReservedResource1'");

    HRESULT hr = Original_ID3D12Device::CreateReservedResource1 (This, pDesc, InitialState, pOptimizedClearValue,
                                                                 pProtectedSession, riid, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateReservedResource2 (ID3D12Device10* This,
                                        const D3D12_RESOURCE_DESC* pDesc,
                                        D3D12_BARRIER_LAYOUT InitialLayout,
                                        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                        ID3D12ProtectedResourceSession* pProtectedSession,
                                        UINT32 NumCastableFormats,
                                        const DXGI_FORMAT* pCastableFormats,
                                        REFIID riid,
                                        void** ppvResource)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_ID3D12Device::CreateReservedResource2 (This, pDesc, InitialLayout, pOptimizedClearValue,
                                                                     pProtectedSession, NumCastableFormats,
                                                                     pCastableFormats, riid, ppvResource);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateReservedResource2'");

    HRESULT hr = Original_ID3D12Device::CreateReservedResource2 (This, pDesc, InitialLayout, pOptimizedClearValue,
                                                                 pProtectedSession, NumCastableFormats,
                                                                 pCastableFormats, riid, ppvResource);

    manager.CallDepthDecrement ();
    return hr;
}

// ============================================================================

HRESULT WINAPI CreateCommandList (ID3D12Device* This,
                                  UINT nodeMask,
                                  D3D12_COMMAND_LIST_TYPE type,
                                  ID3D12CommandAllocator* pCommandAllocator,
                                  ID3D12PipelineState* pInitialState,
                                  REFIID riid,
                                  void** ppCommandList)
{
    HRESULT hr = Original_ID3D12Device::CreateCommandList (This, nodeMask, type, pCommandAllocator,
                                                           pInitialState, riid, ppCommandList);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommandList'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_ID3D12GraphicsCommandList, ppCommandList);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateCommandList1 (ID3D12Device4* This,
                                   UINT nodeMask,
                                   D3D12_COMMAND_LIST_TYPE type,
                                   ID3D12CommandAllocator* pCommandAllocator,
                                   ID3D12PipelineState* pInitialState,
                                   REFIID riid,
                                   void** ppCommandList)
{
    HRESULT hr = Original_ID3D12Device::CreateCommandList1 (This, nodeMask, type, pCommandAllocator,
                                                            pInitialState, riid, ppCommandList);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommandList1'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_ID3D12GraphicsCommandList, ppCommandList);

    return hr;
}

}

// ============================================================================

HRESULT WINAPI Hooked_D3D12GetInterface (REFCLSID rclsid, REFIID riid, void** ppvDebug)
{
    HRESULT hr = Original_D3D12GetInterface (rclsid, riid, ppvDebug);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'D3D12GetInterface'");

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_D3D12CreateDevice (IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel,
                                         REFIID riid, void** ppDevice)
{
    HRESULT hr = Original_D3D12CreateDevice (pAdapter, MinimumFeatureLevel, riid, ppDevice);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'D3D12CreateDevice'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_ID3D12Device, ppDevice);
    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_ID3D12Resource, ppDevice);

    return hr;
}

// ============================================================================

/// @brief Hook D3D12 API calls
/// 
/// @return True if successful, false otherwise
export bool HookD3D12 ()
{
    if (d3d12_module == nullptr)
    {
        d3d12_module = GetModuleHandleA ("d3d12.dll");
        if (d3d12_module == nullptr)
        {
            RAYBENCH_LOG_CRITICAL ("Failed to get handle for 'd3d12.dll': {}!",
                                   GetLastError ());
            return false;
        }
    }

    bool result = true;

    Original_D3D12CreateDevice = reinterpret_cast<pfn_D3D12CreateDevice> (
        GetProcAddress (d3d12_module, "D3D12CreateDevice"));
    Original_D3D12GetInterface = reinterpret_cast<pfn_D3D12GetInterface> (
        GetProcAddress (d3d12_module, "D3D12GetInterface"));

    result &= HookWrap (Original_D3D12CreateDevice, Hooked_D3D12CreateDevice, "D3D12CreateDevice"sv);
    result &= HookWrap (Original_D3D12GetInterface, Hooked_D3D12GetInterface, "D3D12GetInterface"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook D3D12 API calls
/// 
/// @return True if successful, false otherwise
export bool UnhookD3D12 ()
{
    bool result = true;

    result &= UnhookWrap (Original_D3D12CreateDevice, Hooked_D3D12CreateDevice, "D3D12CreateDevice"sv);
    result &= UnhookWrap (Original_D3D12GetInterface, Hooked_D3D12GetInterface, "D3D12GetInterface"sv);

    Original_D3D12CreateDevice = reinterpret_cast<pfn_D3D12CreateDevice> (
        GetProcAddress (d3d12_module, "D3D12CreateDevice"));
    Original_D3D12GetInterface = reinterpret_cast<pfn_D3D12GetInterface> (
        GetProcAddress (d3d12_module, "D3D12GetInterface"));

    return result;
}

}

// ----------------------------------------------------------------------------