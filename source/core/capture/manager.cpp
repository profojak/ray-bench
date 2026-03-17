// ============================================================================

/// @brief Capture manager

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi1_6.h>
#include <dxgi.h>

#include "util/log.h"

export module RayBench.Capture:Manager;

import std;
import RayBench.Util;

namespace raybench::capture
{

/// @brief Capture manager
export class Manager
{
public:

    /// @brief Get the singleton instance of the capture manager
	///
    /// @return The singleton instance
	static Manager& GetManager() noexcept
	{
		static Manager instance;
		return instance;
    }

	// ========================================================================

    /// @brief `IDXGISwapChain::Present` hook callback
	void PrePresent ()
	{
		// TODO: Extract and store frame buffer.
	}

    /// @brief `IDXGISwapChain::Present` hook callback
	void PostPresent (UINT flags)
	{
		if (flags & DXGI_PRESENT_TEST)
		{
			return;
		}

		if (raybench::util::Input::IsKeyJustPressed (capture_frame_key_, is_capture_frame_key_pressed_))
		{
		}
	}

private:

	// ========================================================================

    ///< Shared mutex for synchronizing API calls and capture operations
    std::shared_mutex mutex_;
    ///< Key code to trigger frame capture
    raybench::util::Input::KeyCode capture_frame_key_ = raybench::util::Input::KeyCode::F12;
    ///< Flag indicating if frame capture key is currently pressed
    bool is_capture_frame_key_pressed_ = false;
};

}

// ----------------------------------------------------------------------------