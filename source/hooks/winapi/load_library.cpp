// ----------------------------------------------------------------------------

/// @brief `LoadLibrary` hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.WinAPI:HookLoadLibrary;

import RayBench.Util;

namespace raybench::util::WinAPI
{

using pfn_FreeLibrary = BOOL (WINAPI*)(HMODULE);
using pfn_LoadLibraryA = HMODULE (WINAPI*)(LPCSTR);
using pfn_LoadLibraryExA = HMODULE (WINAPI*)(LPCSTR, HANDLE, DWORD);
using pfn_LoadLibraryW = HMODULE (WINAPI*)(LPCWSTR);
using pfn_LoadLibraryExW = HMODULE (WINAPI*)(LPCWSTR, HANDLE, DWORD);

pfn_FreeLibrary Real_FreeLibrary = FreeLibrary;
pfn_LoadLibraryA Real_LoadLibraryA = LoadLibraryA;
pfn_LoadLibraryExA Real_LoadLibraryExA = LoadLibraryExA;
pfn_LoadLibraryW Real_LoadLibraryW = LoadLibraryW;
pfn_LoadLibraryExW Real_LoadLibraryExW = LoadLibraryExW;

///< `HookTag` for re-entrancy guard of `CreateProcess` hooks
struct LoadLibraryTag
{};

// ----------------------------------------------------------------------------

/// @brief Free the loaded dynamic-link library module and, if necessary,
///        decrement its reference count
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary
static BOOL Hook_FreeLibrary (HMODULE hLibModule)
{
    return Real_FreeLibrary (hLibModule);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
static HMODULE Hook_LoadLibraryA (LPCSTR lpLibFileName)
{
    return Real_LoadLibraryA (lpLibFileName);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexa
static HMODULE Hook_LoadLibraryExA (LPCSTR lpLibFileName,
                                    HANDLE hFile,
                                    DWORD  dwFlags)
{
    return Real_LoadLibraryExA (lpLibFileName, hFile, dwFlags);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryw
static HMODULE Hook_LoadLibraryW (LPCWSTR lpLibFileName)
{
    return Real_LoadLibraryW (lpLibFileName);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw
static HMODULE Hook_LoadLibraryExW (LPCWSTR lpLibFileName,
                                    HANDLE  hFile,
                                    DWORD   dwFlags)
{
    return Real_LoadLibraryExW (lpLibFileName, hFile, dwFlags);
}

// ----------------------------------------------------------------------------

/// @brief Hook `LoadLibrary` API calls
/// @return True if successful, false otherwise
export bool HookLoadLibrary ()
{
    bool result = raybench::util::HookAPICall (&(PVOID&) Real_FreeLibrary, Hook_FreeLibrary);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'FreeLibrary'");
    }

    result = raybench::util::HookAPICall (&(PVOID&) Real_LoadLibraryA, Hook_LoadLibraryA);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'LoadLibraryA'");
    }

    result = raybench::util::HookAPICall (&(PVOID&) Real_LoadLibraryExA, Hook_LoadLibraryExA);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'LoadLibraryExA'");
    }

    result = raybench::util::HookAPICall (&(PVOID&) Real_LoadLibraryW, Hook_LoadLibraryW);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'LoadLibraryW'");
    }

    result = raybench::util::HookAPICall (&(PVOID&) Real_LoadLibraryExW, Hook_LoadLibraryExW);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook 'LoadLibraryExW'");
    }

    return true;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `LoadLibrary` API calls
/// @return True if successful, false otherwise
export bool UnhookLoadLibrary ()
{
    bool result = raybench::util::UnhookAPICall (&(PVOID&) Real_FreeLibrary, Hook_FreeLibrary);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'FreeLibrary'");
    }

    result = raybench::util::UnhookAPICall (&(PVOID&) Real_LoadLibraryA, Hook_LoadLibraryA);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'LoadLibraryA'");
    }

    result = raybench::util::UnhookAPICall (&(PVOID&) Real_LoadLibraryExA, Hook_LoadLibraryExA);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'LoadLibraryExA'");
    }

    result = raybench::util::UnhookAPICall (&(PVOID&) Real_LoadLibraryW, Hook_LoadLibraryW);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'LoadLibraryW'");
    }

    result = raybench::util::UnhookAPICall (&(PVOID&) Real_LoadLibraryExW, Hook_LoadLibraryExW);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to unhook 'LoadLibraryExW'");
    }

    Real_FreeLibrary = FreeLibrary;
    Real_LoadLibraryA = LoadLibraryA;
    Real_LoadLibraryExA = LoadLibraryExA;
    Real_LoadLibraryW = LoadLibraryW;
    Real_LoadLibraryExW = LoadLibraryExW;

    return true;
}

}

// ----------------------------------------------------------------------------