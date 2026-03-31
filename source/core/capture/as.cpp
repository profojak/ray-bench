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
        /// @brief API used for the build
        enum class API : uint8_t
        {
            D3D12,
            NVAPI
        } api {API::D3D12};

        ///< GPU virtual address of the destination memory
        D3D12_GPU_VIRTUAL_ADDRESS dest_addr {0};
        ///< Size of the destination memory
        UINT64 dest_size {0};
        ///< Associated resource
        ID3D12Resource* dest_resource {nullptr};
        ///< Build inputs
        NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX inputs;
        ///< Build inputs geometry descriptions
        std::vector<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX> geometry_descs;
        ///< Size of the copyback buffer
        UINT64 copyback_size {0};
        ///< Copyback buffer resource
        ID3D12Resource* copyback_resource {nullptr};
        ///< Timestamp of the build to associate it with command list execution
        TimeStamp timestamp {0};
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