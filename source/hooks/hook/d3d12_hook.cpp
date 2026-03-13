// ============================================================================

/// @brief D3D12 hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>

#include "d3d12_vtables.hpp"
#include "util/log.h"

export module RayBench.Hook:D3D12.Hook;

import :D3D12.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

///< D3D12 dynamic-link library handle
static HMODULE d3d12_module = nullptr;

// ============================================================================

namespace Hooked_ID3D12Device
{
static bool is_hooked = false;

HRESULT WINAPI CreateCommandQueue (ID3D12Device* This,
                                                 const D3D12_COMMAND_QUEUE_DESC* pDesc,
                                                 REFIID riid,
                                                 void** ppCommandQueue)
{
    HRESULT hr = Original_ID3D12Device::CreateCommandQueue (This, pDesc, riid, ppCommandQueue);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'ID3D12Device::CreateCommandQueue'");

    return hr;
}

// ----------------------------------------------------------------------------

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

    return hr;
}

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

    return hr;
}
};

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_D3D12GetInterface (REFCLSID rclsid, REFIID riid, void** ppvDebug)
{
    HRESULT hr = Original_D3D12GetInterface (rclsid, riid, ppvDebug);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'D3D12GetInterface'");

    return hr;
}

HRESULT WINAPI Hooked_D3D12CreateDevice (IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel,
                                         REFIID riid, void** ppDevice)
{
    HRESULT hr = Original_D3D12CreateDevice (pAdapter, MinimumFeatureLevel, riid, ppDevice);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'D3D12CreateDevice'");

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