// ============================================================================

/// @brief `CreateProcess` hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.Payload:HookCreateProcess;

import std;
import RayBench.Util;
import :Guard;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::payload
{

///< Array of blacklisted processes to reinject into
constexpr std::array<std::string_view, 13> blacklisted_processes = {
    "fxc.exe"sv,
    "cmd.exe"sv,
    "dev.exe"sv,
    "steamwebhelper.exe"sv,
    "gldriverquery.exe"sv,
    "gldriverquery64.exe"sv,
    "vulkandriverquery.exe"sv,
    "vulkandriverquery64.exe"sv,
    "galaxyclient helper.exe"sv,
    "gog galaxy notifications renderer.exe"sv,
    "galaxyoverlay.exe"sv,
    "epicwebhelper.exe"sv,
    "epiconlineservicesuserhelper.exe"sv,
};

// ----------------------------------------------------------------------------

using pfn_CreateProcessA = BOOL (WINAPI*)(LPCSTR,
                                          LPSTR,
                                          LPSECURITY_ATTRIBUTES,
                                          LPSECURITY_ATTRIBUTES,
                                          BOOL,
                                          DWORD,
                                          LPVOID,
                                          LPCSTR,
                                          LPSTARTUPINFOA,
                                          LPPROCESS_INFORMATION);
using pfn_CreateProcessW = BOOL (WINAPI*)(LPCWSTR,
                                          LPWSTR,
                                          LPSECURITY_ATTRIBUTES,
                                          LPSECURITY_ATTRIBUTES,
                                          BOOL,
                                          DWORD,
                                          LPVOID,
                                          LPCWSTR,
                                          LPSTARTUPINFOW,
                                          LPPROCESS_INFORMATION);

pfn_CreateProcessA Original_CreateProcessA = CreateProcessA;
pfn_CreateProcessW Original_CreateProcessW = CreateProcessW;

///< `HookTag` for re-entrancy guard of `CreateProcess` hooks
struct CreateProcessTag
{};

// ============================================================================

/// @brief Check if the process is blacklisted
/// 
/// @param input Process
/// @return True if the process is blacklisted, false otherwise
[[nodiscard]] static bool IsBlacklisted (std::string_view input)
{
    if (input.empty ())
    {
        return false;
    }

    auto lower_input = input
        | std::views::transform ([] (unsigned char c)
                                 {
                                     return static_cast<char>(std::tolower (c));
                                 })
        | std::ranges::to<std::string> ();

    return std::ranges::any_of (blacklisted_processes, [&] (std::string_view process)
                                {
                                    return lower_input.find (process) != std::string::npos;
                                });
}

// ----------------------------------------------------------------------------

/// @brief Check whether to block injection into a new process
/// 
/// @tparam CharT Character type
/// @param application_name Application name of the new process
/// @param command_line Command line of the new process
/// @return True if the new process is blacklisted, false otherwise
export template <typename CharT>
[[nodiscard]] bool BlockInjection (const CharT* application_name, const CharT* command_line)
{
    auto Check = [] (const CharT* ptr) -> bool
        {
            if (!ptr)
            {
                return false;
            }

            if constexpr (std::is_same_v<CharT, char>)
            {
                return IsBlacklisted (ptr);
            }
            else
            {
                return IsBlacklisted (raybench::util::string::WideToNarrow (ptr));
            }
        };

    return Check (application_name) || Check (command_line);
}

// ============================================================================

/// @brief Common implementation for `CreateProcess` hooks
template <typename CharT, typename Func, typename... Args>
static BOOL CreateProcessImpl (const CharT* lpApplicationName,
                               CharT* lpCommandLine,
                               Func real_func,
                               Args... args)
{
    if (ReentrancyGuard<CreateProcessTag>::IsActive ())
    {
        return real_func (lpApplicationName, lpCommandLine, args...);
    }

    ReentrancyGuard<CreateProcessTag> guard;

    if (BlockInjection (lpApplicationName, lpCommandLine) == true)
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            RAYBENCH_LOG_TRACE_ONCE ("Blocking reinjection to a new process: application name {}, command line {}...",
                                     lpApplicationName ? lpApplicationName : "",
                                     lpCommandLine ? lpCommandLine : "");
        }
        else
        {
            RAYBENCH_LOG_TRACE_ONCE ("Blocking reinjection to a new process: application name {}, command line {}...",
                                     lpApplicationName ? raybench::util::string::WideToNarrow (lpApplicationName) : "",
                                     lpCommandLine ? raybench::util::string::WideToNarrow (lpCommandLine) : "");
        }
        return real_func (lpApplicationName, lpCommandLine, args...);
    }

    RAYBENCH_LOG_TRACE_ONCE ("Reinjecting and reconnecting to a new process...");

    auto winapi_dll_path = raybench::util::EnvVar::Get (raybench::util::EnvVar::winapi_dll_path);
    if (winapi_dll_path.has_value ())
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            return raybench::util::LaunchInjectA (lpApplicationName, lpCommandLine, args..., winapi_dll_path.value ().data ());
        }
        else
        {
            return raybench::util::LaunchInjectW (lpApplicationName, lpCommandLine, args..., winapi_dll_path.value ().data ());
        }
    }
    else
    {
        RAYBENCH_LOG_CRITICAL ("Environment variable '{}' not set",
                               raybench::util::EnvVar::winapi_dll_path);
        return FALSE;
    }
}

