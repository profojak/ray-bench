// ============================================================================

/// @brief NVAPI hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
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

    if (!raybench::util::HookAPICall (reinterpret_cast<PVOID*>(&Original_NvAPI_Initialize),
                                      reinterpret_cast<PVOID>(Hooked_NvAPI_Initialize)))
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'NvAPI_Initialize'!");
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