// ============================================================================

/// @brief String utilities

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "log.h"

export module RayBench.Util:String;

import std;
import :Log;

namespace raybench::util::string
{

/// @brief Narrow wide string
/// 
/// @param wstr Wide string
/// @return Narrow string
export std::string WideToNarrow (std::wstring_view wstr)
{
    if (wstr.empty ())
    {
        RAYBENCH_LOG_WARNING ("Requested to convert an empty wide string");
        return {};
    }

    if (wstr.size () > static_cast<size_t>(std::numeric_limits<int>::max ()))
    {
        RAYBENCH_LOG_ERROR ("Failed to convert wide string that is over length limit: {}!",
                            wstr.size());
        return {};
    }
    const int wlen = static_cast<int>(wstr.size ());
    std::array<char, 512> stack_buffer;

    int result = WideCharToMultiByte (CP_UTF8, 0,
                                      wstr.data (), wlen,
                                      stack_buffer.data (), static_cast<int>(stack_buffer.size ()),
                                      nullptr, nullptr);

    // If successful, construct string directly from stack memory
    if (result > 0)
    {
        return std::string (stack_buffer.data (), result);
    }

    // If stack buffer was too small, use `string` heap allocation with C++23 resize_and_overwrite
    if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
    {
        const int size_needed = WideCharToMultiByte (CP_UTF8, 0,
                                                     wstr.data (), wlen,
                                                     nullptr, 0,
                                                     nullptr, nullptr);
        if (size_needed <= 0)
        {
            RAYBENCH_LOG_ERROR ("Failed to convert wide string to narrow string: {}!",
                                GetLastError ());
            return {};
        }

        std::string heap_str;
        heap_str.resize_and_overwrite (static_cast<size_t>(size_needed), [&](char* buf, size_t n) {
            return static_cast<size_t>(WideCharToMultiByte (CP_UTF8, 0,
                                                            wstr.data (), wlen,
                                                            buf, static_cast<int>(n),
                                                            nullptr, nullptr));
        });

        return heap_str;
    }

    return {};
}

}

// ----------------------------------------------------------------------------