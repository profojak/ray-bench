// ============================================================================

/// @brief Platform utilities

export module RayBench.Util:Platform;

import std;

namespace raybench::util
{

export template <std::uint64_t Alignment, std::unsigned_integral T>
[[nodiscard]] constexpr T AlignValue (T original) noexcept
{
    static_assert(std::has_single_bit (Alignment), "Alignment must be a power of two.");
    static_assert(std::numeric_limits<T>::max () > Alignment, "Alignment value is too large.");

    constexpr T alignment_t = static_cast<T>(Alignment);
    return (original + (alignment_t - 1)) & ~(alignment_t - 1);
}

}

// ----------------------------------------------------------------------------