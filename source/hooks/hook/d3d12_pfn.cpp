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

struct Original_ID3D12Device
{
    using pfn_CreateCommandQueue = HRESULT (WINAPI*)(ID3D12Device*,
                                                     const D3D12_COMMAND_QUEUE_DESC*,
                                                     REFIID,
                                                     void**);

    inline static pfn_CreateCommandQueue CreateCommandQueue = nullptr;

    // ------------------------------------------------------------------------

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

    inline static pfn_CreateCommandList CreateCommandList = nullptr;
    inline static pfn_CreateCommandList1 CreateCommandList1 = nullptr;
};

// ----------------------------------------------------------------------------

using pfn_D3D12GetInterface = HRESULT (WINAPI*)(REFCLSID, REFIID, void**);

pfn_D3D12GetInterface Original_D3D12GetInterface = nullptr;

using pfn_D3D12CreateDevice = HRESULT (WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

pfn_D3D12CreateDevice Original_D3D12CreateDevice = nullptr;

}

// ----------------------------------------------------------------------------