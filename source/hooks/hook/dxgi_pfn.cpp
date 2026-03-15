// ============================================================================

/// @brief DXGI function pointers

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>

export module RayBench.Hook:DXGI.Pfn;

namespace raybench::hook
{

namespace Original_IDXGIFactory
{

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