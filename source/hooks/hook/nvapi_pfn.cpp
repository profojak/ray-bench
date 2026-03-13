// ============================================================================

/// @brief NVAPI function pointers

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <nvapi/nvapi.h>

export module RayBench.Hook:NVAPI.Pfn;

namespace raybench::hook
{

// ============================================================================

using pfn_NvAPI_Initialize = NvAPI_Status (WINAPI*)();

pfn_NvAPI_Initialize Original_NvAPI_Initialize = nullptr;

// ----------------------------------------------------------------------------

using pfn_NvAPI_QueryInterface = void* (WINAPI*) (NvU32);

pfn_NvAPI_QueryInterface Original_NvAPI_QueryInterface = nullptr;

}

// ----------------------------------------------------------------------------