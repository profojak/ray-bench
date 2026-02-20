// ----------------------------------------------------------------------------

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
    }

    if (wstr.size () > static_cast<size_t>(std::numeric_limits<int>::max ()))
    {
        RAYBENCH_LOG_ERROR ("Failed to convert wide string that is over length limit: {}",
                            wstr.size());
    }
    const int wlen = static_cast<int>(wstr.size ());
    std::array<char, 512> stack_buffer;

    int result = WideCharToMultiByte (CP_UTF8, 0,
                                      wstr.data (), wlen,
                                      stack_buffer.data (), sizeof (stack_buffer),
                                      nullptr, nullptr);

    // If successful, construct string directly from stack memory
    if (result > 0)
    {
        return std::string (stack_buffer.data (), result);
    }

    // If stack buffer was too small, use `string` heap allocation
    if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
    {
        const int size_needed = WideCharToMultiByte (CP_UTF8, 0,
                                                     wstr.data (), wlen,
                                                     nullptr, 0,
                                                     nullptr, nullptr);
        if (size_needed <= 0)
        {
            RAYBENCH_LOG_ERROR ("Failed to convert wide string to narrow string: {}",
                                GetLastError ());
        }

        std::string heap_str (size_needed, '\0');
        int heap_result = WideCharToMultiByte (CP_UTF8, 0,
                                               wstr.data (), wlen,
                                               heap_str.data (), size_needed,
                                               nullptr, nullptr);
        if (heap_result > 0)
        {
            return heap_str;
        }
    }

    return {};
}

}

// ----------------------------------------------------------------------------