// ============================================================================

/// @brief Capture manager

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
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
private:

    /// @brief Capture mode flags
    enum class CaptureModeFlags : std::uint8_t
    {
        disabled = 0x0,
        write = 0x1,
        track = 0x2,
        write_track = write | track,
    };

    using CaptureMode = std::underlying_type_t<CaptureModeFlags>;

public:

    using APIMutex = std::shared_mutex;

    // ========================================================================

    /// @brief Get the singleton instance of the capture manager
    ///
    /// @return The singleton instance
    static Manager& GetManager () noexcept
    {
        static Manager instance;
        return instance;
    }

    // ------------------------------------------------------------------------

    /// @brief Acquire a shared lock for API calls
    ///
    /// @return A shared lock object
    static auto GetSharedLock ()
    {
        return std::shared_lock<APIMutex> (api_call_mutex_);
    }

    // ------------------------------------------------------------------------

    /// @brief Acquire a unique lock for capture operations
    ///
    /// @return A unique lock object
    static auto GetUniqueLock ()
    {
        return std::unique_lock<APIMutex> (api_call_mutex_);
    }

    // ========================================================================

    /// @brief Get the current capture mode
    CaptureMode GetCaptureMode () const noexcept
    {
        return capture_mode_;
    }

    // ------------------------------------------------------------------------

    /// @brief Check if capture mode is set to write
    bool IsCaptureModeWrite () const noexcept
    {
        return (capture_mode_ & std::to_underlying (CaptureModeFlags::write)) == std::to_underlying (CaptureModeFlags::write);
    }

    // ------------------------------------------------------------------------

    /// @brief Check if capture mode is set to track
    bool IsCaptureModeTrack () const noexcept
    {
        return (capture_mode_ & std::to_underlying (CaptureModeFlags::track)) == std::to_underlying (CaptureModeFlags::track);
    }
    // ------------------------------------------------------------------------

    /// @brief Activate capture mode
    ///
    /// @param lock Shared lock to synchronize with API calls
    void ActivateCapture (std::shared_lock<APIMutex>& lock)
    {
        auto owns_lock = lock.owns_lock ();
        if (owns_lock)
        {
            lock.unlock ();
        }

        {
            auto exclusive_lock = GetUniqueLock ();

            RAYBENCH_LOG_TRACE ("Activating capture...");

            capture_mode_ |= std::to_underlying (CaptureModeFlags::write);
        }

        if (owns_lock)
        {
            lock.lock ();
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Deactivate capture mode
    ///
    /// @param lock Shared lock to synchronize with API calls
    void DeactivateCapture (std::shared_lock<APIMutex>& lock)
    {
        auto owns_lock = lock.owns_lock ();
        if (owns_lock)
        {
            lock.unlock ();
        }

        {
            auto exclusive_lock = GetUniqueLock ();

            RAYBENCH_LOG_TRACE ("Deactivating capture...");

            capture_mode_ &= ~std::to_underlying (CaptureModeFlags::write);
        }

        if (owns_lock)
        {
            lock.lock ();
        }
    }

    // ========================================================================

    /// @brief Increment the API call depth counter
    ///
    /// @return The new API call depth
    std::uint32_t CallDepthIncrement ()
    {
        return ++api_call_depth_;
    }

    // ------------------------------------------------------------------------

    /// @brief Decrement the API call depth counter
    ///
    /// @return The new API call depth
    std::uint32_t CallDepthDecrement ()
    {
        return --api_call_depth_;
    }

    // ========================================================================

    /// @brief `ID3D12Resource::GetGPUVirtualAddress` hook callback
    void Post_ID3D12Resource_GetGPUVirtualAddress (ID3D12Resource*,
                                                   D3D12_GPU_VIRTUAL_ADDRESS addr)
    {
        if (IsCaptureModeTrack () && (addr != 0))
        {
        }
    }

    // ========================================================================

    /// @brief `IDXGISwapChain::Present` hook callback
    void Pre_IDXGISwapChain_Present ()
    {
        // TODO: Extract and store frame buffer.
    }

    // ------------------------------------------------------------------------

    /// @brief `IDXGISwapChain::Present` hook callback
    void Post_IDXGISwapChain_Present (UINT flags, std::shared_lock<APIMutex>& lock)
    {
        if (flags & DXGI_PRESENT_TEST)
        {
            return;
        }

        if (IsCaptureModeWrite ())
        {
            DeactivateCapture (lock);
        }
        else if (IsCaptureModeTrack ())
        {
            if (raybench::util::Input::IsKeyJustPressed (capture_frame_key_, is_capture_frame_key_pressed_))
            {
                ActivateCapture (lock);
            }
        }
    }

    // ========================================================================

    void Post_ID3D12GraphicsCommandList_ResourceBarrier (
        UINT,
        const D3D12_RESOURCE_BARRIER*,
        std::shared_lock<APIMutex>&
    )
    {
        if (IsCaptureModeTrack ())
        {
        }
    }

    // ------------------------------------------------------------------------

    /// @brief `ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure`
    ///        hook callback
    void Post_ID3D12GraphicsCommandList_BuildRaytracingAccelerationStructure (
        ID3D12GraphicsCommandList4*,
        const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*,
        std::shared_lock<APIMutex>&
    )
    {
        if (IsCaptureModeTrack ())
        {
        }
    }

private:

    // ========================================================================

    ///< Mutex for synchronizing API calls and capture operations
    static APIMutex api_call_mutex_;
    ///< Thread-local variable to track the depth of API calls to prevent
    ///  re-entrant capture
    static thread_local std::uint32_t api_call_depth_;

    ///< Current capture mode
    CaptureMode capture_mode_ = std::to_underlying (CaptureModeFlags::track);
    ///< Key code to trigger frame capture
    raybench::util::Input::KeyCode capture_frame_key_ = raybench::util::Input::KeyCode::F12;
    ///< Flag indicating if frame capture key is currently pressed
    bool is_capture_frame_key_pressed_ = false;
};

Manager::APIMutex Manager::api_call_mutex_;
thread_local std::uint32_t Manager::api_call_depth_ = 0;

}

// ----------------------------------------------------------------------------