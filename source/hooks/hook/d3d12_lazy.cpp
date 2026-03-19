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

/// @brief Hook `ID3D12Resource` API calls
///
/// @param ppDevice `ID3D12Device`
/// @return True if successful, false otherwise
bool Hooked_ID3D12Resource::LazyHook (void** ppDevice)
{
    if (ppDevice != nullptr && *ppDevice != nullptr)
    {
        ID3D12Resource* resource = nullptr;
        ID3D12Device* device = reinterpret_cast<ID3D12Device*>(*ppDevice);

        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = 1;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource (&heap_properties,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &resource_desc,
                                                      D3D12_RESOURCE_STATE_COMMON,
                                                      nullptr,
                                                      IID_PPV_ARGS (&resource));
        if (FAILED (hr))
        {
            return false;
        }

        if (Original_ID3D12Resource::GetGPUVirtualAddress == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (resource);
            Original_ID3D12Resource::GetGPUVirtualAddress = reinterpret_cast<Original_ID3D12Resource::pfn_GetGPUVirtualAddress> (
                vtable[static_cast<int>(ID3D12Resource_VTable_ID::GetGPUVirtualAddress)]);

            bool result = HookWrap (Original_ID3D12Resource::GetGPUVirtualAddress, GetGPUVirtualAddress,
                                    "ID3D12Resource::GetGPUVirtualAddress"sv);
            resource->Release ();
            if (result == false)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------

/// @brief Hook `ID3D12GraphicsCommandList` API calls
///
/// @param ppCommandList `ID3D12GraphicsCommandList`
/// @return True if successful, false otherwise
bool Hooked_ID3D12GraphicsCommandList::LazyHook (void** ppCommandList)
{
    if (ppCommandList != nullptr && *ppCommandList != nullptr)
    {
        ID3D12GraphicsCommandList4* command_list4 = nullptr;
        if (FAILED (reinterpret_cast<ID3D12GraphicsCommandList*>(*ppCommandList)->QueryInterface (IID_PPV_ARGS (&command_list4))))
        {
            return false;
        }

        if (Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (command_list4);
            Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure =
                reinterpret_cast<Original_ID3D12GraphicsCommandList::pfn_BuildRaytracingAccelerationStructure> (
                    vtable[static_cast<int>(ID3D12GraphicsCommandList4_VTable_ID::BuildRaytracingAccelerationStructure)]);

            bool result = HookWrap (Original_ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure,
                                    BuildRaytracingAccelerationStructure,
                                    "ID3D12GraphicsCommandList::BuildRaytracingAccelerationStructure"sv);
            if (result == false)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
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
    if (ppDevice != nullptr && *ppDevice != nullptr)
    {
        ID3D12Device* device = reinterpret_cast<ID3D12Device*>(*ppDevice);

        if (Original_ID3D12Device::CreateCommandList == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device);

            Original_ID3D12Device::CreateCommandList = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandList> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreateCommandList)]);

            bool result = HookWrap (Original_ID3D12Device::CreateCommandList, CreateCommandList,
                                    "ID3D12Device::CreateCommandList"sv);
            if (result == false)
            {
                return false;
            }
        }

        ID3D12Device4* device4 = nullptr;
        if (FAILED (reinterpret_cast<ID3D12Device*>(*ppDevice)->QueryInterface (IID_PPV_ARGS (&device4))))
        {
            return false;
        }

        if (Original_ID3D12Device::CreateCommandList1 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device4);

            Original_ID3D12Device::CreateCommandList1 = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommandList1> (
                vtable[static_cast<int>(ID3D12Device4_VTable_ID::CreateCommandList1)]);

            bool result = HookWrap (Original_ID3D12Device::CreateCommandList1, CreateCommandList1,
                                    "ID3D12Device4::CreateCommandList1"sv);
            if (result == false)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

}

// ----------------------------------------------------------------------------