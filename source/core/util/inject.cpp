// ----------------------------------------------------------------------------

/// @brief Utility for dynamic-link library injection

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "log.h"

export module RayBench.Util:Inject;

import std;
import :Log;

namespace raybench::util
{

///< Target memory address for injected dynamic-link library to be loaded at
static void* target_memory_address = nullptr;

// ----------------------------------------------------------------------------

/// @brief Information related to target process to launch and inject into
struct CreateProcessInfo
{
    ///< Path to the application executable
    std::filesystem::path app_path;
    ///< Command line arguments for the application
    std::string arguments;
    ///< Working directory for the application
    std::filesystem::path app_directory;
};

// ----------------------------------------------------------------------------

/// @brief Parse the full command line of a process to extract its information
/// 
/// @param full_command Full command line of the process, including the
///                     executable path and its arguments
/// @return Optional containing the target process information if successful
export [[nodiscard]] std::optional<CreateProcessInfo> GetCreateProcessInfo (std::string_view full_command)
{
    using namespace std::string_view_literals;

    const auto exe_pos = full_command.find (".exe"sv);
    if (exe_pos == std::string_view::npos)
    {
        return std::nullopt;
    }

    const size_t args_start = exe_pos + 4;
    std::filesystem::path path_view {full_command.substr (0, args_start)};

    return CreateProcessInfo {
        .app_path = path_view,
        .arguments = std::format ("\"{}\"{}", path_view.string (), full_command.substr (args_start)),
        .app_directory = path_view.parent_path ()
    };
}

// ----------------------------------------------------------------------------

/// @brief Inject a dynamic-link library into the target process
/// 
/// @param process_handle Handle to the target process to inject into
/// @param dll Path to the dynamic-link library to inject
/// @return True if the injection was successful, false otherwise
bool InjectDLL (HANDLE process_handle, LPCSTR dll)
{
    size_t size_of_memory = (strlen (dll) + 1) * sizeof (char);
    if (size_of_memory == 0)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to calculate size of memory to allocate for dynamic-link library path: {}",
                               GetLastError ());
        return false;
    }

    target_memory_address = VirtualAllocEx (process_handle,
                                            nullptr,
                                            size_of_memory,
                                            MEM_COMMIT | MEM_RESERVE,
                                            PAGE_EXECUTE_READWRITE);
    if (target_memory_address == nullptr)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to allocate memory in target: {}",
                               GetLastError ());
        return false;
    }

    size_t bytes_written = 0;
    bool result = WriteProcessMemory (process_handle,
                                      target_memory_address,
                                      dll,
                                      size_of_memory,
                                      &bytes_written);
    if (result == false || bytes_written != size_of_memory)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to write dynamic-link path to target process memory: {}",
                               GetLastError ());
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

/// @brief Create a remote thread in the target process to load the injected
///        dynamic-link library
/// @param process_handle Handle to the target process to create the remote
///                       thread in
/// @return True if the remote thread was successfully created and executed,
///         false otherwise
bool LoadDLL (HANDLE process_handle)
{
    HMODULE handle = GetModuleHandleA ("kernel32.dll");
    if (handle == nullptr)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to get handle for kernel32.dll: {}",
                               GetLastError ());
        return false;
    }

    FARPROC load_library_address = GetProcAddress (handle, "LoadLibraryA");
    if (load_library_address == nullptr)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to get address for 'LoadLibraryA': {}",
                               GetLastError ());
        return false;
    }

    DWORD thread_id;
    HANDLE thread_handle = CreateRemoteThread (process_handle,
                                               nullptr,
                                               0,
                                               reinterpret_cast<LPTHREAD_START_ROUTINE> (load_library_address),
                                               target_memory_address,
                                               0,
                                               &thread_id);
    if (thread_handle == nullptr)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to create remote thread in target: {}",
                               GetLastError ());
        return false;
    }

    WaitForSingleObject (thread_handle, INFINITE);

    DWORD exit_code;
    bool result = GetExitCodeThread (thread_handle, &exit_code);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to get exit code of remote thread in target: {}",
                               GetLastError ());
        return false;
    }

    if (exit_code == 0)
    {
        RAYBENCH_LOG_CRITICAL ("Remote thread failed to load dynamic-link library into target");
        return false;
    }

    CloseHandle (thread_handle);
    return true;
}

// ----------------------------------------------------------------------------

