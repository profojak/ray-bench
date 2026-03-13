// ============================================================================

/// @brief NVAPI function pointers

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

export module RayBench.Hook:NVAPI.Pfn;

namespace raybench::hook
{

// ============================================================================

using pfn_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx = NvAPI_Status (WINAPI*) (
    ID3D12GraphicsCommandList4*,
    const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS*);

pfn_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx Original_NvAPI_D3D12_BuildRaytracingAccelerationStructureEx = nullptr;

// ----------------------------------------------------------------------------

using pfn_NvAPI_DirectD3D12GraphicsCommandList_Create = NvAPI_Status (WINAPI*) (
    ID3D12GraphicsCommandList*,
    INvAPI_DirectD3D12GraphicsCommandList**);

pfn_NvAPI_DirectD3D12GraphicsCommandList_Create Original_NvAPI_DirectD3D12GraphicsCommandList_Create = nullptr;

// ----------------------------------------------------------------------------

using pfn_NvAPI_Initialize = NvAPI_Status (WINAPI*)();

pfn_NvAPI_Initialize Original_NvAPI_Initialize = nullptr;

// ----------------------------------------------------------------------------

using pfn_NvAPI_QueryInterface = void* (WINAPI*) (NvU32);

pfn_NvAPI_QueryInterface Original_NvAPI_QueryInterface = nullptr;

}

// ----------------------------------------------------------------------------