// ----------------------------------------------------------------------------

/// @brief Entry point of dynamic-link library for Windows API hooks

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <detours/detours.h>

import RayBench.Util;

// Ensure the DLL has an exported function to avoid linker errors
extern "C" __declspec(dllexport) void DummyDLLFunction ()
{}

BOOL APIENTRY DllMain (HMODULE /*hModule*/,
                       DWORD  ul_reason_for_call,
                       LPVOID /*lpReserved*/)
{
    // Detours uses a helper process if injecting into an application with
    // different bitness. If this is the helper, do not attempt to hook.
    if (DetourIsHelperProcess ())
    {
        return TRUE;
    }

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------