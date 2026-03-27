// ============================================================================

/// @brief Capture state

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>

#include "util/log.h"

export module RayBench.Capture:State;

import std;
import RayBench.Util;

namespace raybench::capture
{

/// @brief Map of resources to their GPU virtual addresses for reverse lookup
export class ResourceMap
{
public:

    /// @brief Resource information
    struct ResourceInfo
    {
        ///< Associated resource
        ID3D12Resource* resource {nullptr};
        ///< GPU virtual address of the end of resource address range
        D3D12_GPU_VIRTUAL_ADDRESS end_addr {0};
        ///< Resource state
        D3D12_RESOURCE_STATES state {D3D12_RESOURCE_STATE_COMMON};
    };

    // ========================================================================

    /// @brief Add a resource and its associated GPU virtual address to the map
    ///
    /// @param resource Resource information
    /// @param addr GPU virtual address
    void AddResource (ResourceInfo resource_info, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        const auto& resource = resource_info.resource;
        if (resource == nullptr || resource->GetDesc ().Width == 0 || addr == 0)
        {
            return;
        }

        virtual_map_[addr][resource] = resource_info;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the resource associated with a GPU virtual address
    ///
    /// @param addr GPU virtual address
    /// @param minimum_size Minimum size of resource to be considered a match
    /// @return Optional containing the resource info if found
    [[nodiscard]] std::optional<ResourceInfo> GetResource (D3D12_GPU_VIRTUAL_ADDRESS addr, UINT64 minimum_size) const
    {
        if (addr == 0)
        {
            return std::nullopt;
        }

        // Map uses `std::greater`, so `lower_bound` returns the first entry
        // with an address less than or equal to the searched address
        for (auto it = virtual_map_.lower_bound (addr); it != virtual_map_.end (); ++it)
        {
            // If the search address did not fall within the bounds of any
            // aliased resource at this start address, move to the next lower
            // start address and check again, as there may be larger resources
            // that also alias this address
            if (auto match = FindMatch (it->second, addr, minimum_size))
            {
                return match;
            }
        }

        RAYBENCH_LOG_WARNING ("Failed to find resource for GPU virtual address 0x{:016X} with minimum size {}!",
                              addr, minimum_size);
        return std::nullopt;
    }

private:

    // ========================================================================

    ///< Unordered map of aliased resources
    using AliasedVirtualMap = std::unordered_map<ID3D12Resource*, ResourceInfo>;
    ///< Map of GPU virtual addresses to aliased resources sorted in descending
    ///  order
    std::map<D3D12_GPU_VIRTUAL_ADDRESS, AliasedVirtualMap, std::greater<D3D12_GPU_VIRTUAL_ADDRESS>> virtual_map_;

    // ========================================================================

    /// @brief Helper to find a matching resource within the aliased map
    [[nodiscard]] std::optional<ResourceInfo> FindMatch (const AliasedVirtualMap& aliased_resources,
                                                         D3D12_GPU_VIRTUAL_ADDRESS search_addr,
                                                         UINT64 minimum_size) const
    {
        for (const auto& [resource, entry] : aliased_resources)
        {
            if (search_addr < entry.end_addr && (search_addr + minimum_size) <= entry.end_addr)
            {
                return std::make_optional (entry);
            }
        }
        return std::nullopt;
    }
};

// ============================================================================

/// @brief Map of acceleration structures to their build inputs
export class ASMap
{
public:

    /// @brief Acceleration structure build information
    struct BuildInfo
    {
        ///< GPU virtual address of the destination memory
        D3D12_GPU_VIRTUAL_ADDRESS dest_addr {0};
        ///< Size of the destination memory
        UINT64 dest_size {0};
        ///< Associated resource
        ID3D12Resource* dest_resource {nullptr};
        ///< Build inputs
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs {};
        ///< Build inputs geometry descriptions
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descs;
        ///< Size of the copyback buffer
        UINT64 copyback_size {0};
        ///< Copyback buffer resource
        ID3D12Resource* copyback_resource {nullptr};
    };

    /// @brief Raytracing acceleration structure build input
    struct BuildInput
    {
        ///< GPU virtual address of the inputs buffer
        const D3D12_GPU_VIRTUAL_ADDRESS* dest_addr {nullptr};
        ///< Size of the inputs entry in the inputs buffer
        UINT64 size {0};
        ///< Offset of the inputs entry in the inputs buffer
        UINT64 offset {0};
    };
};

}

// ----------------------------------------------------------------------------