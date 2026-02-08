// ----------------------------------------------------------------------------

/// @brief Microsoft Detours hooking library wrapper
/// 
/// https://github.com/microsoft/Detours/wiki/Using-Detours
/// https://github.com/microsoft/Detours/wiki/Reference

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <detours/detours.h>

export module RayBench.Util:Detours;

import std;

namespace raybench::util
{

// ----------------------------------------------------------------------------

/// @brief Paths related to a process to launch and inject into
struct ProcessPaths
{
    ///< Path to the application executable
    std::string app_path;
    ///< Command line arguments for the application
    std::string arguments;
    ///< Working directory for the application
    std::string app_directory;
};

// ----------------------------------------------------------------------------

/// @brief Parse the full command line of a process to extract its paths
/// 
/// @param full_command Full command line of the process, including the
///                     executable path and its arguments
/// @return Optional containing the parsed paths if successful,
///         `std::nullopt` otherwise
export [[nodiscard]] std::optional<ProcessPaths> GetProcessPaths (std::string_view full_command)
{
    using namespace std::string_view_literals;

    const auto exe_pos = full_command.find (".exe"sv);
    if (exe_pos == std::string_view::npos)
    {
        return std::nullopt;
    }

    const size_t args_start = exe_pos + 4;
    std::filesystem::path path_view {full_command.substr (0, args_start)};

    return ProcessPaths {
        .app_path = path_view.string (),
        .arguments = std::format ("\"{}\"{}", path_view.string (), full_command.substr (args_start)),
        .app_directory = path_view.parent_path ().string ()
    };
}

// ----------------------------------------------------------------------------

/// @brief Launch a process and inject dynamic-link libraries into it using
///        Detours
/// 
/// @param process_paths Paths related to the process to launch and inject into
/// @param startup_info Startup information for the application
/// @param process_info Process information for the application
/// @param dlls_count Number of dynamic-link libraries to inject
/// @param dlls Array of paths to the dynamic-link libraries
/// @return True if the process was launched and the libraries were injected
///         successfully, false otherwise
export bool LaunchAndInject (ProcessPaths process_paths,
                             DWORD dlls_count,
                             LPCSTR* dlls)
{
    STARTUPINFOA startup_info = {};
    startup_info.cb = sizeof (startup_info);
    PROCESS_INFORMATION process_info = {};

    bool result = DetourCreateProcessWithDllsA (process_paths.app_path.data (),
                                                process_paths.arguments.data (),
                                                nullptr,
                                                nullptr,
                                                TRUE,
                                                CREATE_SUSPENDED,
                                                nullptr,
                                                process_paths.app_directory.data (),
                                                &startup_info,
                                                &process_info,
                                                dlls_count,
                                                dlls,
                                                nullptr);

    if (result == false)
    {
        return false;
    }

    // At this point, injected dynamic-link libraries have run their `DllMain`.
    // Resume the main thread of the process to start execution.
    ResumeThread (process_info.hThread);
    return true;
}

// ----------------------------------------------------------------------------

/// @brief Initialize Detours library
/// 
/// Restore the contents in memory import table to its orignal state after
/// a process has been started with Detours. Ensure the restoration happens
/// only once.
static void InitializeDetours ()
{
    static bool initialized = [] ()
        {
            DetourRestoreAfterWith ();
            return true;
        }();
}

// ----------------------------------------------------------------------------

/// @brief Hook an API call using Detours
/// 
/// @param real_fn Pointer to the original function
/// @param hook_fn Pointer to the hook function
/// @return True if the hook was successful, false otherwise
export bool WINAPI HookAPICall (PVOID* real_fn, PVOID hook_fn)
{
    InitializeDetours ();

    DetourTransactionBegin ();
    DetourUpdateThread (GetCurrentThread ());

    LONG error = DetourAttach (real_fn, hook_fn);
    if (error != NO_ERROR)
    {
        DetourTransactionAbort ();
        return false;
    }

    error = DetourTransactionCommit ();
    return error == NO_ERROR;
}

// ----------------------------------------------------------------------------

/// @brief Unhook an API call using Detours
/// 
/// @param real_fn Pointer to the original function
/// @param hook_fn Pointer to the hook function
/// @return True if the unhook was successful, false otherwise
export bool WINAPI UnhookAPICall (PVOID* real_fn, PVOID hook_fn)
{
    DetourTransactionBegin ();
    DetourUpdateThread (GetCurrentThread ());

    LONG error = DetourDetach (real_fn, hook_fn);
    if (error != NO_ERROR)
    {
        DetourTransactionAbort ();
        return false;
    }

    error = DetourTransactionCommit ();
    return error == NO_ERROR;
}

}

// ----------------------------------------------------------------------------