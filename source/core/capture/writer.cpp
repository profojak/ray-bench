// ============================================================================

/// @brief State writer

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>
#include <nvapi/nvapi.h>

#include "util/log.h"

export module RayBench.Capture:Writer;

import std;
import :Tracker;
import RayBench.Util;

namespace raybench::capture
{

/// @brief State writer
export class Writer
{
public:

    void WriteCapture (Tracker& tracker, IDXGISwapChain* This)
    {
        RAYBENCH_LOG_INFO ("{}", tracker.as_tracker_.build_info_map_.size ());

        ID3D12CommandQueue* command_queue = nullptr;
        {
            std::scoped_lock lock (tracker.state_mutex_);
            auto it = tracker.swap_chain_command_queue_map_.find (This);
            if (it == tracker.swap_chain_command_queue_map_.end ())
            {
                RAYBENCH_LOG_ERROR ("Failed to find command queue for swap chain: 0x{:016X}!", reinterpret_cast<std::uintptr_t>(This));
                return;
            }
            command_queue = it->second;
        }

        ID3D12Device* device = nullptr;
        if (FAILED (command_queue->GetDevice (IID_PPV_ARGS (&device))))
        {
            RAYBENCH_LOG_ERROR ("Failed to get device from command queue!");
            return;
        }

        D3D12_COMMAND_QUEUE_DESC queue_desc = command_queue->GetDesc ();

        ID3D12CommandAllocator* command_allocator = nullptr;
        if (FAILED (device->CreateCommandAllocator (queue_desc.Type, IID_PPV_ARGS (&command_allocator))))
        {
            RAYBENCH_LOG_ERROR ("Failed to create command allocator!");
            device->Release ();
            return;
        }

        ID3D12GraphicsCommandList* command_list = nullptr;
        if (FAILED (device->CreateCommandList (0, queue_desc.Type, command_allocator, nullptr, IID_PPV_ARGS (&command_list))))
        {
            RAYBENCH_LOG_ERROR ("Failed to create command list!");
            command_allocator->Release ();
            device->Release ();
            return;
        }

        std::vector<std::pair<ID3D12Resource*, AccelerationStructureTracker::BuildInfo>> readback_buffers;
        for (const auto& [as_addr, build_info] : tracker.as_tracker_.build_info_map_)
        {
            RAYBENCH_ASSERT (build_info.copyback_size != 0, "Invalid acceleration structure staging buffer size!");
            RAYBENCH_ASSERT (build_info.copyback_resource != nullptr, "Invalid acceleration structure staging buffer resource!");

            ID3D12Resource* readback_buffer = nullptr;

            D3D12_HEAP_PROPERTIES readback_heap_props {};
            readback_heap_props.Type = D3D12_HEAP_TYPE_READBACK;
            readback_heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            readback_heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            readback_heap_props.CreationNodeMask = 1;
            readback_heap_props.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC readback_desc {};
            readback_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            readback_desc.Alignment = 0;
            readback_desc.Width = build_info.copyback_size;
            readback_desc.Height = 1;
            readback_desc.DepthOrArraySize = 1;
            readback_desc.MipLevels = 1;
            readback_desc.Format = DXGI_FORMAT_UNKNOWN;
            readback_desc.SampleDesc.Count = 1;
            readback_desc.SampleDesc.Quality = 0;
            readback_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            readback_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            HRESULT hr = device->CreateCommittedResource (&readback_heap_props,
                                                          D3D12_HEAP_FLAG_NONE,
                                                          &readback_desc,
                                                          D3D12_RESOURCE_STATE_COPY_DEST,
                                                          nullptr,
                                                          IID_PPV_ARGS (&readback_buffer));
            if (FAILED (hr))
            {
                RAYBENCH_LOG_ERROR ("Failed to create readback buffer: 0x{:08X}!", hr);
                continue;
            }

            readback_buffers.push_back (std::make_pair(readback_buffer, build_info));

            D3D12_RESOURCE_BARRIER pre_barrier {};
            pre_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            pre_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            pre_barrier.Transition.pResource = build_info.copyback_resource;
            pre_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            pre_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            pre_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            command_list->ResourceBarrier (1, &pre_barrier);

            command_list->CopyResource (readback_buffer, build_info.copyback_resource);

            D3D12_RESOURCE_BARRIER post_barrier {};
            post_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            post_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            post_barrier.Transition.pResource = build_info.copyback_resource;
            post_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            post_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            post_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            command_list->ResourceBarrier (1, &post_barrier);
        }

        if (SUCCEEDED (command_list->Close ()))
        {
            ID3D12CommandList* ppCommandLists[] = { command_list };
            command_queue->ExecuteCommandLists (1, ppCommandLists);

            ID3D12Fence* fence = nullptr;
            if (SUCCEEDED (device->CreateFence (0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (&fence))))
            {
                HANDLE event = CreateEvent (nullptr, FALSE, FALSE, nullptr);
                if (event)
                {
                    command_queue->Signal (fence, 1);
                    if (SUCCEEDED (fence->SetEventOnCompletion (1, event)))
                    {
                        WaitForSingleObject (event, INFINITE);
                    }
                    CloseHandle (event);
                }
                fence->Release ();
            }
        }

        for (auto [rb, build_info] : readback_buffers)
        {
            void* mapped_ptr = nullptr;
            if (SUCCEEDED (rb->Map (0, nullptr, &mapped_ptr)))
            {
                const uint8_t* data = static_cast<const uint8_t*> (mapped_ptr);
                size_t size = static_cast<size_t> (rb->GetDesc ().Width);
                bool has_data = false;
                for (size_t i = 0; i < std::min (size, size_t (1024)); ++i)
                {
                    if (data[i] != 0)
                    {
                        has_data = true;
                        break;
                    }
                }

                if (has_data)
                {
                    RAYBENCH_LOG_DEBUG ("Readback buffer 0x{:016X} contains data!", reinterpret_cast<uintptr_t> (rb));
                }
                else
                {
                    RAYBENCH_LOG_DEBUG ("Readback buffer 0x{:016X} is empty (all zeros in first 1KB).", reinterpret_cast<uintptr_t> (rb));
                }

                rb->Unmap (0, nullptr);
            }

            rb->Release ();
        }

        command_list->Release ();
        command_allocator->Release ();
        device->Release ();
    }
};

}

// ----------------------------------------------------------------------------