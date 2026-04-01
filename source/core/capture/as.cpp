// ============================================================================

/// @brief Acceleration structure tracker

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

#include "util/log.h"

export module RayBench.Capture:AS;

import std;
import RayBench.Util;

namespace raybench::capture
{

/// @brief Acceleration structure tracker
export class AccelerationStructureTracker
{
    friend class Writer;

public:

    ///< Type alias for timestamp to track acceleration structure builds
    using TimeStamp = UINT64;

    /// @brief Raytracing acceleration structure build inputs entry
    struct InputsEntry
    {
        ///< GPU virtual address of the inputs buffer
        const D3D12_GPU_VIRTUAL_ADDRESS* src_addr {nullptr};
        ///< Size of the inputs entry in the inputs buffer
        UINT64 size {0};
        ///< Offset of the inputs entry in the inputs buffer
        UINT64 offset {0};
    };

    // ------------------------------------------------------------------------

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
        std::variant<
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS,
            NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX
        > inputs;
        ///< Build inputs geometry descriptions
        std::variant<
            std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>,
            std::vector<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX>
        > geometry_descs;
        ///< Size of the copyback buffer
        UINT64 copyback_size {0};
        ///< Copyback buffer resource
        ID3D12Resource* copyback_resource {nullptr};
        ///< Timestamp of the build to associate it with command list execution
        TimeStamp timestamp {0};

        // ====================================================================

        /// @brief Get maximum estimated size of the acceleration structure
        ///
        /// @param device Device to query prebuild info from
        /// @return Maximum estimated size of the acceleration structure
        UINT64 GetMaximumSizeAS (ID3D12Device5* device) const
        {
            return std::visit ([&] (const auto& inputs) -> UINT64
                               {
                                   using InputsType = std::decay_t<decltype (inputs)>;

                                   // DirectX 12
                                   if constexpr (std::is_same_v<InputsType, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>)
                                   {
                                       D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};
                                       device->GetRaytracingAccelerationStructurePrebuildInfo (&inputs, &prebuild_info);
                                       return prebuild_info.ResultDataMaxSizeInBytes;
                                   }

                                   // NVAPI
                                   else if constexpr (std::is_same_v<InputsType, NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX>)
                                   {
                                       D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info = {};
                                       NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS prebuild_info_params {};
                                       prebuild_info_params.version = NVAPI_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_EX_PARAMS_VER;
                                       prebuild_info_params.pDesc = &inputs;
                                       prebuild_info_params.pInfo = &prebuild_info;
                                       NvAPI_Status status = NvAPI_D3D12_GetRaytracingAccelerationStructurePrebuildInfoEx (device, &prebuild_info_params);
                                       if (status != NVAPI_OK)
                                       {
                                           return 0;
                                       }
                                       return prebuild_info.ResultDataMaxSizeInBytes;
                                   }
                               }, this->inputs);
        }

        // --------------------------------------------------------------------

        /// @brief Copy geometry descriptions from build inputs
        ///
        /// @param resource Destination resource
        void CopyGeometryDescs (ID3D12Resource* resource)
        {
            dest_resource = resource;
            std::visit ([this] (auto& inputs) -> void
                        {
                            using InputsType = std::decay_t<decltype (inputs)>;

                            // DirectX 12
                            if constexpr (std::is_same_v<InputsType, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>)
                            {
                                if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
                                {
                                    auto& descs = geometry_descs.emplace<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>> ();
                                    for (UINT i = 0; i < inputs.NumDescs; ++i)
                                    {
                                        descs.push_back (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                                                         ? inputs.pGeometryDescs[i] : *inputs.ppGeometryDescs[i]);
                                    }

                                    // Clear pointers to avoid referencing invalid memory
                                    inputs.pGeometryDescs = nullptr;
                                    inputs.ppGeometryDescs = nullptr;
                                }
                            }

                            // NVAPI
                            else if constexpr (std::is_same_v<InputsType, NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX>)
                            {
                                if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
                                {
                                    auto& descs = geometry_descs.emplace<std::vector<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX>> ();
                                    for (UINT i = 0; i < inputs.numDescs; ++i)
                                    {
                                        descs.push_back (inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                                                         ? inputs.pGeometryDescs[i] : *inputs.ppGeometryDescs[i]);
                                    }

                                    // Clear pointers to avoid referencing invalid memory
                                    inputs.pGeometryDescs = nullptr;
                                    inputs.ppGeometryDescs = nullptr;
                                }
                            }
                        }, this->inputs);
        }

