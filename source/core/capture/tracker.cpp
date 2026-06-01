// ============================================================================

/// @brief State tracker

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

#include "util/log.h"

export module RayBench.Capture:Tracker;

import std;
import :AS;
import :Resource;
import RayBench.Util;

namespace raybench::capture
{

/// @brief State tracker
export class Tracker
{
    friend class Writer;

public:

    /// @brief Track the release of 'ID3D12Resource' and remove its GPU virtual
    ///        address
    ///
    /// @param This Resource being released
    /// @param addr GPU virtual address associated with resource being released
    void TrackRelease (ID3D12Resource* This, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        std::unique_lock lock (state_mutex_);
        virtual_address_tracker_.Remove (This, addr);
    }

    // ========================================================================

    /// @brief Track GPU virtual address associated with resource for reverse
    ///        lookup
    ///
    /// @param resource Resource associated with the GPU virtual address
    /// @param addr GPU virtual address
    void TrackVirtualAddress (ID3D12Resource* resource, D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        std::unique_lock lock (state_mutex_);
        virtual_address_tracker_.Add (resource, addr);
    }

    // ========================================================================

    /// @brief Track the creation of 'IDXGISwapChain' for potential future
    ///        capture of state using this swap chain's command queue
    ///
    /// @param pDevice Command queue (guaranteed in DirectX 12)
    /// @param ppSwapChain Created swap chain
    void TrackSwapChain (IUnknown* pDevice, IDXGISwapChain** ppSwapChain)
    {
        ID3D12CommandQueue* command_queue = nullptr;
        HRESULT hr = pDevice->QueryInterface (IID_PPV_ARGS (&command_queue));
        if (FAILED (hr))
        {
            RAYBENCH_LOG_ERROR ("Failed to query command queue from swap chain device: 0x{:08X}!", hr);
            return;
        }

        {
            std::unique_lock lock (state_mutex_);
            swap_chain_command_queue_map_[*ppSwapChain] = command_queue;
        }
    }

    // TODO: Proper tracker!
    std::unordered_map<IDXGISwapChain*, ID3D12CommandQueue*> swap_chain_command_queue_map_;

    // ========================================================================

    /// @brief Track the creation of 'ID3D12Resource' and initialize its state
    ///
    /// @param device Device used to create the resource
    /// @param resource Resource
    /// @param initial_state Initial state of the resource
    void TrackResourceCreation (ID3D12Device* device, ID3D12Resource* resource, D3D12_RESOURCE_STATES initial_state)
    {
        (void) device;
        (void) resource;
        (void) initial_state;
    }

    // ------------------------------------------------------------------------

    /// @brief Track a resource barrier on a specific command list
    ///
    /// @param command_list Command list the barrier is recorded on
    /// @param num_barriers Number of barriers
    /// @param barriers Pointer to the barriers
    void TrackResourceBarrier (ID3D12GraphicsCommandList* command_list, UINT num_barriers, const D3D12_RESOURCE_BARRIER* barriers)
    {
        (void) command_list;
        (void) num_barriers;
        (void) barriers;
    }

    // ========================================================================

