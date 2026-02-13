// ----------------------------------------------------------------------------

/// @brief Entry point of dynamic-link library for Windows API hooks

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

import std;
import RayBench.Util;

// Ensure the DLL has an exported function to avoid linker errors
extern "C" __declspec(dllexport) void DummyDLLFunction ()
{}

BOOL APIENTRY DllMain (HMODULE /*hModule*/,
                       DWORD  ul_reason_for_call,
                       LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            raybench::util::Log::ClientConnect ();
            RAYBENCH_LOG_DEBUG ("DLL attached to process/thread");
            break;
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
            raybench::util::Log::ClientDisconnect ();
            break;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------