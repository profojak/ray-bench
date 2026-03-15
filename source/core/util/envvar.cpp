// ============================================================================

/// @brief Environment variable utilities

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "log.h"

export module RayBench.Util:EnvVar;

import std;
import :Log;

namespace raybench::util::EnvVar
{

///< Maximum length for environment variable values
constexpr size_t max_env_var_length = 8192;

///< Environment variable name for log settings
export constexpr std::string_view log_settings = "RAYBENCH_LOG_SETTINGS";
///< Environment variable name for payload dynamic-link library path
export constexpr std::string_view payload_dll_path = "RAYBENCH_PAYLOAD_DLL_PATH";
///< Environment variable name for target application path to inject into
export constexpr std::string_view target_app_path = "RAYBENCH_TARGET_APP_PATH";

// ============================================================================

/// @brief Set an environment variable
/// 
/// @param name The name of the environment variable
/// @param value The value to set
/// @return True if the environment variable was set successfully, false otherwise
export bool Set (std::string_view name, std::string_view value) noexcept
{
    return SetEnvironmentVariableA (name.data (), value.data ()) != 0;
}

// ----------------------------------------------------------------------------

/// @brief Get the value of an environment variable
/// 
/// @param name The name of the environment variable
/// @return The value of the environment variable, or std::nullopt if not found
export [[nodiscard]] std::optional<std::string> Get (std::string_view name) noexcept
{
    std::array<char, max_env_var_length> buffer {};
    DWORD result = GetEnvironmentVariableA (name.data (), buffer.data (),
                                            static_cast<DWORD> (buffer.size ()));
    if (result == 0)
    {
        DWORD error = GetLastError ();
        if (error != ERROR_ENVVAR_NOT_FOUND)
        {
            RAYBENCH_LOG_ERROR ("Failed to get environment variable '{}': {}", name, error);
        }
        else
        {
            RAYBENCH_LOG_DEBUG ("Not found environment variable '{}'", name);
        }
    }
    else if (result >= buffer.size ())
    {
        RAYBENCH_LOG_ERROR ("Too long environment variable '{}': required size {}", name, result);
    }
    else
    {
        return std::string (buffer.data (), result);
    }
    return std::nullopt;
}

// ----------------------------------------------------------------------------

/// @brief Unset an environment variable
/// 
/// @param name The name of the environment variable
export void Unset (std::string_view name) noexcept
{
    if (SetEnvironmentVariableA (name.data (), nullptr) == 0)
    {
        DWORD error = GetLastError ();
        RAYBENCH_LOG_ERROR ("Failed to unset environment variable '{}': {}", name, error);
    }
}

}

// ----------------------------------------------------------------------------