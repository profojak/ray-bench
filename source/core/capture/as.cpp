// ============================================================================

/// @brief Acceleration structure tracker

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>

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

    // ------------------------------------------------------------------------

    /// @brief Raytracing acceleration structure build inputs entry
    struct InputsEntry
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