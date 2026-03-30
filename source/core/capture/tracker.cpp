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
import :AS;
import :Resource;
import RayBench.Util;

namespace raybench::capture
{

/// @brief State tracker
export class Tracker
{
public:

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

        ResourceStateTracker::ResourceState state;
        state.subresource_states.resize (num_subresources);
        for (auto& subresource_state : state.subresource_states)
        {
            subresource_state = initial_state;
        }

        std::unique_lock lock (state_mutex_);
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
        std::unique_lock lock (state_mutex_);
        pending_transition_tracker_.Update (command_list, num_barriers, barriers);
    }

    // ========================================================================

    void TrackASBuild (ID3D12GraphicsCommandList4* command_list, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc)
    {
        ID3D12Resource* resource = nullptr;
        ID3D12Device5* device = nullptr;
        HRESULT hr = command_list->GetDevice (IID_PPV_ARGS (&device));
        if (FAILED (hr))
        {
            RAYBENCH_LOG_ERROR ("Failed to get device from command list: 0x{:08X}!", hr);
            return;
        }

        // Retrieve destination resource from GPU virtual address
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo (&desc->Inputs, &prebuild_info);

        {
            std::scoped_lock lock (state_mutex_);
            resource = virtual_address_tracker_.Get (desc->DestAccelerationStructureData, prebuild_info.ResultDataMaxSizeInBytes);
            if (resource == nullptr)
            {
                return;
            }
        }

        // Store acceleration structure build information for later retrieval
        // during command list execution
        AccelerationStructureTracker::BuildInfo as_build {};
        as_build.dest_addr = desc->DestAccelerationStructureData;
        as_build.dest_size = prebuild_info.ResultDataMaxSizeInBytes;
        as_build.dest_resource = resource;
        as_build.inputs = desc->Inputs;

        if (desc->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
        {
            for (UINT i = 0; i < desc->Inputs.NumDescs; ++i)
            {
                as_build.geometry_descs.push_back (desc->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                                                   ? desc->Inputs.pGeometryDescs[i] : *desc->Inputs.ppGeometryDescs[i]);
            }

            // Clear pointers to avoid referencing invalid memory
            as_build.inputs.pGeometryDescs = nullptr;
            as_build.inputs.ppGeometryDescs = nullptr;
        }

        // Store build inputs for later retrieval during command list execution
        UINT64 inputs_size = 0;
        std::vector<AccelerationStructureTracker::InputsEntry> inputs_entries;

        if (as_build.inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
        {
            for (UINT i = 0; i < as_build.inputs.NumDescs; ++i)
            {
                const D3D12_RAYTRACING_GEOMETRY_DESC& geometry_desc = as_build.geometry_descs[i];
                if (geometry_desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)
                {
                    const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& triangles_desc = geometry_desc.Triangles;

                    // Transformation matrix
                    if (triangles_desc.Transform3x4)
                    {
                        constexpr UINT64 transform_size = 12 * sizeof (float);
                        inputs_size = raybench::util::AlignValue<D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT> (inputs_size);
                        inputs_entries.emplace_back (
                            AccelerationStructureTracker::InputsEntry {
                            &triangles_desc.Transform3x4,
                            transform_size,
                            inputs_size
                            });
                        inputs_size += transform_size;
                    }

                    // Index buffer
                    if (triangles_desc.IndexCount != 0)
                    {
                        UINT32 index_size = 0;
                        switch (triangles_desc.IndexFormat)
                        {
                            case DXGI_FORMAT_R32_UINT:
                                index_size = 4;
                                inputs_size = raybench::util::AlignValue<4> (inputs_size);
                                break;
                            case DXGI_FORMAT_R16_UINT:
                                index_size = 2;
                                inputs_size = raybench::util::AlignValue<2> (inputs_size);
                                break;
                            default:
                                RAYBENCH_LOG_ERROR ("Unsupported index format: {}!", static_cast<int>(triangles_desc.IndexFormat));
                                break;
                        }
                        const UINT index_buffer_size = triangles_desc.IndexCount * index_size;
                        inputs_entries.emplace_back (
                            AccelerationStructureTracker::InputsEntry {
                            &triangles_desc.IndexBuffer,
                            index_buffer_size,
                            inputs_size
                            });
                        inputs_size += index_buffer_size;
                    }

                    // Vertex buffer
                    if (triangles_desc.VertexCount != 0)
                    {
                        UINT64 vertex_size = triangles_desc.VertexCount * triangles_desc.VertexBuffer.StrideInBytes;
                        inputs_size = raybench::util::AlignValue<4> (inputs_size);
                        inputs_entries.emplace_back (
                            AccelerationStructureTracker::InputsEntry {
                            &triangles_desc.VertexBuffer.StartAddress,
                            vertex_size,
                            inputs_size
                            });
                        inputs_size += vertex_size;
                    }
                }
            }
        }
        else if (desc->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)
        {
            if (desc->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS)
            {
                RAYBENCH_LOG_WARNING_ONCE ("TLAS with array of pointers is not yet supported!");
                return;
            }
            else if (desc->Inputs.NumDescs > 0)
            {
                inputs_size = desc->Inputs.NumDescs * sizeof (D3D12_RAYTRACING_INSTANCE_DESC);
                inputs_entries.emplace_back (
                    AccelerationStructureTracker::InputsEntry {
                        &desc->Inputs.InstanceDescs,
                        inputs_size,
                        0
                    });
            }
        }
        else
        {
            RAYBENCH_LOG_ERROR ("Unsupported acceleration structure type: {}!", static_cast<int>(desc->Inputs.Type));
            return;
        }

        if (inputs_size == 0)
        {
            return;
        }

        as_build.copyback_size = inputs_size;

        // Create copyback buffer for build inputs to be retrieved during
        // command list execution.  Sort entries by destination address to
        // optimize retrieval during command list execution
        std::sort (inputs_entries.begin (), inputs_entries.end (), [] (
            const AccelerationStructureTracker::InputsEntry& a,
            const AccelerationStructureTracker::InputsEntry& b)
            {
                return *a.dest_addr < *b.dest_addr;
            });

        ID3D12Resource* copyback_resource = nullptr;

        D3D12_HEAP_PROPERTIES heap_properties {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resource_desc {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment = 0;
        resource_desc.Width = inputs_size;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = device->CreateCommittedResource (&heap_properties,
                                              D3D12_HEAP_FLAG_NONE,
                                              &resource_desc,
                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                              nullptr,
                                              IID_PPV_ARGS (&copyback_resource));
        if (FAILED (hr))
        {
            RAYBENCH_LOG_ERROR ("Failed to create copyback resource: 0x{:08X}!", hr);
            return;
        }
        as_build.copyback_resource = copyback_resource;

        // Stage build inputs copies to copyback buffer to be executed during
        // command list execution
        auto entry_it = inputs_entries.begin ();
        while (entry_it != inputs_entries.end ())
        {
            ID3D12Resource* src_resource = nullptr;
            {
                std::scoped_lock lock (state_mutex_);
                src_resource = virtual_address_tracker_.Get (*entry_it->dest_addr, entry_it->size);
                if (src_resource == nullptr)
                {
                    RAYBENCH_LOG_ERROR ("Failed to retrieve GPU virtual address for build input resource!");
                    ++entry_it;
                    continue;
                }
            }

            ResourceStateTracker::ResourceState src_state;
            const std::vector<PendingTransitionTracker::PendingTransition>* src_transitions = nullptr;
            std::optional<ResourceStateTracker::ResourceState> src_state_opt;

            {
                std::scoped_lock lock (state_mutex_);
                src_state_opt = resource_state_tracker_.Get (src_resource);
                if (src_state_opt.has_value () == false)
                {
                    RAYBENCH_LOG_ERROR ("Failed to retrieve global state for build input resource!");
                    ++entry_it;
                    continue;
                }
                src_state = src_state_opt.value ();
                src_transitions = pending_transition_tracker_.Get (command_list);
            }

            // Check if there are any pending state transitions for resource
            // on this command list and update the state if so
            if (src_transitions != nullptr)
            {
                for (const auto& transition : *src_transitions)
                {
                    if (transition.resource == src_resource)
                    {
                        if (transition.subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
                        {
                            for (auto& state : src_state.subresource_states)
                            {
                                state = transition.state_after;
                            }
                        }
                        else if (transition.subresource < src_state.subresource_states.size ())
                        {
                            src_state.subresource_states[transition.subresource] = transition.state_after;
                        }
                    }
                }
            }

            bool state_transition_needed = (src_state.subresource_states[0] & D3D12_RESOURCE_STATE_COPY_SOURCE) == 0;

            if (state_transition_needed)
            {
                D3D12_RESOURCE_TRANSITION_BARRIER pre_transition_barrier {};
                pre_transition_barrier.pResource = src_resource;
                pre_transition_barrier.Subresource = 0;
                pre_transition_barrier.StateBefore = src_state.subresource_states[0];
                pre_transition_barrier.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

                D3D12_RESOURCE_BARRIER pre_resource_barrier {};
                pre_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                pre_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                pre_resource_barrier.Transition = pre_transition_barrier;

                command_list->ResourceBarrier (1, &pre_resource_barrier);
            }

            while (entry_it != inputs_entries.end ())
            {
                ID3D12Resource* dummy_resource = nullptr;
                {
                    std::scoped_lock lock (state_mutex_);
                    dummy_resource = virtual_address_tracker_.Get (*entry_it->dest_addr, entry_it->size);
                    if (dummy_resource == nullptr)
                    {
                        break;
                    }
                    else if (dummy_resource != src_resource)
                    {
                        break;
                    }
                }

                auto dest_addr = *entry_it->dest_addr;
                auto dest_offset = entry_it->offset;
                auto dest_size = entry_it->size;
                auto src_offset = dest_addr - src_resource->GetGPUVirtualAddress ();
                command_list->CopyBufferRegion (copyback_resource, dest_offset, src_resource, src_offset, dest_size);
                ++entry_it;
            }

            if (state_transition_needed)
            {
                D3D12_RESOURCE_TRANSITION_BARRIER post_transition_barrier {};
                post_transition_barrier.pResource = src_resource;
                post_transition_barrier.Subresource = 0;
                post_transition_barrier.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
                post_transition_barrier.StateAfter = src_state.subresource_states[0];

                D3D12_RESOURCE_BARRIER post_resource_barrier {};
                post_resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                post_resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                post_resource_barrier.Transition = post_transition_barrier;

                command_list->ResourceBarrier (1, &post_resource_barrier);
            }
        }
    }

    // ========================================================================

    /// @brief Track execution of command lists and commit their pending
    ///        resource transitions to the resource state tracker
    ///
    /// @param num_command_lists Number of command lists being executed
    /// @param pp_command_lists Pointer to the command lists being executed
    void TrackExecuteCommandLists (UINT num_command_lists, ID3D12CommandList* const* pp_command_lists)
    {
        std::unique_lock lock (state_mutex_);
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

    ///< Mutex for synchronizing access to the capture state
    mutable std::shared_mutex state_mutex_;
    ///< Acceleration structure tracker
    AccelerationStructureTracker as_tracker_;
    ///< Map of GPU virtual addresses for reverse lookup
    VirtualAddressTracker virtual_address_tracker_;
    ///< 'ID3D12Resource' state tracker
    ResourceStateTracker resource_state_tracker_;
    ///< Pending resource transitions for command lists tracker
    PendingTransitionTracker pending_transition_tracker_;
};

}

// ----------------------------------------------------------------------------