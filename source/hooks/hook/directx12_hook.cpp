// ============================================================================

/// @brief DirectX 12 hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>
#include <d3d12.h>

#include "util/log.h"

export module RayBench.Hook:DirectX12.Hook;

import :DirectX12.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;

namespace raybench::hook
{

///< D3D12 dynamic-link library handle
static HMODULE d3d12_module = nullptr;
///< DXGI dynamic-link library handle
static HMODULE dxgi_module = nullptr;

// ============================================================================

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

/// @brief Hook `D3D12CreateDevice` and `D3D12GetInterface` API calls
/// 
/// @return True if successful, false otherwise
export bool HookD3D12 ()
{
    bool result = true;

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

    auto Hook = [&result] (auto& real_func, auto hook_func, std::string_view func_name)
        {
            if (!raybench::util::HookAPICall (reinterpret_cast<PVOID*>(&real_func),
                                              reinterpret_cast<PVOID>(hook_func)))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to hook '{}'", func_name);
                result = false;
            }
        };

    Original_D3D12CreateDevice = reinterpret_cast<pfn_D3D12CreateDevice> (
        GetProcAddress (d3d12_module, "D3D12CreateDevice"));
    Original_D3D12GetInterface = reinterpret_cast<pfn_D3D12GetInterface> (
        GetProcAddress (d3d12_module, "D3D12GetInterface"));

    Hook (Original_D3D12CreateDevice, Hooked_D3D12CreateDevice, "D3D12CreateDevice"sv);
    Hook (Original_D3D12GetInterface, Hooked_D3D12GetInterface, "D3D12GetInterface"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `D3D12CreateDevice` and `D3D12GetInterface` API calls
/// 
/// @return True if successful, false otherwise
export bool UnhookD3D12 ()
{
    bool result = true;

    auto Unhook = [&result] (auto& real_func, auto hook_func, std::string_view func_name)
        {
            if (!raybench::util::UnhookAPICall (reinterpret_cast<PVOID*>(&real_func),
                                                reinterpret_cast<PVOID>(hook_func)))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to unhook '{}'", func_name);
                result = false;
            }
        };

    Unhook (Original_D3D12CreateDevice, Hooked_D3D12CreateDevice, "D3D12CreateDevice"sv);
    Unhook (Original_D3D12GetInterface, Hooked_D3D12GetInterface, "D3D12GetInterface"sv);

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