        // --------------------------------------------------------------------

        /// @brief Copy build inputs
        ///
        /// @param inputs_entries Destination vector for inputs entries
        /// @return Total size of the copied inputs
        UINT64 CopyBuildInputs (std::vector<AccelerationStructureTracker::InputsEntry>& inputs_entries)
        {
            return std::visit ([this, &inputs_entries] (auto& inputs) -> UINT64
                               {
                                   using InputsType = std::decay_t<decltype (inputs)>;

                                   // DirectX 12
                                   if constexpr (std::is_same_v<InputsType, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>)
                                   {
                                       return 0;
                                   }

                                   // NVAPI
                                   else if constexpr (std::is_same_v<InputsType, NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX>)
                                   {
                                       if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
                                       {
                                           auto& geo_descs = std::get<std::vector<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX>> (this->geometry_descs);
                                           for (UINT i = 0; i < inputs.numDescs; ++i)
                                           {
                                               const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc = geo_descs[i];
                                               if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES_EX)
                                               {
                                                   return CopyBLAS (desc.triangles, inputs_entries);
                                               }
                                           }
                                           return 0;
                                       }
                                       else if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)
                                       {
                                           if (inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS)
                                           {
                                               RAYBENCH_LOG_WARNING_ONCE ("TLAS with array of pointers is not yet supported!");
                                               return 0;
                                           }
                                           else if (inputs.numDescs > 0 && inputs.instanceDescs != 0)
                                           {
                                               UINT64 inputs_size = inputs.numDescs * sizeof (D3D12_RAYTRACING_INSTANCE_DESC);
                                               inputs_entries.emplace_back (
                                                   AccelerationStructureTracker::InputsEntry {
                                                       &inputs.instanceDescs,
                                                       inputs_size,
                                                       0
                                                   });
                                               return inputs_size;
                                           }
                                           else
                                           {
                                               return 0;
                                           }
                                       }
                                       else
                                       {
                                           RAYBENCH_LOG_ERROR ("Unsupported acceleration structure type: {}!", static_cast<int>(inputs.type));
                                           return 0;
                                       }
                                   }
                                   else
                                   {
                                       return 0;
                                   }
                               }, this->inputs);
        }

        // ====================================================================

        /// @brief Create copyback resource for build inputs
        ///
        /// @param device Device to create the resource on
        /// @return True if resource was created successfully, false otherwise
        bool CreateCopybackResource (ID3D12Device5* device)
        {
            ID3D12Resource* resource = nullptr;

            D3D12_HEAP_PROPERTIES heap_properties {};
            heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask = 1;
            heap_properties.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC resource_desc {};
            resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment = 0;
            resource_desc.Width = copyback_size;
            resource_desc.Height = 1;
            resource_desc.DepthOrArraySize = 1;
            resource_desc.MipLevels = 1;
            resource_desc.Format = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            HRESULT hr = device->CreateCommittedResource (&heap_properties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resource_desc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr,
                                                  IID_PPV_ARGS (&resource));
            if (FAILED (hr))
            {
                RAYBENCH_LOG_ERROR ("Failed to create copyback resource: 0x{:08X}!", hr);
                return false;
            }
            copyback_resource = resource;
            return true;
        }

        // --------------------------------------------------------------------

        /// @brief Copy geometry description entries from BLAS build inputs
        ///
        /// @param triangles_desc Geometry description
        /// @param inputs_entries Destination vector for inputs entries
        /// @return Total size of the copied inputs
        UINT64 CopyBLAS (const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& triangles_desc,
                         std::vector<AccelerationStructureTracker::InputsEntry>& inputs_entries)
        {
            UINT64 inputs_size = 0;

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
                    AccelerationStructureTracker::InputsEntry {
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
                    AccelerationStructureTracker::InputsEntry {
                    &triangles_desc.VertexBuffer.StartAddress,
                    vertex_size,
                    inputs_size
                    });
                inputs_size += vertex_size;
            }

            return inputs_size;
        }
    };

    // ========================================================================

    /// @brief Add an acceleration structure build to the pending builds
    ///
    /// @param command_list Command list the build is recorded on
    /// @param build_info Information about the acceleration structure build
    void Add (ID3D12GraphicsCommandList* command_list, const BuildInfo& build_info)
    {
        pending_builds_[command_list].push_back (build_info);
    }

    // ------------------------------------------------------------------------

    /// @brief Commit pending builds of a command list by signaling the fence
    ///        and associating the builds with the current fence timestamp.
    ///        Also retire staging buffers of overlapped builds
    ///
    /// @param command_queue Command queue executing the command list
    /// @param command_list Command list to commit pending builds for
    void Commit (ID3D12CommandQueue* command_queue, ID3D12GraphicsCommandList* command_list)
    {
        if (fence_ == nullptr)
        {
            ID3D12Device* device = nullptr;
            if (SUCCEEDED (command_queue->GetDevice (IID_PPV_ARGS (&device))))
            {
                device->CreateFence (0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (&fence_));
                device->Release ();
                if (fence_ == nullptr)
                {
                    return;
                }
            }
        }

        // If there are pending builds for this command list, signal the fence
        // and associate the builds with the current fence timestamp
        if (auto it = pending_builds_.find (command_list); it != pending_builds_.end ())
        {
            TimeStamp current_timestamp = ++fence_timestamp_;
            command_queue->Signal (fence_, current_timestamp);

            for (auto& build_info : it->second)
            {
                build_info.timestamp = current_timestamp;

                const UINT64 new_start = build_info.dest_addr;
                const UINT64 new_end = new_start + build_info.dest_size;

                // Check if the previous entry overlaps (starts before
                // 'new_start' but ends after it)
                auto it_overlap = build_info_map_.lower_bound (new_start);
                if (it_overlap != build_info_map_.begin ())
                {
                    if (auto it_prev = std::prev (it_overlap); it_prev->first + it_prev->second.dest_size > new_start)
                    {
                        it_overlap = it_prev;
                    }
                }

                // Iterate through overlapping builds and retire their staging
                // buffers
                while (it_overlap != build_info_map_.end ())
                {
                    const UINT64 cur_start = it_overlap->first;
                    const UINT64 cur_end = cur_start + it_overlap->second.dest_size;

                    if (cur_start >= new_end)
                    {
                        break;
                    }

                    if (new_start < cur_end && cur_start < new_end)
                    {
                        if (it_overlap->second.copyback_resource != nullptr)
                        {
                            retired_builds_.push_back ({
                                .resource = it_overlap->second.copyback_resource,
                                .retirement_timestamp = current_timestamp
                                                       });
                        }
                        it_overlap = build_info_map_.erase (it_overlap);
                    }
                    else
                    {
                        ++it_overlap;
                    }
                }

                build_info_map_[build_info.dest_addr] = build_info;
            }
            pending_builds_.erase (it);
        }

        // Remove staging buffers of retired builds whose associated builds
        // have completed execution
        TimeStamp completed_timestamp = fence_->GetCompletedValue ();
        std::erase_if (retired_builds_, [this, completed_timestamp] (const RetiredBuildInfo& retired_build)
                       {
                           if (retired_build.retirement_timestamp <= completed_timestamp)
                           {
                               if (retired_build.resource != nullptr)
                               {
                                   retired_build.resource->Release ();
                               }
                               return true;
                           }
                           return false;
                       });
    }

private:

    // ========================================================================

    /// @brief Acceleration structure build information waiting for retirement
    struct RetiredBuildInfo
    {
        ///< Associated copyback resource
        ID3D12Resource* resource {nullptr};
        ///< Timestamp of build completion to determine when it can be retired
        TimeStamp retirement_timestamp {0};
    };

    ///< Map of active acceleration structures
    std::map<D3D12_GPU_VIRTUAL_ADDRESS, BuildInfo> build_info_map_;
    ///< Queue of acceleration structure builds waiting for retirement
    std::vector<RetiredBuildInfo> retired_builds_;
    ///< Builds recorded on command lists waiting for execution
    std::unordered_map<ID3D12GraphicsCommandList*, std::vector<BuildInfo>> pending_builds_;

    ///< Fence
    ID3D12Fence* fence_ {nullptr};
    ///< Timestamp fence counter
    TimeStamp fence_timestamp_ {0};
};

}

// ----------------------------------------------------------------------------