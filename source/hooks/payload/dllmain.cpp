// ============================================================================

/// @brief Entry point of dynamic-link library payload for API hooks

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "util/log.h"

import std;
import RayBench.Payload;
import RayBench.Util;

extern "C" __declspec(dllexport) bool Hook ()
{
    return true;
}

// ============================================================================

/// @brief Entry point of the payload DLL
BOOL APIENTRY DllMain (HMODULE /*hModule*/,
                       DWORD  ul_reason_for_call,
                       LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            auto log_settings = raybench::util::EnvVar::Get (raybench::util::EnvVar::log_settings);
            if (log_settings.has_value ())
            {
                raybench::util::Log::GetSettings ().Deserialize (log_settings.value ());
            }
            raybench::util::Log::ClientConnect ();

            raybench::payload::HookCreateProcess ();
            raybench::payload::HookLoadLibrary ();

            RAYBENCH_LOG_DEBUG ("Ready to inject hooks from ray-bench-payload.dll");
        }
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
        case DLL_THREAD_ATTACH:
            break;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------