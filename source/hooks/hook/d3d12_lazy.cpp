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
        ID3D12GraphicsCommandList* command_list = reinterpret_cast<ID3D12GraphicsCommandList*>(*ppCommandList);

        if (Original_ID3D12GraphicsCommandList::ResourceBarrier == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (command_list);
            Original_ID3D12GraphicsCommandList::ResourceBarrier = reinterpret_cast<Original_ID3D12GraphicsCommandList::pfn_ResourceBarrier> (
                vtable[static_cast<int>(ID3D12GraphicsCommandList_VTable_ID::ResourceBarrier)]);

            bool result = HookWrap (Original_ID3D12GraphicsCommandList::ResourceBarrier, ResourceBarrier,
                                    "ID3D12GraphicsCommandList::ResourceBarrier"sv);
            if (result == false)
            {
                return false;
            }
        }

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

        ID3D12GraphicsCommandList7* command_list7 = nullptr;
        if (FAILED (reinterpret_cast<ID3D12GraphicsCommandList*>(*ppCommandList)->QueryInterface (IID_PPV_ARGS (&command_list7))))
        {
            return false;
        }

        if (Original_ID3D12GraphicsCommandList::Barrier == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (command_list7);
            Original_ID3D12GraphicsCommandList::Barrier = reinterpret_cast<Original_ID3D12GraphicsCommandList::pfn_Barrier> (
                vtable[static_cast<int>(ID3D12GraphicsCommandList7_VTable_ID::Barrier)]);
            bool result = HookWrap (Original_ID3D12GraphicsCommandList::Barrier, Barrier,
                                    "ID3D12GraphicsCommandList::Barrier"sv);
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

        if (Original_ID3D12Device::CreateCommittedResource == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device);
            Original_ID3D12Device::CreateCommittedResource = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommittedResource> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreateCommittedResource)]);

            bool result = HookWrap (Original_ID3D12Device::CreateCommittedResource, CreateCommittedResource,
                                    "ID3D12Device::CreateCommittedResource"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_ID3D12Device::CreatePlacedResource == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device);
            Original_ID3D12Device::CreatePlacedResource = reinterpret_cast<Original_ID3D12Device::pfn_CreatePlacedResource> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreatePlacedResource)]);

            bool result = HookWrap (Original_ID3D12Device::CreatePlacedResource, CreatePlacedResource,
                                    "ID3D12Device::CreatePlacedResource"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_ID3D12Device::CreateReservedResource == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device);
            Original_ID3D12Device::CreateReservedResource = reinterpret_cast<Original_ID3D12Device::pfn_CreateReservedResource> (
                vtable[static_cast<int>(ID3D12Device_VTable_ID::CreateReservedResource)]);

            bool result = HookWrap (Original_ID3D12Device::CreateReservedResource, CreateReservedResource,
                                    "ID3D12Device::CreateReservedResource"sv);
            if (result == false)
            {
                return false;
            }
        }

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

        if (Original_ID3D12Device::CreateCommittedResource1 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device4);
            Original_ID3D12Device::CreateCommittedResource1 = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommittedResource1> (
                vtable[static_cast<int>(ID3D12Device4_VTable_ID::CreateCommittedResource1)]);

            bool result = HookWrap (Original_ID3D12Device::CreateCommittedResource1, CreateCommittedResource1,
                                    "ID3D12Device4::CreateCommittedResource1"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_ID3D12Device::CreateReservedResource1 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device4);
            Original_ID3D12Device::CreateReservedResource1 = reinterpret_cast<Original_ID3D12Device::pfn_CreateReservedResource1> (
                vtable[static_cast<int>(ID3D12Device4_VTable_ID::CreateReservedResource1)]);

            bool result = HookWrap (Original_ID3D12Device::CreateReservedResource1, CreateReservedResource1,
                                    "ID3D12Device4::CreateReservedResource1"sv);
            if (result == false)
            {
                return false;
            }
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

        ID3D12Device8* device8 = nullptr;
        if (FAILED (reinterpret_cast<ID3D12Device*>(*ppDevice)->QueryInterface (IID_PPV_ARGS (&device8))))
        {
            return false;
        }

        if (Original_ID3D12Device::CreateCommittedResource2 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device8);
            Original_ID3D12Device::CreateCommittedResource2 = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommittedResource2> (
                vtable[static_cast<int>(ID3D12Device8_VTable_ID::CreateCommittedResource2)]);

            bool result = HookWrap (Original_ID3D12Device::CreateCommittedResource2, CreateCommittedResource2,
                                    "ID3D12Device8::CreateCommittedResource2"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_ID3D12Device::CreatePlacedResource1 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device8);
            Original_ID3D12Device::CreatePlacedResource1 = reinterpret_cast<Original_ID3D12Device::pfn_CreatePlacedResource1> (
                vtable[static_cast<int>(ID3D12Device8_VTable_ID::CreatePlacedResource1)]);

            bool result = HookWrap (Original_ID3D12Device::CreatePlacedResource1, CreatePlacedResource1,
                                    "ID3D12Device8::CreatePlacedResource1"sv);
            if (result == false)
            {
                return false;
            }
        }

        ID3D12Device10* device10 = nullptr;
        if (FAILED (reinterpret_cast<ID3D12Device*>(*ppDevice)->QueryInterface (IID_PPV_ARGS (&device10))))
        {
            return false;
        }

        if (Original_ID3D12Device::CreateCommittedResource3 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device10);
            Original_ID3D12Device::CreateCommittedResource3 = reinterpret_cast<Original_ID3D12Device::pfn_CreateCommittedResource3> (
                vtable[static_cast<int>(ID3D12Device10_VTable_ID::CreateCommittedResource3)]);

            bool result = HookWrap (Original_ID3D12Device::CreateCommittedResource3, CreateCommittedResource3,
                                    "ID3D12Device10::CreateCommittedResource3"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_ID3D12Device::CreatePlacedResource2 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device10);
            Original_ID3D12Device::CreatePlacedResource2 = reinterpret_cast<Original_ID3D12Device::pfn_CreatePlacedResource2> (
                vtable[static_cast<int>(ID3D12Device10_VTable_ID::CreatePlacedResource2)]);

            bool result = HookWrap (Original_ID3D12Device::CreatePlacedResource2, CreatePlacedResource2,
                                    "ID3D12Device10::CreatePlacedResource2"sv);
            if (result == false)
            {
                return false;
            }
        }

        if (Original_ID3D12Device::CreateReservedResource2 == nullptr)
        {
            void** vtable = *reinterpret_cast<void***> (device10);
            Original_ID3D12Device::CreateReservedResource2 = reinterpret_cast<Original_ID3D12Device::pfn_CreateReservedResource2> (
                vtable[static_cast<int>(ID3D12Device10_VTable_ID::CreateReservedResource2)]);

            bool result = HookWrap (Original_ID3D12Device::CreateReservedResource2, CreateReservedResource2,
                                    "ID3D12Device10::CreateReservedResource2"sv);
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