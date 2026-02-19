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

namespace raybench::util::WinAPI
{

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