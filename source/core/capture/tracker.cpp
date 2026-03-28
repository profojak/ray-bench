// ============================================================================

/// @brief State tracker

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
import :Resource;
import RayBench.Util;

namespace raybench::capture
{

/// @brief State tracker
export class Tracker
{
public:

    /// @brief Track the creation of 'ID3D12Resource' and initialize its state
    ///
    /// @param device Device used to create the resource
    /// @param resource Resource
    /// @param initial_state Initial state of the resource
    void TrackResourceCreation (ID3D12Device* device, ID3D12Resource* resource, D3D12_RESOURCE_STATES initial_state)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc ();
        UINT plane_count = 1;

        D3D12_FEATURE_DATA_FORMAT_INFO format_info = {desc.Format, 0};
        if (SUCCEEDED (device->CheckFeatureSupport (D3D12_FEATURE_FORMAT_INFO, &format_info, sizeof (format_info))))
        {
            plane_count = format_info.PlaneCount;
        }

        UINT num_subresources = 0;
        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            num_subresources = 1;
        }
        else
        {
            num_subresources = desc.MipLevels * plane_count;
            if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
            {
                num_subresources *= desc.DepthOrArraySize;
            }
        }

        ResourceState state;
        state.subresource_states.resize (num_subresources);
        for (auto& subresource_state : state.subresource_states)
        {
            subresource_state = initial_state;
        }

        resource_state_tracker_.Update (resource, state);
    }

private:

    // ========================================================================

    ///< 'ID3D12Resource' state tracker
    ResourceStateTracker resource_state_tracker_;
};

}

// ----------------------------------------------------------------------------