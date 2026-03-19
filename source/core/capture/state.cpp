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
import :VirtualMap;
import RayBench.Util;

namespace raybench::capture
{

/// @brief Capture state
export class State
{
private:

    // ========================================================================

    ///< Map of GPU virtual addresses
    VirtualMap virtual_map_;
};

}

// ----------------------------------------------------------------------------