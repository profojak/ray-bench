// ============================================================================

/// @brief Re-entrancy guard for Windows API hooks

export module RayBench.Payload:Guard;

import std;

namespace raybench::payload
{

/// @brief Re-entrancy guard for Windows API hooks to prevent inifinite
///        recursion when a hooked function is called within the hook itself
/// 
/// @tparam HookTag A unique type used to distinguish different hooks
export template <typename HookTag>
class ReentrancyGuard
{
public:

    /// @brief Re-entrancy guard constructor increments the counter
    ///        for the current thread
    ReentrancyGuard ()
    {
        ++counter_;
    }

    /// @brief Re-entrancy guard destructor decrements the counter
    ///        for the current thread
    ~ReentrancyGuard ()
    {
        --counter_;
    }

    ReentrancyGuard (const ReentrancyGuard&) = delete;
    ReentrancyGuard& operator=(const ReentrancyGuard&) = delete;

    // ------------------------------------------------------------------------

    /// @brief Check if the current thread is already executing a hook
    ///        for the given HookTag
    [[nodiscard]] static bool IsActive ()
    {
        return counter_ > 0;
    }

    // ------------------------------------------------------------------------

    /// @brief Get the current reference count for the given `HookTag`, which
    ///        indicates how many times the hook has been entered for
    ///        the current thread
    /// 
    /// @return The current reference count for the given `HookTag`
    [[nodiscard]] static std::uint32_t GetRef ()
    {
        return counter_;
    }

private:

    // ========================================================================

    ///< Counter to track the number of active hooks for the current thread
    static thread_local std::uint32_t counter_;
};

/// @brief Thread-local counter initialization for the `ReentrancyGuard` class
/// 
/// @tparam HookTag Unique type used to distinguish different hooks
template <typename HookTag>
thread_local std::uint32_t ReentrancyGuard<HookTag>::counter_ = 0;

}

// ----------------------------------------------------------------------------