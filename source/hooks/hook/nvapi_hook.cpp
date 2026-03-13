// ============================================================================

/// @brief NVAPI hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

#include "nvapi_ids.hpp"
#include "util/log.h"

export module RayBench.Hook:NVAPI.Hook;

import :NVAPI.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

///< NVAPI dynamic-link library handle
static HMODULE nvapi_module = nullptr;

// ============================================================================

NvAPI_Status WINAPI Hooked_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx (
    ID3D12GraphicsCommandList4* pCommandList,
    const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pBuildParams)
{
    NvAPI_Status status = Original_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx (pCommandList, pBuildParams);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'NvAPI_D3D12_BuildRaytracingAccelerationStructureEx'");

    return status;
}

// ----------------------------------------------------------------------------

NvAPI_Status WINAPI Hooked_NvAPI_DirectD3D12GraphicsCommandList_Create (
    ID3D12GraphicsCommandList* pDXD3D12GraphicsCommandList,
    INvAPI_DirectD3D12GraphicsCommandList** ppReturnD3D12GraphicsCommandList)
{
    NvAPI_Status status = Original_NvAPI_DirectD3D12GraphicsCommandList_Create (pDXD3D12GraphicsCommandList,
                                                                                ppReturnD3D12GraphicsCommandList);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'NvAPI_DirectD3D12GraphicsCommandList_Create'");

    return status;
}

// ----------------------------------------------------------------------------

NvAPI_Status WINAPI Hooked_NvAPI_Initialize ()
{
    NvAPI_Status status = Original_NvAPI_Initialize ();

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'NvAPI_Initialize'");

    return status;
}

// ============================================================================

/// @brief Hook NVAPI API calls
/// 
/// @return True if successful, false otherwise
export bool HookNvAPI ()
{
    if (nvapi_module == nullptr)
    {
        nvapi_module = GetModuleHandleA ("nvapi64.dll");
        if (nvapi_module == nullptr)
        {
            RAYBENCH_LOG_CRITICAL ("Failed to get handle for 'nvapi64.dll': {}!",
                                   GetLastError ());
            return false;
        }
    }

    bool result = true;

    Original_NvAPI_QueryInterface = reinterpret_cast<pfn_NvAPI_QueryInterface> (
        GetProcAddress (nvapi_module, "nvapi_QueryInterface"));

    Original_NvAPI_Initialize = reinterpret_cast<pfn_NvAPI_Initialize> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(0x0150E828)));
    Original_NvAPI_DirectD3D12GraphicsCommandList_Create = reinterpret_cast<pfn_NvAPI_DirectD3D12GraphicsCommandList_Create> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_DirectD3D12GraphicsCommandList_Create)));
    Original_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx = reinterpret_cast<pfn_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_D3D12_BuildRaytracingAccelerationStructureEx)));

    result &= HookWrap (Original_NvAPI_Initialize, Hooked_NvAPI_Initialize, "NvAPI_Initialize"sv);
    result &= HookWrap (Original_NvAPI_DirectD3D12GraphicsCommandList_Create, Hooked_NvAPI_DirectD3D12GraphicsCommandList_Create, "NvAPI_DirectD3D12GraphicsCommandList_Create"sv);
    result &= HookWrap (Original_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx, Hooked_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx, "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook NVAPI API calls
///
/// @return True if successful, false otherwise
export bool UnhookNvAPI ()
{
    bool result = true;

    UnhookWrap (Original_NvAPI_Initialize, Hooked_NvAPI_Initialize, "NvAPI_Initialize"sv);
    UnhookWrap (Original_NvAPI_DirectD3D12GraphicsCommandList_Create, Hooked_NvAPI_DirectD3D12GraphicsCommandList_Create, "NvAPI_DirectD3D12GraphicsCommandList_Create"sv);
    UnhookWrap (Original_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx, Hooked_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx, "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx"sv);

    Original_NvAPI_Initialize = reinterpret_cast<pfn_NvAPI_Initialize> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_Initialize)));
    Original_NvAPI_DirectD3D12GraphicsCommandList_Create = reinterpret_cast<pfn_NvAPI_DirectD3D12GraphicsCommandList_Create> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_DirectD3D12GraphicsCommandList_Create)));
    Original_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx = reinterpret_cast<pfn_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_D3D12_BuildRaytracingAccelerationStructureEx)));

    return result;
}

}

// ----------------------------------------------------------------------------