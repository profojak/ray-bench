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
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};
        NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS prebuild_info_params {};
        prebuild_info_params.version = NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS_VER;
        prebuild_info_params.pDesc = &pBuildParams->pDesc->inputs;
        prebuild_info_params.pInfo = &prebuild_info;
        NvAPI_Status status = NvAPI_D3D12_GetRaytracingAccelerationStructurePrebuildInfoEx (device, &prebuild_info_params);
        if (status != NVAPI_OK)
        {
            RAYBENCH_LOG_ERROR ("Failed to get prebuild info for acceleration structure build: {}!",
                                static_cast<int>(status));
            device->Release ();
            return;
        }

        {
            std::scoped_lock lock (state_mutex_);
            resource = virtual_address_tracker_.Get (pBuildParams->pDesc->destAccelerationStructureData,
                                                     prebuild_info.ResultDataMaxSizeInBytes);
            if (resource == nullptr)
            {
                device->Release ();
                return;
            }
        }

        // Store acceleration structure build information for later retrieval
        // during command list execution
        D3D12_GPU_VIRTUAL_ADDRESS dest_addr = pBuildParams->pDesc->destAccelerationStructureData;
        UINT64 dest_size = prebuild_info.ResultDataMaxSizeInBytes;
        ID3D12Resource* dest_resource = resource;
        NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX inputs = pBuildParams->pDesc->inputs;

        std::vector<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX> geometry_desc;
        if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
        {
            for (UINT i = 0; i < inputs.numDescs; ++i)
            {
                geometry_desc.push_back (inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                                         ? inputs.pGeometryDescs[i] : *inputs.ppGeometryDescs[i]);
            }

            // Clear pointers to avoid referencing invalid memory
            inputs.pGeometryDescs = nullptr;
            inputs.ppGeometryDescs = nullptr;
        }

        struct InputsEntry
        {
            ///< GPU virtual address of the inputs buffer
            const D3D12_GPU_VIRTUAL_ADDRESS* src_addr {nullptr};
            ///< Size of the inputs entry in the inputs buffer
            UINT64 size {0};
            ///< Offset of the inputs entry in the inputs buffer
            UINT64 offset {0};
        };

        // Store build inputs for later retrieval during command list execution
        UINT64 inputs_size = 0;
        std::vector<InputsEntry> inputs_entries;

        if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
        {
            for (UINT i = 0; i < inputs.numDescs; ++i)
            {
                const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc = geometry_desc[i];
                if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES_EX)
                {
                    const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& triangles_desc = desc.triangles;

                    // Transformation matrix
                    if (triangles_desc.Transform3x4)
                    {
                        constexpr UINT64 transform_size = 12 * sizeof (float);
                        inputs_size = raybench::util::AlignValue<D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT> (inputs_size);
                        inputs_entries.emplace_back (
                            InputsEntry {
                            &triangles_desc.Transform3x4,
                            transform_size,
                            inputs_size
                            });
                        inputs_size += transform_size;
                    }

                    // Index buffer
                    if (triangles_desc.IndexCount != 0 && triangles_desc.IndexBuffer != 0)
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
                            InputsEntry {
                            &triangles_desc.IndexBuffer,
                            index_buffer_size,
                            inputs_size
                            });
                        inputs_size += index_buffer_size;
                    }

                    // Vertex buffer
                    if (triangles_desc.VertexCount != 0 && triangles_desc.VertexBuffer.StartAddress != 0)
                    {
                        UINT64 vertex_size = triangles_desc.VertexCount * triangles_desc.VertexBuffer.StrideInBytes;
                        inputs_size = raybench::util::AlignValue<4> (inputs_size);
                        inputs_entries.emplace_back (
                            InputsEntry {
                            &triangles_desc.VertexBuffer.StartAddress,
                            vertex_size,
                            inputs_size
                            });
                        inputs_size += vertex_size;
                    }
                }
            }
        }
        else if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)
        {
            if (inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS)
            {
                RAYBENCH_LOG_WARNING_ONCE ("TLAS with array of pointers is not yet supported!");
                device->Release ();
                return;
            }
            else if (inputs.numDescs > 0 && inputs.instanceDescs != 0)
            {
                inputs_size = inputs.numDescs * sizeof (D3D12_RAYTRACING_INSTANCE_DESC);
                inputs_entries.emplace_back (
                    InputsEntry {
                        &inputs.instanceDescs,
                        inputs_size,
                        0
                    });
            }
        }
        else
        {
            RAYBENCH_LOG_ERROR ("Unsupported acceleration structure type: {}!", static_cast<int>(inputs.type));
            device->Release ();
            return;
        }

        if (inputs_size == 0)
        {
            device->Release ();
            return;
        }

        // Create copyback buffer for build inputs to be retrieved during
        // command list execution.  Sort entries by destination address to
        // optimize retrieval during command list execution
        std::sort (inputs_entries.begin (), inputs_entries.end (), [] (
            const InputsEntry& a,
            const InputsEntry& b)
            {
                if (a.src_addr == nullptr || b.src_addr == nullptr)
                {
                    return a.src_addr < b.src_addr;
                }
                return *a.src_addr < *b.src_addr;
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

            while (entry_it != inputs_entries.end ())
            {
                if (entry_it->src_addr == nullptr || *entry_it->src_addr == 0)
                {
                    ++entry_it;
                    continue;
                }

                ID3D12Resource* current_src_resource = nullptr;
                {
                    std::scoped_lock lock (state_mutex_);
                    current_src_resource = virtual_address_tracker_.Get (*entry_it->src_addr, entry_it->size);
                    if (current_src_resource == nullptr || current_src_resource != src_resource)
                    {
                        break;
                    }
                }

                auto addr = *entry_it->src_addr;
                auto offset = entry_it->offset;
                auto size = entry_it->size;
                auto src_offset = addr - src_resource->GetGPUVirtualAddress ();
                command_list->CopyBufferRegion (copyback_resource, offset, src_resource, src_offset, size);
                ++entry_it;
            }
        }

        (void) dest_addr;
        (void) dest_size;
        (void) dest_resource;

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