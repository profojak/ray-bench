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
{};

}

// ----------------------------------------------------------------------------