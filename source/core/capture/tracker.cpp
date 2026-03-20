// ============================================================================

/// @brief Capture tracker

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>

#include "util/log.h"

export module RayBench.Capture:Tracker;

import std;
import :State;
import RayBench.Util;

namespace raybench::capture
{

/// @brief Capture tracker
export class Tracker
{
public:

    void GetGPUVirtualAddress (ID3D12Resource* resource, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        std::scoped_lock<std::mutex> lock (state_mutex_);
        state_.AddVirtualAddress (resource, addr);
    }

    // ========================================================================

    void BuildRaytracingAccelerationStructure (
        ID3D12GraphicsCommandList4* command_list,
        const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc
    )
    {
        ID3D12Resource* resource = nullptr;
        ID3D12Device5* device = nullptr;
        HRESULT hr = command_list->GetDevice (IID_PPV_ARGS (&device));
        if (FAILED (hr))
        {
            RAYBENCH_LOG_ERROR ("Failed to get device from command list: 0x{:08X}!", hr);
            return;
        }

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo (&desc->Inputs, &prebuild_info);

        {
            std::scoped_lock<std::mutex> lock (state_mutex_);
            bool result = state_.GetVirtualAddress (resource, desc->DestAccelerationStructureData, prebuild_info.ResultDataMaxSizeInBytes);
            if (result == false)
            {
                return;
            }
        }
    }

private:

    // ========================================================================

    ///< Capture state
    State state_;
    ///< Mutex for synchronizing access to the capture state
    std::mutex state_mutex_;
};

}

// ----------------------------------------------------------------------------