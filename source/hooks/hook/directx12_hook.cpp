// ============================================================================

/// @brief DirectX 12 hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.Hook:DirectX12.Hook;

import :DirectX12.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;

namespace raybench::hook
{

///< DXGI dynamic-link library handle
static HMODULE dxgi_module = nullptr;

// ============================================================================

HRESULT Hooked_CreateDXGIFactory (REFIID riid, void** ppFactory)
{
    return Original_CreateDXGIFactory (riid, ppFactory);
}

HRESULT Hooked_CreateDXGIFactory1 (REFIID riid, void** ppFactory)
{
    return Original_CreateDXGIFactory1 (riid, ppFactory);
}

HRESULT Hooked_CreateDXGIFactory2 (UINT Flags, REFIID riid, void** ppFactory)
{
    return Original_CreateDXGIFactory2 (Flags, riid, ppFactory);
}

// ============================================================================

/// @brief Hook `CreateDXGIFactory` API calls
/// 
/// @return True if successful, false otherwise
export bool HookCreateDXGIFactory ()
{
    bool result = true;

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

    auto Hook = [&result] (auto& real_func, auto hook_func, std::string_view func_name)
        {
            if (!raybench::util::HookAPICall (reinterpret_cast<PVOID*>(&real_func),
                                              reinterpret_cast<PVOID>(hook_func)))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to hook '{}'", func_name);
                result = false;
            }
        };

    Original_CreateDXGIFactory = reinterpret_cast<pfn_CreateDXGIFactory> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory"));
    Original_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory1"));
    Original_CreateDXGIFactory2 = reinterpret_cast<pfn_CreateDXGIFactory2> (
        GetProcAddress (dxgi_module, "CreateDXGIFactory2"));

    Hook (Original_CreateDXGIFactory, Hooked_CreateDXGIFactory, "CreateDXGIFactory"sv);
    Hook (Original_CreateDXGIFactory1, Hooked_CreateDXGIFactory1, "CreateDXGIFactory1"sv);
    Hook (Original_CreateDXGIFactory2, Hooked_CreateDXGIFactory2, "CreateDXGIFactory2"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `CreateDXGIFactory` API calls
/// 
/// @return True if successful, false otherwise
export bool UnhookCreateDXGIFactory ()
{
    bool result = true;

    auto Unhook = [&result] (auto& real_func, auto hook_func, std::string_view func_name)
        {
            if (!raybench::util::UnhookAPICall (reinterpret_cast<PVOID*>(&real_func),
                                                reinterpret_cast<PVOID>(hook_func)))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to unhook '{}'", func_name);
                result = false;
                return;
            }
        };

    Unhook (Original_CreateDXGIFactory, Hooked_CreateDXGIFactory, "CreateDXGIFactory"sv);
    Unhook (Original_CreateDXGIFactory1, Hooked_CreateDXGIFactory1, "CreateDXGIFactory1"sv);
    Unhook (Original_CreateDXGIFactory2, Hooked_CreateDXGIFactory2, "CreateDXGIFactory2"sv);

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