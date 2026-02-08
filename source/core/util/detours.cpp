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

namespace raybench::util
{

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
    InitializeDetours ();

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