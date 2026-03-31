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
        (void) command_list;
        (void) desc;
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
        (void) command_list;
        (void) pBuildParams;
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
        (void) This;
        (void) NumCommandLists;
        (void) ppCommandLists;
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