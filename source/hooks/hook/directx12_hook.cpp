// ============================================================================

/// @brief DirectX 12 hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>
#include <d3d12.h>

#include "d3d12_vtables.h"
#include "util/log.h"

export module RayBench.Hook:DirectX12.Hook;

import :DirectX12.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

///< D3D12 dynamic-link library handle
static HMODULE d3d12_module = nullptr;
///< DXGI dynamic-link library handle
static HMODULE dxgi_module = nullptr;

// ============================================================================

struct Hooked_ID3D12Device
{
    inline static HRESULT WINAPI CreateCommandQueue (ID3D12Device* This,
                                                     const D3D12_COMMAND_QUEUE_DESC* pDesc,
                                                     REFIID riid,
                                                     void** ppCommandQueue)
    {
        return Original_ID3D12Device::CreateCommandQueue (This, pDesc, riid, ppCommandQueue);
    }

    // ------------------------------------------------------------------------

    inline static HRESULT WINAPI CreateCommandList (ID3D12Device* This,
                                                    UINT nodeMask,
                                                    D3D12_COMMAND_LIST_TYPE type,
                                                    ID3D12CommandAllocator* pCommandAllocator,
                                                    ID3D12PipelineState* pInitialState,
                                                    REFIID riid,
                                                    void** ppCommandList)
    {
        return Original_ID3D12Device::CreateCommandList (This, nodeMask, type, pCommandAllocator,
                                                         pInitialState, riid, ppCommandList);
    }

    inline static HRESULT WINAPI CreateCommandList1 (ID3D12Device4* This,
                                                     UINT nodeMask,
                                                     D3D12_COMMAND_LIST_TYPE type,
                                                     ID3D12CommandAllocator* pCommandAllocator,
                                                     ID3D12PipelineState* pInitialState,
                                                     REFIID riid,
                                                     void** ppCommandList)
    {
        return Original_ID3D12Device::CreateCommandList1 (This, nodeMask, type, pCommandAllocator,
                                                          pInitialState, riid, ppCommandList);
    }

    // ------------------------------------------------------------------------

    static void Hook ();
};

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_D3D12GetInterface (REFCLSID rclsid, REFIID riid, void** ppvDebug)
{
    return Original_D3D12GetInterface (rclsid, riid, ppvDebug);
}

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_D3D12CreateDevice (IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel,
                                         REFIID riid, void** ppDevice)
{
    return Original_D3D12CreateDevice (pAdapter, MinimumFeatureLevel, riid, ppDevice);
}

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_CreateDXGIFactory (REFIID riid, void** ppFactory)
{
    return Original_CreateDXGIFactory (riid, ppFactory);
}

HRESULT WINAPI Hooked_CreateDXGIFactory1 (REFIID riid, void** ppFactory)
{
    return Original_CreateDXGIFactory1 (riid, ppFactory);
}

HRESULT WINAPI Hooked_CreateDXGIFactory2 (UINT Flags, REFIID riid, void** ppFactory)
{
    return Original_CreateDXGIFactory2 (Flags, riid, ppFactory);
}

// ============================================================================

/// @brief Hook API calls using virtual function tables of dummy objects
void Hooked_ID3D12Device::Hook ()
{
    auto hook_once = [] ()
        {
            ID3D12Device* device = nullptr;
            HRESULT hr = Original_D3D12CreateDevice (nullptr, D3D_FEATURE_LEVEL_12_0,
                                                     IID_PPV_ARGS (&device));
            if (FAILED (hr))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to create D3D12 device: {}!", hr);
                return;
            }

            void** vtable = *reinterpret_cast<void***> (device);
            Original_ID3D12Device::CreateCommandQueue = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandQueue> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreateCommandQueue)]);
            Original_ID3D12Device::CreateCommandList = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandList> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreateCommandList)]);

            HookWrap(Original_ID3D12Device::CreateCommandQueue, Hooked_ID3D12Device::CreateCommandQueue, "ID3D12Device::CreateCommandQueue"sv);
            HookWrap (Original_ID3D12Device::CreateCommandList, Hooked_ID3D12Device::CreateCommandList, "ID3D12Device::CreateCommandList"sv);

            ID3D12Device4* device4 = nullptr;
            hr = device->QueryInterface (IID_PPV_ARGS (&device4));
            if (FAILED (hr))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to query 'ID3D12Device4' interface: {}!", hr);
                device->Release ();
                return;
            }

            vtable = *reinterpret_cast<void***> (device4);
            Original_ID3D12Device::CreateCommandList1 = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandList1> (
                vtable[static_cast<int>(ID3D12Device4_VTable_ID::CreateCommandList1)]);

            HookWrap (Original_ID3D12Device::CreateCommandList1, Hooked_ID3D12Device::CreateCommandList1, "ID3D12Device4::CreateCommandList1"sv);

            device4->Release ();
            device->Release ();
        };

    static std::once_flag init_flag;
    std::call_once (init_flag, hook_once);
}

// ============================================================================

/// @brief Hook `D3D12CreateDevice` and `D3D12GetInterface` API calls
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

    // Hook API calls using virtual function tables of dummy objects
    Hooked_ID3D12Device::Hook ();

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `D3D12CreateDevice` and `D3D12GetInterface` API calls
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

// ============================================================================

/// @brief Hook `CreateDXGIFactory` API calls
/// 
/// @return True if successful, false otherwise
export bool HookCreateDXGIFactory ()
{
    if (dxgi_module == nullptr)
    {
        dxgi_module = GetModuleHandleA ("dxgi.dll");
        if (dxgi_module == nullptr)
        {
            RAYBENCH_LOG_CRITICAL ("Failed to get handle for 'dxgi.dll': {}!",
                                   GetLastError ());
            return false;
        }
    }

    bool result = true;

    Original_CreateDXGIFactory = reinterpret_cast<pfn_CreateDXGIFactory> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory"));
    Original_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory1"));
    Original_CreateDXGIFactory2 = reinterpret_cast<pfn_CreateDXGIFactory2> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory2"));

    result &= HookWrap (Original_CreateDXGIFactory, Hooked_CreateDXGIFactory, "CreateDXGIFactory"sv);
    result &= HookWrap (Original_CreateDXGIFactory1, Hooked_CreateDXGIFactory1, "CreateDXGIFactory1"sv);
    result &= HookWrap (Original_CreateDXGIFactory2, Hooked_CreateDXGIFactory2, "CreateDXGIFactory2"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `CreateDXGIFactory` API calls
/// 
/// @return True if successful, false otherwise
export bool UnhookCreateDXGIFactory ()
{
    bool result = true;

    result &= UnhookWrap (Original_CreateDXGIFactory, Hooked_CreateDXGIFactory, "CreateDXGIFactory"sv);
    result &= UnhookWrap (Original_CreateDXGIFactory1, Hooked_CreateDXGIFactory1, "CreateDXGIFactory1"sv);
    result &= UnhookWrap (Original_CreateDXGIFactory2, Hooked_CreateDXGIFactory2, "CreateDXGIFactory2"sv);

    Original_CreateDXGIFactory = reinterpret_cast<pfn_CreateDXGIFactory> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory"));
    Original_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory"));
    Original_CreateDXGIFactory2 = reinterpret_cast<pfn_CreateDXGIFactory2> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory"));

    return result;
}

}

// ----------------------------------------------------------------------------