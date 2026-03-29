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

/// @brief Track virtual addresses of 'ID3D12Resource's for reverse lookup
class VirtualAddressTracker
{
public:

    /// @brief Add a GPU virtual address associated with a resource to virtual
    ///        address map
    ///
    /// @param resource Resource associated with the GPU virtual address
    /// @param addr GPU virtual address
    void Add (ID3D12Resource* resource, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        if (resource == nullptr || resource->GetDesc ().Width == 0 || addr == 0)
        {
            return;
        }

        auto& aliased_map = virtual_address_map_[addr];
        auto& end_addr = aliased_map[resource];
        end_addr = addr + resource->GetDesc ().Width;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the resource associated with a GPU virtual address
    ///
    /// @param addr GPU virtual address
    /// @param minimum_size Minimum size of resource to be considered a match
    /// @return The resource associated with the GPU virtual address, or
    ///         'nullptr' if not found
    [[nodiscard]] ID3D12Resource* Get (D3D12_GPU_VIRTUAL_ADDRESS addr, UINT64 minimum_size)
    {
        if (addr == 0)
        {
            return nullptr;
        }

        // Map uses 'std::greater', so 'lower_bound' returns the first entry
        // with an address less than or equal to the searched address
        auto it = virtual_address_map_.lower_bound (addr);

        while (it != virtual_address_map_.end ())
        {
            // Check all aliased resources at this start address
            if (ID3D12Resource* resource = FindMatch (it->second, addr, minimum_size))
            {
                return resource;
            }

            // If the search address did not fall within the bounds of any
            // aliased resource at this start address, move to the next lower
            // start address and check again, as there may be larger resources
            // that also alias this address
            ++it;
        }

        RAYBENCH_LOG_WARNING ("Failed to find resource for GPU virtual address 0x{:016X} with minimum size {}!",
                              addr, minimum_size);
        return nullptr;
    }

private:

    // ========================================================================

    ///< Map of aliased resources and their end addresses that share the same
    ///  GPU virtual address
    using AliasedVirtualAddressMap = std::map<ID3D12Resource*, UINT64>;
    ///< Map of GPU virtual address to aliased resources sorted in descending
    ///  order
    std::map<D3D12_GPU_VIRTUAL_ADDRESS, AliasedVirtualAddressMap, std::greater<D3D12_GPU_VIRTUAL_ADDRESS>> virtual_address_map_;

    // ========================================================================

    /// @brief Find a resource in the aliased resources map that matches search
    ///        address and minimum size criteria
    ///
    /// @param aliased_resources Map of resources and their end addresses that
    ///                          share the same GPU virtual address
    /// @param search_addr GPU virtual address to search for
    /// @param minimum_size Minimum size of resource to be considered a match
    /// @return The resource that matches search criteria, or 'nullptr' if not
    ///         found
    [[nodiscard]] ID3D12Resource* FindMatch (const AliasedVirtualAddressMap& aliased_resources,
                                             D3D12_GPU_VIRTUAL_ADDRESS search_addr,
                                             UINT64 minimum_size) const
    {
        for (const auto& [resource, end_addr] : aliased_resources)
        {
            // Check if the search address is within the resource's address
            // range and if the resource size satisfies the minimum size
            if (search_addr < end_addr && (search_addr + minimum_size) <= end_addr)
            {
                return resource;
            }
        }

        return nullptr;
    }
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