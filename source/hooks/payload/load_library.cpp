// ============================================================================

/// @brief `LoadLibrary` hooks

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

export module RayBench.Payload:HookLoadLibrary;

import RayBench.Util;
import :Guard;

using namespace std::literals;

namespace raybench::payload
{

///< D3D12 dynamic-link library hook flag
static bool hooked_d3d12_module = false;
///< DXGI dynamic-link library hook flag
static bool hooked_dxgi_module = false;
///< NVAPI dynamic-link library hook flag
static bool hooked_nvapi_module = false;

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
using pfn_Hook = bool (*)();

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
    for (const auto& lib : blacklisted_libraries)
    {
        if (lower_name == lib)
            return true;
    }

    return false;
}

// ----------------------------------------------------------------------------

/// @brief Hook library
/// 
/// @param system_lib System library to check if already loaded
/// @param hook_lib Hook library with custom hooks of system library functions
/// @return Handle to hooked library if successful, `nullptr` otherwise
HMODULE HookLibrary (std::string_view system_lib, std::filesystem::path hook_lib)
{
    HMODULE hook_module = nullptr;
    HMODULE system_module = GetModuleHandleA (system_lib.data ());
    if (system_module == nullptr)
    {
        RAYBENCH_LOG_TRACE ("Skipping unloaded dynamic-link library: {}...",
                            system_lib);
        return nullptr;
    }

    const auto& hook_filename = hook_lib.filename ().string ();
    hook_module = Original_LoadLibraryA (hook_lib.string ().data ());
    if (hook_module == nullptr)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to load {}: {}!",
                               hook_filename, GetLastError ());
        return nullptr;
    }

    pfn_Hook hook_func = reinterpret_cast<pfn_Hook> (GetProcAddress (hook_module, "Hook"));
    if (hook_func == nullptr)
    {
        RAYBENCH_LOG_CRITICAL ("{} does not export 'Hook' function!",
                               hook_filename);
        return nullptr;
    }

    bool result = hook_func ();
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to hook {}!",
                               hook_filename);
        return nullptr;
    }

    RAYBENCH_LOG_INFO ("Hooked {} API calls with hooks from {}",
                       system_lib, hook_filename);

    return hook_module;
}

// ----------------------------------------------------------------------------

/// @brief Hook libraries
/// 
/// @return True if hooked, false otherwise
static void HookLibraries ()
{
    static std::filesystem::path dll_path;

    if (dll_path.empty ())
    {
        const auto env_var = raybench::util::EnvVar::Get (raybench::util::EnvVar::winapi_dll_path);
        if (env_var.has_value ())
        {
            dll_path = std::filesystem::path (env_var.value ()).parent_path ();
        }
        else
        {
            RAYBENCH_LOG_CRITICAL ("Environment variable for required dynamic-link libraries path not set!");
            return;
        }
    }

    if (hooked_d3d12_module == false)
    {
        // TODO
    }

    if (hooked_dxgi_module == false)
    {
        // TODO
    }

    if (hooked_nvapi_module == false)
    {
        // TODO
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
        HookLibraries ();
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

    Hook (Original_FreeLibrary, Hooked_FreeLibrary, "FreeLibrary"sv);
    Hook (Original_LoadLibraryA, Hooked_LoadLibraryA, "LoadLibraryA"sv);
    Hook (Original_LoadLibraryExA, Hooked_LoadLibraryExA, "LoadLibraryExA"sv);
    Hook (Original_LoadLibraryW, Hooked_LoadLibraryW, "LoadLibraryW"sv);
    Hook (Original_LoadLibraryExW, Hooked_LoadLibraryExW, "LoadLibraryExW"sv);

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

    Unhook (Original_FreeLibrary, Hooked_FreeLibrary, "FreeLibrary"sv);
    Unhook (Original_LoadLibraryA, Hooked_LoadLibraryA, "LoadLibraryA"sv);
    Unhook (Original_LoadLibraryExA, Hooked_LoadLibraryExA, "LoadLibraryExA"sv);
    Unhook (Original_LoadLibraryW, Hooked_LoadLibraryW, "LoadLibraryW"sv);
    Unhook (Original_LoadLibraryExW, Hooked_LoadLibraryExW, "LoadLibraryExW"sv);

    Original_FreeLibrary = FreeLibrary;
    Original_LoadLibraryA = LoadLibraryA;
    Original_LoadLibraryExA = LoadLibraryExA;
    Original_LoadLibraryW = LoadLibraryW;
    Original_LoadLibraryExW = LoadLibraryExW;

    return result;
}

}

// ----------------------------------------------------------------------------