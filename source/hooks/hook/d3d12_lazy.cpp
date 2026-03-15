// ============================================================================

/// @brief D3D12 lazy hook implementation

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>

#include "d3d12_vtables.hpp"
#include "util/log.h"

export module RayBench.Hook:D3D12.Lazy;

import :D3D12.Hook;
import :D3D12.Pfn;

import std;
import RayBench.Util;

using namespace std::literals;
using raybench::util::HookWrap;
using raybench::util::UnhookWrap;

namespace raybench::hook
{

/// @brief Hook `ID3D12GraphicsCommandList` API calls
///
/// @param ppCommandList `ID3D12GraphicsCommandList`
/// @return True if successful, false otherwise
bool Hooked_ID3D12GraphicsCommandList::LazyHook (void** ppCommandList)
{
    static bool is_hooked = false;

    if (is_hooked == false && ppCommandList != nullptr && *ppCommandList != nullptr)
    {
        if (Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure == nullptr)
        {
            ID3D12GraphicsCommandList4* command_list4 = nullptr;
            if (FAILED (reinterpret_cast<ID3D12GraphicsCommandList*>(*ppCommandList)->QueryInterface (IID_PPV_ARGS (&command_list4))))
            {
                return false;
            }
            void** vtable = *reinterpret_cast<void***> (command_list4);
            Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure =
                reinterpret_cast<Original_ID3D12GraphicsCommandList::pfn_BuildRaytracingAccelerationStructure> (
                    vtable[static_cast<int>(ID3D12GraphicsCommandList4_VTable_ID::BuildRaytracingAccelerationStructure)]);

            HookWrap (Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure,
                      BuildRaytracingAccelerationStructure,
                      "ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure"sv);
        }

        is_hooked = true;
    }
    return true;
}

// ----------------------------------------------------------------------------

/// @brief Hook `ID3D12Device` API calls
///
/// @param ppDevice `ID3D12Device`
/// @return True if successful, false otherwise
bool Hooked_ID3D12Device::LazyHook (void** ppDevice)
{
    static bool is_hooked = false;

    if (is_hooked == false && ppDevice != nullptr && *ppDevice != nullptr)
    {
        if (Original_ID3D12Device::CreateCommandList == nullptr)
        {
            ID3D12Device* device = reinterpret_cast<ID3D12Device*>(*ppDevice);
            void** vtable = *reinterpret_cast<void***> (device);

            Original_ID3D12Device::CreateCommandList = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandList> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreateCommandList)]);

            HookWrap (Original_ID3D12Device::CreateCommandList, CreateCommandList,
                      "ID3D12Device::CreateCommandList"sv);
        }

        if (Original_ID3D12Device::CreateCommandList1 == nullptr)
        {
            ID3D12Device4* device4 = nullptr;
            if (FAILED (reinterpret_cast<ID3D12Device*>(*ppDevice)->QueryInterface (IID_PPV_ARGS (&device4))))
            {
                return false;
            }
            void** vtable = *reinterpret_cast<void***> (device4);

            Original_ID3D12Device::CreateCommandList1 = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandList1> (
                vtable[static_cast<int>(ID3D12Device4_VTable_ID::CreateCommandList1)]);

            HookWrap (Original_ID3D12Device::CreateCommandList1, CreateCommandList1,
                      "ID3D12Device4::CreateCommandList1"sv);
        }

        is_hooked = true;
    }

    return true;
}

}

// ----------------------------------------------------------------------------