    /// @brief Track an acceleration structure build and stage necessary copies
    ///        for build
    ///
    /// @param command_list Command list the build is recorded on
    /// @param desc Description of the acceleration structure build
    void TrackASBuild (ID3D12GraphicsCommandList4* command_list,
                       const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc)
    {
        if (desc == nullptr)
        {
            RAYBENCH_LOG_ERROR ("Acceleration structure build description is null!");
            return;
        }

        ID3D12Resource* resource = nullptr;
        ID3D12Device5* device = nullptr;
        HRESULT hr = command_list->GetDevice (IID_PPV_ARGS (&device));
        if (FAILED (hr))
        {
            RAYBENCH_LOG_ERROR ("Failed to get device from command list: 0x{:08X}!", hr);
            return;
        }

        // Retrieve destination resource from GPU virtual address
        AccelerationStructureTracker::BuildInfo build_info {};
        build_info.dest_addr = desc->DestAccelerationStructureData;
        build_info.inputs = desc->Inputs;
        build_info.dest_size = build_info.GetMaximumSizeAS (device);
        if (build_info.dest_size == 0)
        {
            RAYBENCH_LOG_ERROR ("Failed to get prebuild info for acceleration structure build!");
            device->Release ();
            return;
        }

        {
            std::scoped_lock lock (state_mutex_);
            resource = virtual_address_tracker_.Get (build_info.dest_addr, build_info.dest_size);
            if (resource == nullptr)
            {
                device->Release ();
                return;
            }
        }

        // Store build inputs for later retrieval during command list execution
        build_info.CopyGeometryDescs (resource);
        UINT64 inputs_size = 0;
        std::vector<AccelerationStructureTracker::InputsEntry> inputs_entries;
        inputs_size = build_info.CopyBuildInputs (inputs_entries);

        if (inputs_size == 0)
        {
            // No geometry data to copy
            device->Release ();
            return;
        }

        build_info.copyback_size = inputs_size;

        // Create copyback buffer for build inputs to be retrieved during
        // command list execution.  Sort entries by destination address to
        // optimize retrieval during command list execution
        std::sort (inputs_entries.begin (), inputs_entries.end (), [] (
            const AccelerationStructureTracker::InputsEntry& a,
            const AccelerationStructureTracker::InputsEntry& b)
            {
                if (a.src_addr == nullptr || b.src_addr == nullptr)
                {
                    return a.src_addr < b.src_addr;
                }
                return *a.src_addr < *b.src_addr;
            });

        bool copyback_resource_created = build_info.CreateCopybackResource (device);
        if (copyback_resource_created == false)
        {
            device->Release ();
            return;
        }

        // Stage build inputs copies to copyback buffer to be executed during
        // command list execution
        auto entry_it = inputs_entries.begin ();
        while (entry_it != inputs_entries.end ())
        {
            if (entry_it->src_addr == nullptr || *entry_it->src_addr == 0)
            {
                ++entry_it;
                continue;
            }

            ID3D12Resource* src_resource = nullptr;
            {
                std::scoped_lock lock (state_mutex_);
                src_resource = virtual_address_tracker_.Get (*entry_it->src_addr, entry_it->size);
                if (src_resource == nullptr)
                {
                    RAYBENCH_LOG_ERROR ("Failed to retrieve GPU virtual address for build input resource: 0x{:016X}!",
                                        *entry_it->src_addr);
                    ++entry_it;
                    continue;
                }
            }

            // Compute the source address range to batch copies from the same
            // resource without redundant lookups
            const D3D12_GPU_VIRTUAL_ADDRESS src_start = src_resource->GetGPUVirtualAddress ();
            const D3D12_GPU_VIRTUAL_ADDRESS src_end = src_start + src_resource->GetDesc ().Width;

            while (entry_it != inputs_entries.end ())
            {
                if (entry_it->src_addr == nullptr || *entry_it->src_addr == 0)
                {
                    ++entry_it;
                    continue;
                }

                // Check if entry falls within the current source address range
                const D3D12_GPU_VIRTUAL_ADDRESS entry_addr = *entry_it->src_addr;
                const D3D12_GPU_VIRTUAL_ADDRESS entry_end = entry_addr + entry_it->size;
                if (entry_addr < src_start || entry_end > src_end)
                {
                    break;
                }

                command_list->CopyBufferRegion (build_info.copyback_resource, entry_it->offset,
                                                src_resource, entry_addr - src_start, entry_it->size);
                ++entry_it;
            }
        }

        {
            std::scoped_lock lock (state_mutex_);
            as_tracker_.Add (command_list, build_info);
        }

        device->Release ();
    }

    // ------------------------------------------------------------------------

