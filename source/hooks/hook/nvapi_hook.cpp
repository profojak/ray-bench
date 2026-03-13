// ============================================================================

/// @brief NVAPI hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

#include "nvapi_ids.h"
#include "util/log.h"

export module RayBench.Hook:NVAPI.Hook;

import :NVAPI.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;

namespace raybench::hook
{

///< NVAPI dynamic-link library handle
static HMODULE nvapi_module = nullptr;

// ============================================================================

NvAPI_Status WINAPI Hooked_NvAPI_DirectD3D12GraphicsCommandList_Create (
    ID3D12GraphicsCommandList* pDXD3D12GraphicsCommandList,
    INvAPI_DirectD3D12GraphicsCommandList** ppReturnD3D12GraphicsCommandList)
{
    return Original_NvAPI_DirectD3D12GraphicsCommandList_Create (pDXD3D12GraphicsCommandList,
                                                                 ppReturnD3D12GraphicsCommandList);
}

// ----------------------------------------------------------------------------

NvAPI_Status WINAPI Hooked_NvAPI_Initialize ()
{
    return Original_NvAPI_Initialize ();
}

// ============================================================================

/// @brief Hook NVAPI API calls
/// 
/// @return True if successful, false otherwise
export bool HookNvAPI ()
{
    bool result = true;

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

    Original_NvAPI_QueryInterface = reinterpret_cast<pfn_NvAPI_QueryInterface> (
        GetProcAddress (nvapi_module, "nvapi_QueryInterface"));

    Original_NvAPI_Initialize = reinterpret_cast<pfn_NvAPI_Initialize> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(0x0150E828)));
    Original_NvAPI_DirectD3D12GraphicsCommandList_Create = reinterpret_cast<pfn_NvAPI_DirectD3D12GraphicsCommandList_Create> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_DirectD3D12GraphicsCommandList_Create)));

    if (!raybench::util::HookAPICall (reinterpret_cast<PVOID*>(&Original_NvAPI_Initialize),
                                      reinterpret_cast<PVOID>(Hooked_NvAPI_Initialize)))
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'NvAPI_Initialize'!");
        result = false;
    }

    if (!raybench::util::HookAPICall (reinterpret_cast<PVOID*>(&Original_NvAPI_DirectD3D12GraphicsCommandList_Create),
                                      reinterpret_cast<PVOID>(Hooked_NvAPI_DirectD3D12GraphicsCommandList_Create)))
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'NvAPI_DirectD3D12GraphicsCommandList_Create'!");
        result = false;
    }

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook NVAPI API calls
///
/// @return True if successful, false otherwise
export bool UnhookNvAPI ()
{
    bool result = true;

    if (!raybench::util::UnhookAPICall (reinterpret_cast<PVOID*>(&Original_NvAPI_Initialize),
                                        reinterpret_cast<PVOID>(Hooked_NvAPI_Initialize)))
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'NvAPI_Initialize'!");
        result = false;
    }

    Original_NvAPI_Initialize = reinterpret_cast<pfn_NvAPI_Initialize> (
        Original_NvAPI_QueryInterface (static_cast<NvU32>(NvAPI_pfn_ID::NvAPI_Initialize)));

    return result;
}

}

// ----------------------------------------------------------------------------