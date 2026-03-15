// ============================================================================

/// @brief DXGI lazy hook implementation

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi1_6.h>
#include <dxgi.h>

#include "dxgi_vtables.hpp"
#include "util/log.h"

export module RayBench.Hook:DXGI.Lazy;

import :DXGI.Hook;
import :DXGI.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

/// @brief Hook `IDXGIFactory` API calls
///
/// @param ppFactory `IDXGIFactory`
/// @return True if successful, false otherwise
bool Hooked_IDXGIFactory::LazyHook (void** ppFactory)
{
    extern bool is_hooked;

    if (ppFactory != nullptr && *ppFactory != nullptr)
    {
        IDXGIFactory* factory = reinterpret_cast<IDXGIFactory*>(*ppFactory);

        if (Original_IDXGIFactory::CreateSwapChain == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory);

            Original_IDXGIFactory::CreateSwapChain = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChain> (
                vtable[static_cast<int>(IDXGIFactory_VTable_ID::CreateSwapChain)]);

            HookWrap (Original_IDXGIFactory::CreateSwapChain, CreateSwapChain,
                      "IDXGIFactory::CreateSwapChain"sv);
        }

        IDXGIFactory2* factory2 = nullptr;
        if (FAILED (reinterpret_cast<IDXGIFactory*>(*ppFactory)->QueryInterface (IID_PPV_ARGS (&factory2))))
        {
            return false;
        }

        if (Original_IDXGIFactory::CreateSwapChainForHwnd == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory2);

            Original_IDXGIFactory::CreateSwapChainForHwnd = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChainForHwnd> (
                vtable[static_cast<int>(IDXGIFactory2_VTable_ID::CreateSwapChainForHwnd)]);

            HookWrap (Original_IDXGIFactory::CreateSwapChainForHwnd, CreateSwapChainForHwnd,
                      "IDXGIFactory2::CreateSwapChainForHwnd"sv);
        }

        if (Original_IDXGIFactory::CreateSwapChainForCoreWindow == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory2);

            Original_IDXGIFactory::CreateSwapChainForCoreWindow = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChainForCoreWindow> (
                vtable[static_cast<int>(IDXGIFactory2_VTable_ID::CreateSwapChainForCoreWindow)]);

            HookWrap (Original_IDXGIFactory::CreateSwapChainForCoreWindow, CreateSwapChainForCoreWindow,
                      "IDXGIFactory2::CreateSwapChainForCoreWindow"sv);
        }

        if (Original_IDXGIFactory::CreateSwapChainForComposition == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory2);

            Original_IDXGIFactory::CreateSwapChainForComposition = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChainForComposition> (
                vtable[static_cast<int>(IDXGIFactory2_VTable_ID::CreateSwapChainForComposition)]);

            HookWrap (Original_IDXGIFactory::CreateSwapChainForComposition, CreateSwapChainForComposition,
                      "IDXGIFactory2::CreateSwapChainForComposition"sv);
        }

        is_hooked = true;
    }
    return true;
}

}

// ----------------------------------------------------------------------------