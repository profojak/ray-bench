// ============================================================================

/// @brief `LoadLibrary` hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.Payload:HookLoadLibrary;

import RayBench.Hook;
import RayBench.Util;
import :Guard;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::payload
{

///< D3D12 dynamic-link library load flag
static bool d3d12_flag = false;
///< D3D12 Core dynamic-link library load flag
static bool d3d12core_flag = false;
///< DXGI dynamic-link library load flag
static bool dxgi_flag = false;
///< NVAPI dynamic-link library load flag
static bool nvapi_flag = false;

// ----------------------------------------------------------------------------

///< Array of blacklisted libraries to not hook into
constexpr std::array<std::string_view, 30> blacklisted_libraries = {
    "kernel32.dll"sv,
    "user32.dll"sv,
    "gdi32.dll"sv,
    "advapi32.dll"sv,
    "shell32.dll"sv,
    "setupapi.dll"sv,
    "version.dll"sv,
    // Cryptography, security, and trust
    "crypt32.dll"sv,
    "wintrust.dll"sv,
    "msasn1.dll"sv,
    "cryptnet.dll"sv,
    "cryptbase.dll"sv,
    "secur32.dll"sv,
    "bcrypt.dll"sv,
    "wldp.dll"sv,
    // Device and driver
    "drvstore.dll"sv,
    "devobj.dll"sv,
    // Networking
    "iphlpapi.dll"sv,
    // Controller input
    "xinput1_4.dll"sv,
    "xinput9_1_0.dll"sv,
    // Legacy multimedia
    "winmm.dll"sv,
    // NVIDIA PhysX
    "pxfoundation_x64.dll"sv,
    "physx3common_x64.dll"sv,
    // Razer Chroma integration
    "cchromaeditorlibrary64.dll"sv,
    "rzchromasdk64.dll"sv,
    // Steam
    "steamclient64.dll"sv,
    "gameservicessteam.dll"sv,
    "gameoverlayrenderer.dll"sv,
    "gameoverlayrenderer64.dll"sv,
    // NVIDIA GeForce Now
    "gfnruntimesdk.dll"sv
};

// ----------------------------------------------------------------------------

using pfn_FreeLibrary = BOOL (WINAPI*)(HMODULE);
using pfn_LoadLibraryA = HMODULE (WINAPI*)(LPCSTR);
using pfn_LoadLibraryExA = HMODULE (WINAPI*)(LPCSTR, HANDLE, DWORD);
using pfn_LoadLibraryW = HMODULE (WINAPI*)(LPCWSTR);
using pfn_LoadLibraryExW = HMODULE (WINAPI*)(LPCWSTR, HANDLE, DWORD);

pfn_FreeLibrary Original_FreeLibrary = FreeLibrary;
pfn_LoadLibraryA Original_LoadLibraryA = LoadLibraryA;
pfn_LoadLibraryExA Original_LoadLibraryExA = LoadLibraryExA;
pfn_LoadLibraryW Original_LoadLibraryW = LoadLibraryW;
pfn_LoadLibraryExW Original_LoadLibraryExW = LoadLibraryExW;

///< `HookTag` for re-entrancy guard of `CreateProcess` hooks
struct LoadLibraryTag
{};

// ============================================================================

/// @brief Check if the library is blacklisted and should not trigger hooking
/// 
/// @param input Dynamic-link library
/// @return True if the library is blacklisted, false otherwise
template <typename CharT>
static bool IsBlacklisted (std::basic_string_view<CharT> path)
{
    if (path.empty ())
        return false;

    std::string lower_name;
    if constexpr (std::is_same_v<CharT, char>)
    {
        const auto pos = path.find_last_of ("\\/");
        const auto filename = (pos == std::basic_string_view<CharT>::npos) ? path : path.substr (pos + 1);
        lower_name = filename
            | std::views::transform ([] (unsigned char c)
                                     {
                                         return static_cast<char>(std::tolower (c));
                                     })
            | std::ranges::to<std::string> ();
    }
    else
    {
        const auto pos = path.find_last_of (L"\\/");
        const auto filename = (pos == std::basic_string_view<CharT>::npos) ? path : path.substr (pos + 1);
        lower_name = raybench::util::string::WideToNarrow (filename)
            | std::views::transform ([] (unsigned char c)
                                     {
                                         return static_cast<char>(std::tolower (c));
                                     })
            | std::ranges::to<std::string> ();
    }

    // Ignore Windows API Set forwarders
    if (lower_name.starts_with ("api-ms-win-"))
        return true;

    // Check against the blacklist array
    return std::ranges::contains (blacklisted_libraries, lower_name);
}

// ----------------------------------------------------------------------------

/// @brief Hook API calls if library is already loaded
static bool HookLibrary (std::string_view system_lib, bool (*hook_func)())
{
    if (hook_func () == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook {} API calls!",
                               system_lib);
        return false;
    }

    RAYBENCH_LOG_DEBUG ("Hooked {} API calls",
                        system_lib);

    return true;
}

// ----------------------------------------------------------------------------

