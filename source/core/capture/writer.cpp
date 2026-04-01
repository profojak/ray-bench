// ============================================================================

/// @brief State writer

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

#include "util/log.h"

export module RayBench.Capture:Writer;

import std;
import :Tracker;
import RayBench.Util;

namespace raybench::capture
{

/// @brief State writer
export class Writer
{
public:

    void WriteCapture (Tracker& tracker, IDXGISwapChain* This)
    {
        RAYBENCH_LOG_INFO ("{}", tracker.as_tracker_.build_info_map_.size ());

        ID3D12CommandQueue* command_queue = nullptr;
        {
            std::scoped_lock lock (tracker.state_mutex_);
            auto it = tracker.swap_chain_command_queue_map_.find (This);
            if (it == tracker.swap_chain_command_queue_map_.end ())
            {
                RAYBENCH_LOG_ERROR ("Failed to find command queue for swap chain: 0x{:016X}!", reinterpret_cast<std::uintptr_t>(This));
                return;
            }
            command_queue = it->second;
        }

        for (const auto& [as_addr, build_info] : tracker.as_tracker_.build_info_map_)
        {
            RAYBENCH_ASSERT (build_info.copyback_size != 0, "Invalid acceleration structure staging buffer size!");
            RAYBENCH_ASSERT (build_info.copyback_resource != nullptr, "Invalid acceleration structure staging buffer resource!");
        }
    }
};

}

// ----------------------------------------------------------------------------