    /// @brief Track an acceleration structure build and stage necessary copies
    ///        for build
    ///
    /// @param command_list Command list the build is recorded on
    /// @param pBuildParams Description of the acceleration structure build
    void TrackASBuild (ID3D12GraphicsCommandList4* command_list,
                       const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pBuildParams)
    {
        if (pBuildParams == nullptr || pBuildParams->pDesc == nullptr)
        {
            RAYBENCH_LOG_ERROR ("Acceleration structure build description is null!");
            return;
        }

        ID3D12Resource* resource = nullptr;
        ID3D12Device5* device = nullptr;
        HRESULT hr = command_list->GetDevice (IID_PPV_ARGS (&device));
        if (FAILED (hr))
        {
            RAYBENCH_LOG_ERROR ("Failed to get device from command list: 0x{:08X}!", hr);
            return;
        }

        // Retrieve destination resource from GPU virtual address
        AccelerationStructureTracker::BuildInfo build_info {};
        build_info.dest_addr = pBuildParams->pDesc->destAccelerationStructureData;
        build_info.inputs = pBuildParams->pDesc->inputs;
        build_info.dest_size = build_info.GetMaximumSizeAS (device);
        if (build_info.dest_size == 0)
        {
            RAYBENCH_LOG_ERROR ("Failed to get prebuild info for acceleration structure build!");
            device->Release ();
            return;
        }

        {
            std::scoped_lock lock (state_mutex_);
            resource = virtual_address_tracker_.Get (build_info.dest_addr, build_info.dest_size);
            if (resource == nullptr)
            {
                device->Release ();
                return;
            }
        }

        // Store build inputs for later retrieval during command list execution
        build_info.CopyGeometryDescs (resource);
        UINT64 inputs_size = 0;
        std::vector<AccelerationStructureTracker::InputsEntry> inputs_entries;
        inputs_size = build_info.CopyBuildInputs (inputs_entries);

        if (inputs_size == 0)
        {
            // No geometry data to copy
            device->Release ();
            return;
        }

        build_info.copyback_size = inputs_size;

        // Create copyback buffer for build inputs to be retrieved during
        // command list execution.  Sort entries by destination address to
        // optimize retrieval during command list execution
        std::sort (inputs_entries.begin (), inputs_entries.end (), [] (
            const AccelerationStructureTracker::InputsEntry& a,
            const AccelerationStructureTracker::InputsEntry& b)
            {
                if (a.src_addr == nullptr || b.src_addr == nullptr)
                {
                    return a.src_addr < b.src_addr;
                }
                return *a.src_addr < *b.src_addr;
            });

        bool copyback_resource_created = build_info.CreateCopybackResource (device);
        if (copyback_resource_created == false)
        {
            device->Release ();
            return;
        }

        // Stage build inputs copies to copyback buffer to be executed during
        // command list execution
        auto entry_it = inputs_entries.begin ();
        while (entry_it != inputs_entries.end ())
        {
            if (entry_it->src_addr == nullptr || *entry_it->src_addr == 0)
            {
                ++entry_it;
                continue;
            }

            ID3D12Resource* src_resource = nullptr;
            {
                std::scoped_lock lock (state_mutex_);
                src_resource = virtual_address_tracker_.Get (*entry_it->src_addr, entry_it->size);
                if (src_resource == nullptr)
                {
                    RAYBENCH_LOG_ERROR ("Failed to retrieve GPU virtual address for build input resource: 0x{:016X}!",
                                        *entry_it->src_addr);
                    ++entry_it;
                    continue;
                }
            }

            // Compute the source address range to batch copies from the same
            // resource without redundant lookups
            const D3D12_GPU_VIRTUAL_ADDRESS src_start = src_resource->GetGPUVirtualAddress ();
            const D3D12_GPU_VIRTUAL_ADDRESS src_end = src_start + src_resource->GetDesc ().Width;

            while (entry_it != inputs_entries.end ())
            {
                if (entry_it->src_addr == nullptr || *entry_it->src_addr == 0)
                {
                    ++entry_it;
                    continue;
                }

                // Check if entry falls within the current source address range
                const D3D12_GPU_VIRTUAL_ADDRESS entry_addr = *entry_it->src_addr;
                const D3D12_GPU_VIRTUAL_ADDRESS entry_end = entry_addr + entry_it->size;
                if (entry_addr < src_start || entry_end > src_end)
                {
                    break;
                }

                command_list->CopyBufferRegion (build_info.copyback_resource, entry_it->offset,
                                                src_resource, entry_addr - src_start, entry_it->size);
                ++entry_it;
            }
        }

        {
            std::scoped_lock lock (state_mutex_);
            as_tracker_.Add (command_list, build_info);
        }

        device->Release ();
    }

    // ========================================================================

    /// @brief Track execution of command lists and commit their pending
    ///        resource transitions to the resource state tracker
    ///
    /// @param This Command queue executing the command lists
    /// @param NumCommandLists Number of command lists being executed
    /// @param pp_command_lists Pointer to the command lists being executed
    void TrackExecuteCommandLists (ID3D12CommandQueue* This, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
    {
        std::unique_lock lock (state_mutex_);
        for (UINT i = 0; i < NumCommandLists; ++i)
        {
            ID3D12GraphicsCommandList* command_list = nullptr;
            if (SUCCEEDED (ppCommandLists[i]->QueryInterface (IID_PPV_ARGS (&command_list))))
            {
                as_tracker_.Commit (This, command_list);
                command_list->Release ();
            }
        }
    }

private:

    // ========================================================================

    ///< Mutex for synchronizing access to the capture state
    mutable std::shared_mutex state_mutex_;
    ///< Acceleration structure tracker
    AccelerationStructureTracker as_tracker_;
    ///< Map of GPU virtual addresses for reverse lookup
    VirtualAddressTracker virtual_address_tracker_;
};

}

// ----------------------------------------------------------------------------