/// @brief Check if the target libraries are already loaded and hook API calls
static void HookOnLoad ()
{
    if (d3d12_flag == false)
    {
        HMODULE module = GetModuleHandleA ("d3d12.dll");
        if (module)
            d3d12_flag = HookLibrary ("d3d12.dll", raybench::hook::HookD3D12);
    }
    if (d3d12core_flag == false)
    {
        HMODULE module = GetModuleHandleA ("d3d12core.dll");
        if (module)
            d3d12core_flag = HookLibrary ("d3d12core.dll", raybench::hook::HookD3D12);
    }
    if (dxgi_flag == false)
    {
        HMODULE module = GetModuleHandleA ("dxgi.dll");
        if (module)
            dxgi_flag = HookLibrary ("dxgi.dll", raybench::hook::HookDXGI);
    }
    if (nvapi_flag == false)
    {
        HMODULE module = GetModuleHandleA ("nvapi64.dll");
        if (module)
            nvapi_flag = HookLibrary ("nvapi64.dll", raybench::hook::HookNvAPI);
    }
}

// ============================================================================

/// @brief Common implementation for 'LoadLibrary' hooks
template <typename CharT, typename Func, typename... Args>
static HMODULE LoadLibraryImpl (const CharT* lpFileName, Func real_func, Args... args)
{
    if (ReentrancyGuard<LoadLibraryTag>::IsActive ())
    {
        return real_func (lpFileName, args...);
    }

    ReentrancyGuard<LoadLibraryTag> guard;

    if (IsBlacklisted<CharT> (lpFileName))
    {
        RAYBENCH_LOG_TRACE_ONCE ("Blocking hooking while loading library: process ID {}...",
                                 GetProcessId (GetCurrentProcess ()));
        return real_func (lpFileName, args...);
    }

    HMODULE module = real_func (lpFileName, args...);
    DWORD last_error = GetLastError ();

    if (guard.GetRef () == 1)
    {
        HookOnLoad ();
    }

    SetLastError (last_error);

    return module;
}

// ----------------------------------------------------------------------------

/// @brief Free the loaded dynamic-link library module and, if necessary,
///        decrement its reference count
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary
static BOOL Hooked_FreeLibrary (HMODULE hLibModule)
{
    return Original_FreeLibrary (hLibModule);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
static HMODULE Hooked_LoadLibraryA (LPCSTR lpLibFileName)
{
    return LoadLibraryImpl (lpLibFileName, Original_LoadLibraryA);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexa
static HMODULE Hooked_LoadLibraryExA (LPCSTR lpLibFileName,
                                      HANDLE hFile,
                                      DWORD  dwFlags)
{
    return LoadLibraryImpl (lpLibFileName, Original_LoadLibraryExA, hFile, dwFlags);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryw
static HMODULE Hooked_LoadLibraryW (LPCWSTR lpLibFileName)
{
    return LoadLibraryImpl (lpLibFileName, Original_LoadLibraryW);
}

// ----------------------------------------------------------------------------

/// @brief Load the specified module into the address space of the calling
///        process
/// 
/// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw
static HMODULE Hooked_LoadLibraryExW (LPCWSTR lpLibFileName,
                                      HANDLE  hFile,
                                      DWORD   dwFlags)
{
    return LoadLibraryImpl (lpLibFileName, Original_LoadLibraryExW, hFile, dwFlags);
}

// ============================================================================

/// @brief Hook `LoadLibrary` API calls
/// @return True if successful, false otherwise
export bool HookLoadLibrary ()
{
    // Check if libraries are already loaded (they may be statically linked)
    // and if so, hook the libraries immediately
    HookOnLoad ();

    bool result = true;

    result &= HookWrap (Original_FreeLibrary, Hooked_FreeLibrary, "FreeLibrary"sv);
    result &= HookWrap (Original_LoadLibraryA, Hooked_LoadLibraryA, "LoadLibraryA"sv);
    result &= HookWrap (Original_LoadLibraryExA, Hooked_LoadLibraryExA, "LoadLibraryExA"sv);
    result &= HookWrap (Original_LoadLibraryW, Hooked_LoadLibraryW, "LoadLibraryW"sv);
    result &= HookWrap (Original_LoadLibraryExW, Hooked_LoadLibraryExW, "LoadLibraryExW"sv);

    return result;
}

// ----------------------------------------------------------------------------

/// @brief Unhook `LoadLibrary` API calls
/// @return True if successful, false otherwise
export bool UnhookLoadLibrary ()
{
    bool result = true;

    result &= UnhookWrap (Original_FreeLibrary, Hooked_FreeLibrary, "FreeLibrary"sv);
    result &= UnhookWrap (Original_LoadLibraryA, Hooked_LoadLibraryA, "LoadLibraryA"sv);
    result &= UnhookWrap (Original_LoadLibraryExA, Hooked_LoadLibraryExA, "LoadLibraryExA"sv);
    result &= UnhookWrap (Original_LoadLibraryW, Hooked_LoadLibraryW, "LoadLibraryW"sv);
    result &= UnhookWrap (Original_LoadLibraryExW, Hooked_LoadLibraryExW, "LoadLibraryExW"sv);

    Original_FreeLibrary = FreeLibrary;
    Original_LoadLibraryA = LoadLibraryA;
    Original_LoadLibraryExA = LoadLibraryExA;
    Original_LoadLibraryW = LoadLibraryW;
    Original_LoadLibraryExW = LoadLibraryExW;

    return result;
}

}

// ----------------------------------------------------------------------------