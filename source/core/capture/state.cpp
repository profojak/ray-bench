// ============================================================================

/// @brief Capture state

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>

#include "util/log.h"

export module RayBench.Capture:State;

import std;
import RayBench.Util;

namespace raybench::capture
{

/// @brief Capture state
export class State
{
public:

    /// @brief Add a GPU virtual address associated with a resource to virtual
    ///        address map
    ///
    /// @param resource Resource associated with the GPU virtual address
    /// @param addr GPU virtual address
    void AddVirtualAddress (ID3D12Resource* resource, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        if (resource == nullptr || resource->GetDesc ().Width == 0 || addr == 0)
        {
            return;
        }

        auto& aliased_map = virtual_map_[addr];
        auto& end_addr = aliased_map[resource];
        end_addr = addr + resource->GetDesc ().Width;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the resource associated with a GPU virtual address
    ///
    /// @param resource Output parameter for resource associated with the GPU
    /// @param addr GPU virtual address
    /// @param minimum_size Minimum size of resource to be considered a match
    /// @return True if the resource was found, false otherwise
    bool GetVirtualAddress (ID3D12Resource* resource, D3D12_GPU_VIRTUAL_ADDRESS addr, UINT64 minimum_size)
    {
        if (addr == 0)
        {
            return false;
        }

        // Map uses `std::greater`, so `lower_bound` returns the first entry
        // with an address less than or equal to the searched address
        auto it = virtual_map_.lower_bound (addr);

        while (it != virtual_map_.end ())
        {
            // Check all aliased resources at this start address
            if (FindMatch (it->second, addr, minimum_size, resource))
            {
                return true;
            }

            // If the search address did not fall within the bounds of any
            // aliased resource at this start address, move to the next lower
            // start address and check again, as there may be larger resources
            // that also alias this address
            ++it;
        }

        RAYBENCH_LOG_WARNING ("Failed to find resource for GPU virtual address 0x{:016X} with minimum size {}!",
                              addr, minimum_size);
        return false;
    }

private:

    // ========================================================================

    ///< Map of aliased resources and their end addresses that share the same
    ///  GPU virtual address
    using AliasedVirtualMap = std::map<ID3D12Resource*, UINT64>;
    ///< Map of GPU virtual addresses to aliased resources sorted in descending
    ///  order
    using VirtualMap = std::map<D3D12_GPU_VIRTUAL_ADDRESS, AliasedVirtualMap, std::greater<D3D12_GPU_VIRTUAL_ADDRESS>>;

    ///< Map of GPU virtual addresses
    VirtualMap virtual_map_;

    // ========================================================================

    /// @brief Find a resource in the aliased resources map that matches search
    ///        address and minimum size criteria
    ///
    /// @param aliased_resources Map of resources and their end addresses that
    ///                          share the same GPU virtual address
    /// @param search_addr GPU virtual address to search for
    /// @param minimum_size Minimum size of resource to be considered a match
    /// @param out_resource Output parameter for resource that matches search
    /// @return True if a matching resource was found, false otherwise
    bool FindMatch (const AliasedVirtualMap& aliased_resources,
                    D3D12_GPU_VIRTUAL_ADDRESS search_addr,
                    UINT64 minimum_size,
                    ID3D12Resource*& out_resource) const
    {
        for (const auto& [resource, end_addr] : aliased_resources)
        {
            // Check if the search address is within the resource's address
            // range and if the resource size satisfies the minimum size
            if (search_addr < end_addr && (search_addr + minimum_size) <= end_addr)
            {
                out_resource = resource;
                return true;
            }
        }

        return false;
    }
};

}

// ----------------------------------------------------------------------------