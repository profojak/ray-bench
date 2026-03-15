// ============================================================================

/// @brief DXGI hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#include "util/log.h"

export module RayBench.Hook:DXGI.Hook;

import :DXGI.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

///< DXGI dynamic-link library handle
static HMODULE dxgi_module = nullptr;

// ============================================================================

HRESULT WINAPI Hooked_CreateDXGIFactory (REFIID riid, void** ppFactory)
{
    HRESULT hr = Original_CreateDXGIFactory (riid, ppFactory);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'CreateDXGIFactory'");

    return hr;
}

HRESULT WINAPI Hooked_CreateDXGIFactory1 (REFIID riid, void** ppFactory)
{
    HRESULT hr = Original_CreateDXGIFactory1 (riid, ppFactory);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'CreateDXGIFactory1'");

    return hr;
}

HRESULT WINAPI Hooked_CreateDXGIFactory2 (UINT Flags, REFIID riid, void** ppFactory)
{
    HRESULT hr = Original_CreateDXGIFactory2 (Flags, riid, ppFactory);

    RAYBENCH_LOG_TRACE_ONCE ("Hooked 'CreateDXGIFactory2'");

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