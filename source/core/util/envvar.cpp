// ----------------------------------------------------------------------------

/// @brief Environment variable utilities

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "log.h"

export module RayBench.Util:EnvVar;

import std;
import :Log;

namespace raybench::util
{

///< Maximum length for environment variable values
constexpr size_t max_env_var_length = 8192;

///< Environment variable name for log settings
export constexpr std::string_view log_settings = "RAY_BENCH_LOG_SETTINGS";

/// @brief Set an environment variable
/// 
/// @param name The name of the environment variable
/// @param value The value to set
/// @return True if the environment variable was set successfully, false otherwise
export bool SetEnvVar (std::string_view name, std::string_view value) noexcept
{
    return SetEnvironmentVariableA (name.data (), value.data ()) != 0;
}

/// @brief Get the value of an environment variable
/// 
/// @param name The name of the environment variable
/// @return The value of the environment variable, or std::nullopt if not found
export [[nodiscard]] std::optional<std::string> GetEnvVar (std::string_view name) noexcept
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
            RAYBENCH_LOG_DEBUG ("Environment variable '{}' not found", name);
        }
    }
    else if (result >= buffer.size ())
    {
        RAYBENCH_LOG_ERROR ("Environment variable '{}' value is too long ({} characters)", name, result);
    }
    else
    {
        return std::string (buffer.data (), result);
    }
    return std::nullopt;
}

/// @brief Unset an environment variable
/// 
/// @param name The name of the environment variable
export void UnsetEnvVar (std::string_view name) noexcept
{
    if (SetEnvironmentVariableA (name.data (), nullptr) == 0)
    {
        DWORD error = GetLastError ();
        RAYBENCH_LOG_ERROR ("Failed to unset environment variable '{}': {}", name, error);
    }
}

// ----------------------------------------------------------------------------