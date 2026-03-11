// ============================================================================

/// @brief Entry point of dynamic-link library for D3D12 API hooks

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

extern "C" __declspec(dllexport) bool Hook ()
{
    return true;
}

BOOL APIENTRY DllMain (HMODULE /*hModule*/,
                       DWORD  ul_reason_for_call,
                       LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
        case DLL_THREAD_ATTACH:
            break;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------