// ----------------------------------------------------------------------------

/// @brief Create a new process and its primary thread
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa
static BOOL WINAPI Hooked_CreateProcessA (LPCSTR lpApplicationName,
                                        LPSTR lpCommandLine,
                                        LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                        LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                        BOOL bInheritHandles,
                                        DWORD dwCreationFlags,
                                        LPVOID lpEnvironment,
                                        LPCSTR lpCurrentDirectory,
                                        LPSTARTUPINFOA lpStartupInfo,
                                        LPPROCESS_INFORMATION lpProcessInformation)
{
    return CreateProcessImpl (lpApplicationName,
                              lpCommandLine,
                              Original_CreateProcessA,
                              lpProcessAttributes,
                              lpThreadAttributes,
                              bInheritHandles,
                              dwCreationFlags,
                              lpEnvironment,
                              lpCurrentDirectory,
                              lpStartupInfo,
                              lpProcessInformation);
}

// ----------------------------------------------------------------------------

/// @brief Create a new process and its primary thread
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
static BOOL WINAPI Hooked_CreateProcessW (LPCWSTR lpApplicationName,
                                        LPWSTR lpCommandLine,
                                        LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                        LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                        BOOL bInheritHandles,
                                        DWORD dwCreationFlags,
                                        LPVOID lpEnvironment,
                                        LPCWSTR lpCurrentDirectory,
                                        LPSTARTUPINFOW lpStartupInfo,
                                        LPPROCESS_INFORMATION lpProcessInformation)
{
    return CreateProcessImpl (lpApplicationName,
                              lpCommandLine,
                              Original_CreateProcessW,
                              lpProcessAttributes,
                              lpThreadAttributes,
                              bInheritHandles,
                              dwCreationFlags,
                              lpEnvironment,
                              lpCurrentDirectory,
                              lpStartupInfo,
                              lpProcessInformation);
}

// ============================================================================

/// @brief Hook `CreateProcess` API calls
/// @return True if successful, false otherwise
export bool HookCreateProcess ()
{
    bool result = true;

    result &= HookWrap (Original_CreateProcessA, Hooked_CreateProcessA, "CreateProcessA"sv);
    result &= HookWrap (Original_CreateProcessW, Hooked_CreateProcessW, "CreateProcessW"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `CreateProcess` API calls
/// @return True if successful, false otherwise
export bool UnhookCreateProcess ()
{
    bool result = true;

    result &= UnhookWrap (Original_CreateProcessA, Hooked_CreateProcessA, "CreateProcessA"sv);
    result &= UnhookWrap (Original_CreateProcessW, Hooked_CreateProcessW, "CreateProcessW"sv);

    Original_CreateProcessA = CreateProcessA;
    Original_CreateProcessW = CreateProcessW;

    return result;
}

}

// ----------------------------------------------------------------------------