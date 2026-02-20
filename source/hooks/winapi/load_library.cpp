// ----------------------------------------------------------------------------

/// @brief `LoadLibrary` hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.WinAPI:HookLoadLibrary;

import RayBench.Util;
import :Guard;

using namespace std::literals;

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

/// @brief Check if the dynamic-link library loads the Steam overlay
/// @param path Dynamic-link library path
/// @return True if it does, false otherwise
static bool IsSteamOverlay (std::string_view path)
{
    return path.contains ("gameoverlayrenderer.dll") || path.contains ("gameoverlayrenderer64.dll");
}

/// @brief Check if the dynamic-link library loads the Steam overlay
/// @param path Dynamic-link library path
/// @return True if it does, false otherwise
static bool IsSteamOverlay (std::wstring_view path)
{
    return path.contains (L"gameoverlayrenderer.dll") || path.contains (L"gameoverlayrenderer64.dll");
}

// ----------------------------------------------------------------------------

/// @brief Hook libraries
static void HookLibraries ()
{
    // TODO
}

// ----------------------------------------------------------------------------

/// @brief Common implementation for 'LoadLibrary' hooks
template <typename CharT, typename Func, typename... Args>
static HMODULE LoadLibraryImpl (const CharT* lpFileName, Func real_func, Args... args)
{
    if (lpFileName && IsSteamOverlay (lpFileName))
    {
        return 0;
    }

    if (ReentrancyGuard<LoadLibraryTag>::IsActive ())
    {
        return real_func (lpFileName, args...);
    }

    ReentrancyGuard<LoadLibraryTag> guard;

    HMODULE module = real_func (lpFileName, args...);
    DWORD last_error = GetLastError ();

    if (guard.GetRef () == 1)
    {
        HookLibraries ();

        if constexpr (std::is_same_v<CharT, char>)
        {
            RAYBENCH_LOG_TRACE ("Hooked libraries while loading DLL {}...",
                                lpFileName);
        }
        else
        {
            RAYBENCH_LOG_TRACE ("Hooked libraries while loading DLL {}...",
                                raybench::util::string::WideToNarrow (lpFileName));
        }
    }

    SetLastError (last_error);

    return module;
}

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
    return LoadLibraryImpl (lpLibFileName, Real_LoadLibraryA);
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
    return LoadLibraryImpl (lpLibFileName, Real_LoadLibraryExA, hFile, dwFlags);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryw
static HMODULE Hook_LoadLibraryW (LPCWSTR lpLibFileName)
{
    return LoadLibraryImpl (lpLibFileName, Real_LoadLibraryW);
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
    return LoadLibraryImpl (lpLibFileName, Real_LoadLibraryExW, hFile, dwFlags);
}

// ----------------------------------------------------------------------------

/// @brief Hook `LoadLibrary` API calls
/// @return True if successful, false otherwise
export bool HookLoadLibrary ()
{
    bool result = true;

    auto Hook = [&result] (auto& real_func, auto hook_func, std::string_view func_name)
        {
            if (!raybench::util::HookAPICall (reinterpret_cast<PVOID*>(&real_func),
                                              reinterpret_cast<PVOID>(hook_func)))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to hook '{}'", func_name);
                result = false;
            }
        };

    Hook (Real_FreeLibrary, Hook_FreeLibrary, "FreeLibrary"sv);
    Hook (Real_LoadLibraryA, Hook_LoadLibraryA, "LoadLibraryA"sv);
    Hook (Real_LoadLibraryExA, Hook_LoadLibraryExA, "LoadLibraryExA"sv);
    Hook (Real_LoadLibraryW, Hook_LoadLibraryW, "LoadLibraryW"sv);
    Hook (Real_LoadLibraryExW, Hook_LoadLibraryExW, "LoadLibraryExW"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `LoadLibrary` API calls
/// @return True if successful, false otherwise
export bool UnhookLoadLibrary ()
{
    bool result = true;

    auto Unhook = [&result] (auto& real_func, auto hook_func, std::string_view func_name)
        {
            if (!raybench::util::UnhookAPICall (reinterpret_cast<PVOID*>(&real_func),
                                              reinterpret_cast<PVOID>(hook_func)))
            {
                RAYBENCH_LOG_CRITICAL ("Failed to unhook '{}'", func_name);
                result = false;
            }
        };

    Unhook (Real_FreeLibrary, Hook_FreeLibrary, "FreeLibrary"sv);
    Unhook (Real_LoadLibraryA, Hook_LoadLibraryA, "LoadLibraryA"sv);
    Unhook (Real_LoadLibraryExA, Hook_LoadLibraryExA, "LoadLibraryExA"sv);
    Unhook (Real_LoadLibraryW, Hook_LoadLibraryW, "LoadLibraryW"sv);
    Unhook (Real_LoadLibraryExW, Hook_LoadLibraryExW, "LoadLibraryExW"sv);

    Real_FreeLibrary = FreeLibrary;
    Real_LoadLibraryA = LoadLibraryA;
    Real_LoadLibraryExA = LoadLibraryExA;
    Real_LoadLibraryW = LoadLibraryW;
    Real_LoadLibraryExW = LoadLibraryExW;

    return result;
}

}

// ----------------------------------------------------------------------------