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

/// @brief Track state of 'ID3D12Resource's
class ResourceStateTracker
{
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

}

// ----------------------------------------------------------------------------