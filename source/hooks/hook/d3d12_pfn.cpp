// ============================================================================

/// @brief D3D12 function pointers

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>

export module RayBench.Hook:D3D12.Pfn;

namespace raybench::hook
{

// ============================================================================

namespace Original_ID3D12Resource
{
using pfn_GetGPUVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS (WINAPI*)(ID3D12Resource*);

pfn_GetGPUVirtualAddress GetGPUVirtualAddress = nullptr;

// ----------------------------------------------------------------------------

using pfn_Release = ULONG (WINAPI*)(ID3D12Resource*);

pfn_Release Release = nullptr;
}

// ============================================================================

namespace Original_ID3D12GraphicsCommandList
{
using pfn_ResourceBarrier = void (WINAPI*)(ID3D12GraphicsCommandList*, UINT, const D3D12_RESOURCE_BARRIER*);

pfn_ResourceBarrier ResourceBarrier = nullptr;

// ----------------------------------------------------------------------------

using pfn_Barrier = void (WINAPI*)(ID3D12GraphicsCommandList7*, UINT32, const D3D12_BARRIER_GROUP*);

pfn_Barrier Barrier = nullptr;

// ----------------------------------------------------------------------------

using pfn_BuildRaytracingAccelerationStructure = void (WINAPI*) (
    ID3D12GraphicsCommandList4*,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*,
    UINT,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*);

pfn_BuildRaytracingAccelerationStructure BuildRaytracingAccelerationStructure = nullptr;
}

// ============================================================================

namespace Original_ID3D12CommandQueue
{
using pfn_ExecuteCommandLists = void (WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

pfn_ExecuteCommandLists ExecuteCommandLists = nullptr;
}

// ============================================================================

namespace Original_ID3D12Device
{
using pfn_CreateCommittedResource = HRESULT (WINAPI*) (ID3D12Device*,
                                                       const D3D12_HEAP_PROPERTIES*,
                                                       D3D12_HEAP_FLAGS,
                                                       const D3D12_RESOURCE_DESC*,
                                                       D3D12_RESOURCE_STATES,
                                                       const D3D12_CLEAR_VALUE*,
                                                       REFIID,
                                                       void**);
using pfn_CreateCommittedResource1 = HRESULT (WINAPI*) (ID3D12Device4*,
                                                        const D3D12_HEAP_PROPERTIES*,
                                                        D3D12_HEAP_FLAGS,
                                                        const D3D12_RESOURCE_DESC*,
                                                        D3D12_RESOURCE_STATES,
                                                        const D3D12_CLEAR_VALUE*,
                                                        ID3D12ProtectedResourceSession*,
                                                        REFIID,
                                                        void**);
using pfn_CreateCommittedResource2 = HRESULT (WINAPI*)  (ID3D12Device8*,
                                                         const D3D12_HEAP_PROPERTIES*,
                                                         D3D12_HEAP_FLAGS,
                                                         const D3D12_RESOURCE_DESC1*,
                                                         D3D12_RESOURCE_STATES,
                                                         const D3D12_CLEAR_VALUE*,
                                                         ID3D12ProtectedResourceSession*,
                                                         REFIID,
                                                         void**);
using pfn_CreateCommittedResource3 = HRESULT (WINAPI*) (ID3D12Device10*,
                                                        const D3D12_HEAP_PROPERTIES*,
                                                        D3D12_HEAP_FLAGS,
                                                        const D3D12_RESOURCE_DESC1*,
                                                        D3D12_BARRIER_LAYOUT,
                                                        const D3D12_CLEAR_VALUE*,
                                                        ID3D12ProtectedResourceSession*,
                                                        UINT32,
                                                        const DXGI_FORMAT*,
                                                        REFIID,
                                                        void**);

pfn_CreateCommittedResource CreateCommittedResource = nullptr;
pfn_CreateCommittedResource1 CreateCommittedResource1 = nullptr;
pfn_CreateCommittedResource2 CreateCommittedResource2 = nullptr;
pfn_CreateCommittedResource3 CreateCommittedResource3 = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreatePlacedResource = HRESULT (WINAPI*) (ID3D12Device*,
                                                    ID3D12Heap*,
                                                    UINT64,
                                                    const D3D12_RESOURCE_DESC*,
                                                    D3D12_RESOURCE_STATES,
                                                    const D3D12_CLEAR_VALUE*,
                                                    REFIID,
                                                    void**);
using pfn_CreatePlacedResource1 = HRESULT (WINAPI*) (ID3D12Device8*,
                                                     ID3D12Heap*,
                                                     UINT64,
                                                     const D3D12_RESOURCE_DESC1*,
                                                     D3D12_RESOURCE_STATES,
                                                     const D3D12_CLEAR_VALUE*,
                                                     REFIID,
                                                     void**);
using pfn_CreatePlacedResource2 = HRESULT (WINAPI*) (ID3D12Device10*,
                                                     ID3D12Heap*,
                                                     UINT64,
                                                     const D3D12_RESOURCE_DESC1*,
                                                     D3D12_BARRIER_LAYOUT,
                                                     const D3D12_CLEAR_VALUE*,
                                                     UINT32,
                                                     const DXGI_FORMAT*,
                                                     REFIID,
                                                     void**);

pfn_CreatePlacedResource CreatePlacedResource = nullptr;
pfn_CreatePlacedResource1 CreatePlacedResource1 = nullptr;
pfn_CreatePlacedResource2 CreatePlacedResource2 = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateReservedResource = HRESULT (WINAPI*) (ID3D12Device*,
                                                      const D3D12_RESOURCE_DESC*,
                                                      D3D12_RESOURCE_STATES,
                                                      const D3D12_CLEAR_VALUE*,
                                                      REFIID,
                                                      void**);
using pfn_CreateReservedResource1 = HRESULT (WINAPI*)  (ID3D12Device4*,
                                                        const D3D12_RESOURCE_DESC*,
                                                        D3D12_RESOURCE_STATES,
                                                        const D3D12_CLEAR_VALUE*,
                                                        ID3D12ProtectedResourceSession*,
                                                        REFIID,
                                                        void**);
using pfn_CreateReservedResource2 = HRESULT (WINAPI*)  (ID3D12Device10*,
                                                        const D3D12_RESOURCE_DESC*,
                                                        D3D12_BARRIER_LAYOUT,
                                                        const D3D12_CLEAR_VALUE*,
                                                        ID3D12ProtectedResourceSession*,
                                                        UINT32,
                                                        const DXGI_FORMAT*,
                                                        REFIID,
                                                        void**);

pfn_CreateReservedResource CreateReservedResource = nullptr;
pfn_CreateReservedResource1 CreateReservedResource1 = nullptr;
pfn_CreateReservedResource2 CreateReservedResource2 = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateCommandQueue = HRESULT (WINAPI*)(ID3D12Device*,
                                                 const D3D12_COMMAND_QUEUE_DESC*,
                                                 REFIID,
                                                 void**);
using pfn_CreateCommandQueue1 = HRESULT (WINAPI*)(ID3D12Device9*,
                                                  const D3D12_COMMAND_QUEUE_DESC*,
                                                  REFIID,
                                                  REFIID,
                                                  void**);

pfn_CreateCommandQueue CreateCommandQueue = nullptr;
pfn_CreateCommandQueue1 CreateCommandQueue1 = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateCommandList = HRESULT (WINAPI*)(ID3D12Device*,
                                                UINT,
                                                D3D12_COMMAND_LIST_TYPE,
                                                ID3D12CommandAllocator*,
                                                ID3D12PipelineState*,
                                                REFIID,
                                                void**);
using pfn_CreateCommandList1 = HRESULT (WINAPI*)(ID3D12Device4*,
                                                 UINT,
                                                 D3D12_COMMAND_LIST_TYPE,
                                                 ID3D12CommandAllocator*,
                                                 ID3D12PipelineState*,
                                                 REFIID,
                                                 void**);

pfn_CreateCommandList CreateCommandList = nullptr;
pfn_CreateCommandList1 CreateCommandList1 = nullptr;
}

// ============================================================================

using pfn_D3D12GetInterface = HRESULT (WINAPI*)(REFCLSID, REFIID, void**);

pfn_D3D12GetInterface Original_D3D12GetInterface = nullptr;

using pfn_D3D12CreateDevice = HRESULT (WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

pfn_D3D12CreateDevice Original_D3D12CreateDevice = nullptr;

}

// ----------------------------------------------------------------------------