/// @brief Inject a dynamic-link library into the target process and create
///        a remote thread to load it
/// @param process_handle Handle to the target process to inject into and
///                       create the remote thread in
/// @param dll Path to the dynamic-link library to inject and load into the
///            target
/// @return True if the dynamic-link library was successfully injected and
///         loaded into
bool InjectLoadDLL (HANDLE process_handle, LPCSTR dll)
{
    RAYBENCH_LOG_TRACE ("Injecting dynamic-link library into target...");

    bool result = InjectDLL (process_handle, dll);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to inject dynamic-link library into target: {}",
                               GetLastError ());
        return false;
    }

    RAYBENCH_LOG_TRACE ("Loading dynamic-link library into target...");

    result = LoadDLL (process_handle);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to load dynamic-link library into process: {}",
                               GetLastError ());
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

/// @brief Launch a process and inject dynamic-link library into it
export BOOL LaunchInjectA (LPCSTR lpApplicationName,
                           LPSTR lpCommandLine,
                           LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           LPSECURITY_ATTRIBUTES lpThreadAttributes,
                           BOOL bInheritHandles,
                           DWORD dwCreationFlags,
                           LPVOID lpEnvironment,
                           LPCSTR lpCurrentDirectory,
                           LPSTARTUPINFOA lpStartupInfo,
                           LPPROCESS_INFORMATION lpProcessInformation,
                           LPCSTR dll)
{
    BOOL result = CreateProcessA (lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                  lpProcessInformation);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to launch suspended target: {}",
                               GetLastError ());
        return false;
    }

    RAYBENCH_LOG_DEBUG ("Launched suspended target: process ID {}", lpProcessInformation->dwProcessId);

    if (InjectLoadDLL (lpProcessInformation->hProcess, dll) == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to inject and load dynamic-link library into target: {}",
                               GetLastError ());
        TerminateProcess (lpProcessInformation->hProcess, 1);
        CloseHandle (lpProcessInformation->hThread);
        CloseHandle (lpProcessInformation->hProcess);
        return false;
    }
    ResumeThread (lpProcessInformation->hThread);

    RAYBENCH_LOG_DEBUG ("Injected and loaded dynamic-link library into target and resumed: process ID {}", lpProcessInformation->dwProcessId);

    return result;
}

/// @brief Launch a process and inject dynamic-link library into it
export BOOL LaunchInjectW (LPCWSTR lpApplicationName,
                           LPWSTR lpCommandLine,
                           LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           LPSECURITY_ATTRIBUTES lpThreadAttributes,
                           BOOL bInheritHandles,
                           DWORD dwCreationFlags,
                           LPVOID lpEnvironment,
                           LPCWSTR lpCurrentDirectory,
                           LPSTARTUPINFOW lpStartupInfo,
                           LPPROCESS_INFORMATION lpProcessInformation,
                           LPCSTR dll)
{
    BOOL result = CreateProcessW (lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                  lpProcessInformation);
    if (result == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to launch suspended target: {}",
                               GetLastError ());
        return false;
    }

    RAYBENCH_LOG_DEBUG ("Launched suspended target: process ID {}", lpProcessInformation->dwProcessId);

    if (InjectLoadDLL (lpProcessInformation->hProcess, dll) == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to inject and load dynamic-link library into target: {}",
                               GetLastError ());
        TerminateProcess (lpProcessInformation->hProcess, 1);
        CloseHandle (lpProcessInformation->hThread);
        CloseHandle (lpProcessInformation->hProcess);
        return false;
    }
    ResumeThread (lpProcessInformation->hThread);

    RAYBENCH_LOG_DEBUG ("Injected and loaded dynamic-link library into target and resumed: process ID {}", lpProcessInformation->dwProcessId);

    return result;
}

/// @brief Launch a process and inject dynamic-link library into it
/// 
/// @param create_process_info Information related to the target process
///                            to launch and inject into
/// @param dll Path to the dynamic-link library to inject
export BOOL LaunchInject (CreateProcessInfo& create_process_info, LPCSTR dll)
{
    STARTUPINFOA startup_info {};
    startup_info.cb = sizeof (startup_info);
    PROCESS_INFORMATION process_info {};

    return LaunchInjectA (create_process_info.app_path.string ().c_str (),
                          const_cast<LPSTR>(create_process_info.arguments.data ()),
                          nullptr,
                          nullptr,
                          TRUE,
                          CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED,
                          nullptr,
                          create_process_info.app_directory.string ().c_str (),
                          &startup_info,
                          &process_info,
                          dll);
}

}

// ----------------------------------------------------------------------------