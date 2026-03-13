// ============================================================================

/// @brief DirectX 12 function pointers

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>
#include <d3d12.h>

export module RayBench.Hook:DirectX12.Pfn;

namespace raybench::hook
{

// ============================================================================

using pfn_D3D12GetInterface = HRESULT (WINAPI*)(REFCLSID, REFIID, void**);

pfn_D3D12GetInterface Original_D3D12GetInterface = nullptr;

// ----------------------------------------------------------------------------

using pfn_D3D12CreateDevice = HRESULT (WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

pfn_D3D12CreateDevice Original_D3D12CreateDevice = nullptr;

// ----------------------------------------------------------------------------

using pfn_CreateDXGIFactory = HRESULT (WINAPI*) (REFIID, void**);
using pfn_CreateDXGIFactory1 = HRESULT (WINAPI*) (REFIID, void**);
using pfn_CreateDXGIFactory2 = HRESULT (WINAPI*) (UINT, REFIID, void**);

pfn_CreateDXGIFactory Original_CreateDXGIFactory = nullptr;
pfn_CreateDXGIFactory1 Original_CreateDXGIFactory1 = nullptr;
pfn_CreateDXGIFactory2 Original_CreateDXGIFactory2 = nullptr;

}

// ----------------------------------------------------------------------------