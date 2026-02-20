// ----------------------------------------------------------------------------

/// @brief `CreateProcess` hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.WinAPI:HookCreateProcess;

import std;
import RayBench.Util;
import :Guard;

using namespace std::literals;

namespace raybench::util::WinAPI
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

pfn_CreateProcessA Real_CreateProcessA = CreateProcessA;
pfn_CreateProcessW Real_CreateProcessW = CreateProcessW;

///< `HookTag` for re-entrancy guard of `CreateProcess` hooks
struct CreateProcessTag
{};

// ----------------------------------------------------------------------------

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

/// @brief Convert wide string to ordinary one
/// 
/// @param wstr Wide string
/// @return Converted wide string
static std::string ConvertWideString (std::wstring_view wstr)
{
    if (wstr.empty ())
    {
        return std::string {};
    }

    int size_needed = WideCharToMultiByte (CP_UTF8, 0, wstr.data (), (int) wstr.size (), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        RAYBENCH_LOG_ERROR ("Failed to convert wide string to narrow string: {}", GetLastError ());
        return std::string {};
    }

    std::string str (size_needed, 0);
    int result = WideCharToMultiByte (CP_UTF8, 0, wstr.data (), (int) wstr.size (), str.data (), size_needed, nullptr, nullptr);
    if (result <= 0)
    {
        RAYBENCH_LOG_ERROR ("Failed to convert wide string to narrow string: {}", GetLastError ());
        return std::string {};
    }

    return str;
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
                return IsBlacklisted (ConvertWideString (ptr));
            }
        };

    return Check (application_name) || Check (command_line);
}

// ----------------------------------------------------------------------------

/// @brief Create a new process and its primary thread
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa
static BOOL WINAPI Hook_CreateProcessA (LPCSTR lpApplicationName,
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
    if (raybench::util::WinAPI::ReentrancyGuard<CreateProcessTag>::IsActive ())
    {
        return Real_CreateProcessA (lpApplicationName,
                                    lpCommandLine,
                                    lpProcessAttributes,
                                    lpThreadAttributes,
                                    bInheritHandles,
                                    dwCreationFlags,
                                    lpEnvironment,
                                    lpCurrentDirectory,
                                    lpStartupInfo,
                                    lpProcessInformation);
    }

    ReentrancyGuard<CreateProcessTag> guard;

    if (BlockInjection (lpApplicationName, lpCommandLine) == true)
    {
        RAYBENCH_LOG_WARNING ("Blocked reinjection to a new process: application name '{}', command line '{}'",
                              lpApplicationName, lpCommandLine);
        return Real_CreateProcessA (lpApplicationName,
                                    lpCommandLine,
                                    lpProcessAttributes,
                                    lpThreadAttributes,
                                    bInheritHandles,
                                    dwCreationFlags,
                                    lpEnvironment,
                                    lpCurrentDirectory,
                                    lpStartupInfo,
                                    lpProcessInformation);
    }

    RAYBENCH_LOG_TRACE ("Reinjecting and reconnecting to a new process...");

    auto winapi_dll_path = raybench::util::EnvVar::Get (raybench::util::EnvVar::winapi_dll_path);
    if (winapi_dll_path.has_value ())
    {
        return raybench::util::LaunchInjectA (lpApplicationName,
                                              lpCommandLine,
                                              lpProcessAttributes,
                                              lpThreadAttributes,
                                              bInheritHandles,
                                              dwCreationFlags,
                                              lpEnvironment,
                                              lpCurrentDirectory,
                                              lpStartupInfo,
                                              lpProcessInformation,
                                              winapi_dll_path.value ().data ());
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
/// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
static BOOL WINAPI Hook_CreateProcessW (LPCWSTR lpApplicationName,
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
    if (raybench::util::WinAPI::ReentrancyGuard<CreateProcessTag>::IsActive ())
    {
        return Real_CreateProcessW (lpApplicationName,
                                    lpCommandLine,
                                    lpProcessAttributes,
                                    lpThreadAttributes,
                                    bInheritHandles,
                                    dwCreationFlags,
                                    lpEnvironment,
                                    lpCurrentDirectory,
                                    lpStartupInfo,
                                    lpProcessInformation);
    }

    ReentrancyGuard<CreateProcessTag> guard;

    if (BlockInjection (lpApplicationName, lpCommandLine) == true)
    {
        RAYBENCH_LOG_WARNING ("Blocked reinjection to a new process: application name '{}', command line '{}'",
                              ConvertWideString (lpApplicationName), ConvertWideString (lpCommandLine));
        return Real_CreateProcessW (lpApplicationName,
                                    lpCommandLine,
                                    lpProcessAttributes,
                                    lpThreadAttributes,
                                    bInheritHandles,
                                    dwCreationFlags,
                                    lpEnvironment,
                                    lpCurrentDirectory,
                                    lpStartupInfo,
                                    lpProcessInformation);
    }

    RAYBENCH_LOG_TRACE ("Reinjecting and reconnecting to a new process...");

    auto winapi_dll_path = raybench::util::EnvVar::Get (raybench::util::EnvVar::winapi_dll_path);
    if (winapi_dll_path.has_value ())
    {
        return raybench::util::LaunchInjectW (lpApplicationName,
                                              lpCommandLine,
                                              lpProcessAttributes,
                                              lpThreadAttributes,
                                              bInheritHandles,
                                              dwCreationFlags,
                                              lpEnvironment,
                                              lpCurrentDirectory,
                                              lpStartupInfo,
                                              lpProcessInformation,
                                              winapi_dll_path.value ().data ());
    }
    else
    {
        RAYBENCH_LOG_CRITICAL ("Environment variable '{}' not set",
                               raybench::util::EnvVar::winapi_dll_path);
        return FALSE;
    }
}

// ----------------------------------------------------------------------------

/// @brief Hook `CreateProcess` API calls
/// @return True if successful, false otherwise
export bool HookCreateProcess ()
{
    bool result = raybench::util::HookAPICall (&(PVOID&) Real_CreateProcessA, Hook_CreateProcessA);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'CreateProcessA'");
    }

    result = raybench::util::HookAPICall (&(PVOID&) Real_CreateProcessW, Hook_CreateProcessW);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'CreateProcessW'");
    }

    return true;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `CreateProcess` API calls
/// @return True if successful, false otherwise
export bool UnhookCreateProcess ()
{
    bool result = raybench::util::UnhookAPICall (&(PVOID&) Real_CreateProcessA, Hook_CreateProcessA);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'CreateProcessA'");
    }

    result = raybench::util::UnhookAPICall (&(PVOID&) Real_CreateProcessW, Hook_CreateProcessW);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'CreateProcessW'");
    }

    Real_CreateProcessA = CreateProcessA;
    Real_CreateProcessW = CreateProcessW;

    return true;
}

}

// ----------------------------------------------------------------------------