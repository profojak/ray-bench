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

/// @brief Hook `IDXGISwapChain` API calls
///
/// @param ppSwapChain `IDXGISwapChain`
/// @return True if successful, false otherwise
bool Hooked_IDXGISwapChain::LazyHook (void** ppSwapChain)
{
    if (ppSwapChain != nullptr && *ppSwapChain != nullptr)
    {
        IDXGISwapChain* swap_chain = reinterpret_cast<IDXGISwapChain*>(*ppSwapChain);

        if (Original_IDXGISwapChain::Present == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (swap_chain);
            Original_IDXGISwapChain::Present = reinterpret_cast<Original_IDXGISwapChain::pfn_Present> (
                vtable[static_cast<int>(IDXGISwapChain_VTable_ID::Present)]);
            bool result = HookWrap (Original_IDXGISwapChain::Present, Present, "IDXGISwapChain::Present"sv);
            if (result == false)
            {
                return false;
            }
        }

        IDXGISwapChain1* swap_chain1 = nullptr;
        if (FAILED (reinterpret_cast<IDXGISwapChain*>(*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&swap_chain1))))
        {
            return false;
        }

        if (Original_IDXGISwapChain::Present1 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (swap_chain1);
            Original_IDXGISwapChain::Present1 = reinterpret_cast<Original_IDXGISwapChain::pfn_Present1> (
                vtable[static_cast<int>(IDXGISwapChain1_VTable_ID::Present1)]);
            bool result = HookWrap (Original_IDXGISwapChain::Present1, Present1, "IDXGISwapChain::Present1"sv);
            if (result == false)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------

/// @brief Hook `IDXGIFactory` API calls
///
/// @param ppFactory `IDXGIFactory`
/// @return True if successful, false otherwise
bool Hooked_IDXGIFactory::LazyHook (void** ppFactory)
{
    if (ppFactory != nullptr && *ppFactory != nullptr)
    {
        IDXGIFactory* factory = reinterpret_cast<IDXGIFactory*>(*ppFactory);

        if (Original_IDXGIFactory::CreateSwapChain == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory);

            Original_IDXGIFactory::CreateSwapChain = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChain> (
                vtable[static_cast<int>(IDXGIFactory_VTable_ID::CreateSwapChain)]);

            bool result = HookWrap (Original_IDXGIFactory::CreateSwapChain, CreateSwapChain,
                                    "IDXGIFactory::CreateSwapChain"sv);
            if (result == false)
            {
                return false;
            }
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

            bool result = HookWrap (Original_IDXGIFactory::CreateSwapChainForHwnd, CreateSwapChainForHwnd,
                                    "IDXGIFactory2::CreateSwapChainForHwnd"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_IDXGIFactory::CreateSwapChainForCoreWindow == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory2);

            Original_IDXGIFactory::CreateSwapChainForCoreWindow = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChainForCoreWindow> (
                vtable[static_cast<int>(IDXGIFactory2_VTable_ID::CreateSwapChainForCoreWindow)]);

            bool result = HookWrap (Original_IDXGIFactory::CreateSwapChainForCoreWindow, CreateSwapChainForCoreWindow,
                                    "IDXGIFactory2::CreateSwapChainForCoreWindow"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_IDXGIFactory::CreateSwapChainForComposition == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (factory2);

            Original_IDXGIFactory::CreateSwapChainForComposition = reinterpret_cast<Original_IDXGIFactory::pfn_CreateSwapChainForComposition> (
                vtable[static_cast<int>(IDXGIFactory2_VTable_ID::CreateSwapChainForComposition)]);

            bool result = HookWrap (Original_IDXGIFactory::CreateSwapChainForComposition, CreateSwapChainForComposition,
                                    "IDXGIFactory2::CreateSwapChainForComposition"sv);
            if (result == false)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

}

// ----------------------------------------------------------------------------