// ============================================================================

/// @brief Resource state tracker

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

/// @brief Track virtual addresses of resources for reverse lookup
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
        const UINT64 resource_size = resource ? resource->GetDesc ().Width : 0;
        if (resource_size == 0 || addr == 0)
        {
            return;
        }

        auto& aliased_map = virtual_address_map_[addr];
        auto& end_addr = aliased_map[resource];
        end_addr = addr + resource_size;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the resource associated with a GPU virtual address
    ///
    /// @param addr GPU virtual address
    /// @param minimum_size Minimum size of resource to be considered a match
    /// @return The resource associated with the GPU virtual address, or
    ///         'nullptr' if not found
    [[nodiscard]] ID3D12Resource* Get (D3D12_GPU_VIRTUAL_ADDRESS addr, UINT64 minimum_size) const
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

    // ------------------------------------------------------------------------

    /// @brief Remove a resource from the virtual address map
    ///
    /// @param resource Resource to remove from the virtual address map
    /// @param addr GPU virtual address associated with the resource
    void Remove (ID3D12Resource* resource, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        auto it = virtual_address_map_.find (addr);
        if (it != virtual_address_map_.end ())
        {
            auto& aliased_map = it->second;
            aliased_map.erase (resource);
            if (aliased_map.empty ())
            {
                virtual_address_map_.erase (it);
            }
        }
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

}

// ----------------------------------------------------------------------------