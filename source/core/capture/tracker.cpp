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

    // ------------------------------------------------------------------------

    /// @brief Track a resource barrier on a specific command list
    ///
    /// @param command_list Command list the barrier is recorded on
    /// @param num_barriers Number of barriers
    /// @param barriers Pointer to the barriers
    void TrackResourceBarrier (ID3D12GraphicsCommandList* command_list, UINT num_barriers, const D3D12_RESOURCE_BARRIER* barriers)
    {
        pending_transition_tracker_.Update (command_list, num_barriers, barriers);
    }

    // ------------------------------------------------------------------------

    /// @brief Track execution of command lists and commit their pending
    ///        resource transitions to the resource state tracker
    ///
    /// @param num_command_lists Number of command lists being executed
    /// @param pp_command_lists Pointer to the command lists being executed
    void TrackExecuteCommandLists (UINT num_command_lists, ID3D12CommandList* const* pp_command_lists)
    {
        for (UINT i = 0; i < num_command_lists; ++i)
        {
            ID3D12GraphicsCommandList* command_list = nullptr;
            if (SUCCEEDED (pp_command_lists[i]->QueryInterface (IID_PPV_ARGS (&command_list))))
            {
                pending_transition_tracker_.Commit (command_list, resource_state_tracker_);
                command_list->Release ();
            }
        }
    }

private:

    // ========================================================================

    ///< 'ID3D12Resource' state tracker
    ResourceStateTracker resource_state_tracker_;
    ///< Pending resource transitions for command lists tracker
    PendingTransitionTracker pending_transition_tracker_;
};

}

// ----------------------------------------------------------------------------