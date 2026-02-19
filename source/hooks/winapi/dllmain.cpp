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
        {
            auto log_settings = raybench::util::EnvVar::Get (raybench::util::EnvVar::log_settings);
            if (log_settings.has_value ())
            {
                raybench::util::Log::GetSettings ().Deserialize (log_settings.value ());
            }
            raybench::util::Log::ClientConnect ();
            RAYBENCH_LOG_INFO ("Loaded Windows API hooks dynamic-link library");
        }
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
        {
            RAYBENCH_LOG_TRACE ("Unloading Windows API hooks dynamic-link library...");
            raybench::util::Log::ClientDisconnect ();
        }
        break;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------