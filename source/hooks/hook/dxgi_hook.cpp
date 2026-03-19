// ============================================================================

/// @brief DXGI hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi1_6.h>
#include <dxgi.h>

#include "lazy_hook.hpp"
#include "util/log.h"

export module RayBench.Hook:DXGI.Hook;

import :DXGI.Pfn;

import std;
import RayBench.Capture;
import RayBench.Util;

using namespace std::literals;
using Manager = raybench::capture::Manager;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

///< DXGI dynamic-link library handle
static HMODULE dxgi_module = nullptr;

// ============================================================================

namespace Hooked_IDXGISwapChain
{
RAYBENCH_LAZY_INIT;

HRESULT WINAPI Present (IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_IDXGISwapChain::Present (This, SyncInterval, Flags);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'IDXGISwapChain::Present'");

    std::shared_lock<Manager::APIMutex> lock = Manager::GetSharedLock ();

    manager.Pre_IDXGISwapChain_Present ();
    HRESULT hr = Original_IDXGISwapChain::Present (This, SyncInterval, Flags);
    manager.Post_IDXGISwapChain_Present (Flags, lock);

    manager.CallDepthDecrement ();
    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI Present1 (IDXGISwapChain1* This,
                         UINT SyncInterval,
                         UINT PresentFlags,
                         const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    auto& manager = Manager::GetManager ();

    uint32_t call_depth = manager.CallDepthIncrement ();
    if (call_depth > 1)
    {
        HRESULT hr = Original_IDXGISwapChain::Present1 (This, SyncInterval, PresentFlags, pPresentParameters);
        manager.CallDepthDecrement ();
        return hr;
    }

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'IDXGISwapChain::Present1'");

    std::shared_lock<Manager::APIMutex> lock = Manager::GetSharedLock ();

    manager.Pre_IDXGISwapChain_Present ();
    HRESULT hr = Original_IDXGISwapChain::Present1 (This, SyncInterval, PresentFlags, pPresentParameters);
    manager.Post_IDXGISwapChain_Present (PresentFlags, lock);

    manager.CallDepthDecrement ();
    return hr;
}

}

// ============================================================================

namespace Hooked_IDXGIFactory
{
RAYBENCH_LAZY_INIT;

HRESULT WINAPI CreateSwapChain (IDXGIFactory* This,
                                IUnknown* pDevice,
                                DXGI_SWAP_CHAIN_DESC* pDesc,
                                IDXGISwapChain** ppSwapChain)
{
    HRESULT hr = Original_IDXGIFactory::CreateSwapChain (This, pDevice, pDesc, ppSwapChain);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'IDXGIFactory::CreateSwapChain'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGISwapChain, ppSwapChain);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateSwapChainForHwnd (IDXGIFactory2* This,
                                       IUnknown* pDevice,
                                       HWND hWnd,
                                       const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                       const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
                                       IDXGIOutput* pRestrictToOutput,
                                       IDXGISwapChain1** ppSwapChain)
{
    HRESULT hr = Original_IDXGIFactory::CreateSwapChainForHwnd (This, pDevice, hWnd, pDesc, pFullscreenDesc,
                                                                pRestrictToOutput, ppSwapChain);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'IDXGIFactory::CreateSwapChainForHwnd'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGISwapChain, ppSwapChain);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateSwapChainForCoreWindow (IDXGIFactory2* This,
                                             IUnknown* pDevice,
                                             IUnknown* pWindow,
                                             const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                             IDXGIOutput* pRestrictToOutput,
                                             IDXGISwapChain1** ppSwapChain)
{
    HRESULT hr = Original_IDXGIFactory::CreateSwapChainForCoreWindow (This, pDevice, pWindow, pDesc,
                                                                      pRestrictToOutput, ppSwapChain);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'IDXGIFactory::CreateSwapChainForCoreWindow'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGISwapChain, ppSwapChain);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI CreateSwapChainForComposition (IDXGIFactory2* This,
                                              IUnknown* pDevice,
                                              const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                              IDXGIOutput* pRestrictToOutput,
                                              IDXGISwapChain1** ppSwapChain)
{
    HRESULT hr = Original_IDXGIFactory::CreateSwapChainForComposition (This, pDevice, pDesc, pRestrictToOutput,
                                                                       ppSwapChain);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'IDXGIFactory::CreateSwapChainForComposition'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGISwapChain, ppSwapChain);

    return hr;
}

}

// ============================================================================

HRESULT WINAPI Hooked_CreateDXGIFactory (REFIID riid, void** ppFactory)
{
    HRESULT hr = Original_CreateDXGIFactory (riid, ppFactory);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'CreateDXGIFactory'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGIFactory, ppFactory);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_CreateDXGIFactory1 (REFIID riid, void** ppFactory)
{
    HRESULT hr = Original_CreateDXGIFactory1 (riid, ppFactory);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'CreateDXGIFactory1'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGIFactory, ppFactory);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT WINAPI Hooked_CreateDXGIFactory2 (UINT Flags, REFIID riid, void** ppFactory)
{
    HRESULT hr = Original_CreateDXGIFactory2 (Flags, riid, ppFactory);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'CreateDXGIFactory2'");

    RAYBENCH_LAZY_HOOK (SUCCEEDED (hr), Hooked_IDXGIFactory, ppFactory);

    return hr;
}

// ============================================================================

/// @brief Hook DXGI API calls
/// 
/// @return True if successful, false otherwise
export bool HookDXGI ()
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

/// @brief Unhook DXGI API calls
///
/// @return True if successful, false otherwise
export bool UnhookDXGI ()
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