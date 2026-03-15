// ============================================================================

/// @brief DXGI function pointers

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi1_6.h>
#include <dxgi.h>

export module RayBench.Hook:DXGI.Pfn;

namespace raybench::hook
{

namespace Original_IDXGIFactory
{
using pfn_CreateSwapChain = HRESULT (WINAPI*) (IDXGIFactory*,
                                               IUnknown*,
                                               DXGI_SWAP_CHAIN_DESC*,
                                               IDXGISwapChain**);

pfn_CreateSwapChain CreateSwapChain = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateSwapChainForHwnd = HRESULT (WINAPI*) (IDXGIFactory2*,
                                                      IUnknown*,
                                                      HWND,
                                                      const DXGI_SWAP_CHAIN_DESC1*,
                                                      const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,
                                                      IDXGIOutput*,
                                                      IDXGISwapChain1**);

pfn_CreateSwapChainForHwnd CreateSwapChainForHwnd = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateSwapChainForCoreWindow = HRESULT (WINAPI*) (IDXGIFactory2*,
                                                            IUnknown*,
                                                            IUnknown*,
                                                            const DXGI_SWAP_CHAIN_DESC1*,
                                                            IDXGIOutput*,
                                                            IDXGISwapChain1**);

pfn_CreateSwapChainForCoreWindow CreateSwapChainForCoreWindow = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateSwapChainForComposition = HRESULT (WINAPI*) (IDXGIFactory2*,
                                                             IUnknown*,
                                                             const DXGI_SWAP_CHAIN_DESC1*,
                                                             IDXGIOutput*,
                                                             IDXGISwapChain1**);

pfn_CreateSwapChainForComposition CreateSwapChainForComposition = nullptr;
}

// ============================================================================

using pfn_CreateDXGIFactory = HRESULT (WINAPI*) (REFIID, void**);
using pfn_CreateDXGIFactory1 = HRESULT (WINAPI*) (REFIID, void**);
using pfn_CreateDXGIFactory2 = HRESULT (WINAPI*) (UINT, REFIID, void**);

pfn_CreateDXGIFactory Original_CreateDXGIFactory = nullptr;
pfn_CreateDXGIFactory1 Original_CreateDXGIFactory1 = nullptr;
pfn_CreateDXGIFactory2 Original_CreateDXGIFactory2 = nullptr;

}

// ----------------------------------------------------------------------------