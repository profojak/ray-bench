// ============================================================================

/// @brief 'ID3D12Resource' state trackers and related utilities

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>

#include "util/log.h"

export module RayBench.Capture:Resource;

import std;
import RayBench.Util;

namespace raybench::capture
{

/// @brief State of 'ID3D12Resource'
struct ResourceState
{
    ///< State of each subresource of 'ID3D12Resource'
    std::vector<D3D12_RESOURCE_STATES> subresource_states;
};

// ============================================================================

class PendingTransitionTracker;

/// @brief Track state of 'ID3D12Resource's
class ResourceStateTracker
{
    friend class PendingTransitionTracker;

public:

    /// @brief Update the state of a resource
    void Update (ID3D12Resource* resource, const ResourceState& state)
    {
        std::unique_lock lock (mutex_);
        state_map_[resource] = state;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the state of a resource
    [[nodiscard]] std::optional<ResourceState> Get (ID3D12Resource* resource)
    {
        std::shared_lock lock (mutex_);
        auto it = state_map_.find (resource);
        if (it != state_map_.end ())
        {
            return it->second;
        }
        else
        {
            return std::nullopt;
        }
    }

private:

    // ========================================================================

    ///< Mutex to protect access to the resource state map
    std::shared_mutex mutex_;
    ///< Map of 'ID3D12Resource' to their state
    std::unordered_map<ID3D12Resource*, ResourceState> state_map_;
};

// ============================================================================

/// @brief Track pending resource transitions for command lists
class PendingTransitionTracker
{
public:

    /// @brief Update the pending resource transitions of a command list with
    ///        new barriers
    ///
    /// @param command_list Command list the barriers are recorded on
    /// @param num_barriers Number of barriers
    /// @param barriers Pointer to the barriers
    void Update (ID3D12GraphicsCommandList* command_list, UINT num_barriers, const D3D12_RESOURCE_BARRIER* barriers)
    {
        std::unique_lock lock (mutex_);
        auto& cl_transitions = pending_transitions_[command_list];

        for (UINT i = 0; i < num_barriers; ++i)
        {
            const auto& barrier = barriers[i];
            if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
            {
                const auto& transition = barrier.Transition;
                cl_transitions.push_back ({
                    .resource = transition.pResource,
                    .subresource = transition.Subresource,
                    .state_after = transition.StateAfter
                });
            }
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Commit the pending resource transitions of a command list to the
    ///        resource state tracker and clear the pending transitions
    ///
    /// @param command_list Command list to commit
    /// @param resource_state_tracker Resource state tracker to update
    void Commit (ID3D12GraphicsCommandList* command_list, ResourceStateTracker& resource_state_tracker)
    {
        std::unique_lock lock (mutex_);
        auto it = pending_transitions_.find (command_list);
        if (it == pending_transitions_.end ())
        {
            return;
        }

        std::unique_lock state_lock (resource_state_tracker.mutex_);
        const auto& cl_transitions = it->second;
        for (const auto& transition : cl_transitions)
        {
            ID3D12Resource* resource = transition.resource;
            UINT subresource = transition.subresource;
            D3D12_RESOURCE_STATES state_after = transition.state_after;

            auto state_it = resource_state_tracker.state_map_.find (resource);
            if (state_it == resource_state_tracker.state_map_.end ())
            {
                // TODO: Still missing some resource hooks
                // (e.g., 'IDXGISwapChain::GetBuffer')
                RAYBENCH_LOG_ERROR_ONCE ("Resource not found in state tracker when committing pending transitions!");
                continue;
            }

            auto& resource_state = state_it->second;
            if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            {
                for (auto& state : resource_state.subresource_states)
                {
                    state = state_after;
                }
            }
            else if (subresource >= resource_state.subresource_states.size ())
            {
                RAYBENCH_LOG_ERROR_ONCE ("Invalid subresource index when committing pending transitions!");
                continue;
            }
            else
            {
                resource_state.subresource_states[subresource] = state_after;
            }
        }
        pending_transitions_.erase (it);
    }

private:

    // ========================================================================

    /// @brief Pending resource transition
    struct PendingTransition
    {
        ///< Resource to transition
        ID3D12Resource* resource;     
        ///< Subresource index, or all subresources
        UINT subresource;
        ///< Final state
        D3D12_RESOURCE_STATES state_after;
    };

    ///< Mutex to protect access to the pending transitions map
    std::shared_mutex mutex_;
    ///< Map of command list to its sequence of pending resource transitions
    std::unordered_map<ID3D12GraphicsCommandList*, std::vector<PendingTransition>> pending_transitions_;
};

}

// ----------------------------------------------------------------------------