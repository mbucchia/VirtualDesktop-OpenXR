// MIT License
//
// Copyright(c) 2024 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

namespace D3D12Utils {

    class CommandList {
      public:
        ComPtr<ID3D12GraphicsCommandList> Commands;

      private:
        ComPtr<ID3D12CommandAllocator> Allocator;
        uint64_t CompletedFenceValue{0};

        friend class CommandContext;
    };

    class CommandContext {
      public:
        CommandContext(ID3D12Device* Device, const std::wstring& DebugName = L"Unnamed")
            : m_Device(Device), m_DebugName(DebugName) {
            // Create a command queue for our commands.
            D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
            commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            CHECK_HRCMD(
                m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(m_CommandQueue.ReleaseAndGetAddressOf())));
            m_CommandQueue->SetName((DebugName + L" Command Queue").c_str());

            CHECK_HRCMD(m_Device->CreateFence(
                0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_CompletionFence.ReleaseAndGetAddressOf())));
            m_CompletionFence->SetName((DebugName + L" Completion Fence").c_str());
        }

        ~CommandContext() {
            Flush();
        }

        CommandList GetCommandList() {
            std::unique_lock lock(m_CommandListPoolMutex);

            if (m_AvailableCommandList.empty()) {
                // Recycle completed command lists.
                while (!m_PendingCommandList.empty() &&
                       IsCommandListCompleted(m_PendingCommandList.front().CompletedFenceValue)) {
                    m_AvailableCommandList.push_back(std::move(m_PendingCommandList.front()));
                    m_PendingCommandList.pop_front();
                }
            }

            CommandList commandList;
            if (m_AvailableCommandList.empty()) {
                // Allocate a new command list if needed.
                CHECK_HRCMD(m_Device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandList.Allocator.ReleaseAndGetAddressOf())));
                CHECK_HRCMD(m_Device->CreateCommandList(0,
                                                        D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                        commandList.Allocator.Get(),
                                                        nullptr,
                                                        IID_PPV_ARGS(commandList.Commands.ReleaseAndGetAddressOf())));
                commandList.Commands->SetName(L"Shading Rate Creation Command List");
            } else {
                commandList = m_AvailableCommandList.front();
                m_AvailableCommandList.pop_front();

                // Reset the command list before reuse.
                CHECK_HRCMD(commandList.Allocator->Reset());
                CHECK_HRCMD(commandList.Commands->Reset(commandList.Allocator.Get(), nullptr));
            }

            return commandList;
        }

        uint64_t SubmitCommandList(CommandList CommandList) {
            std::unique_lock lock(m_CommandListPoolMutex);

            CHECK_HRCMD(CommandList.Commands->Close());
            // TODO: This call goes through the Detoured version, which causes unwanted tracing.
            m_CommandQueue->ExecuteCommandLists(
                1, reinterpret_cast<ID3D12CommandList**>(CommandList.Commands.GetAddressOf()));
            CommandList.CompletedFenceValue = ++m_CompletionFenceValue;
            m_CommandQueue->Signal(m_CompletionFence.Get(), CommandList.CompletedFenceValue);
            m_PendingCommandList.push_back(std::move(CommandList));

            return CommandList.CompletedFenceValue;
        }

        void Flush() {
            if (m_CompletionFenceValue) {
                wil::unique_handle handle;
                *handle.put() = CreateEventEx(nullptr, L"Destruction Fence", 0, EVENT_ALL_ACCESS);
                CHECK_HRCMD(m_CompletionFence->SetEventOnCompletion(m_CompletionFenceValue, handle.get()));
                WaitForSingleObject(handle.get(), INFINITE);
            }
        }

        bool IsCommandListCompleted(uint64_t CompletedFenceValue) {
            return m_CompletionFence->GetCompletedValue() >= CompletedFenceValue;
        }

        ID3D12Fence* GetCompletionFence() const {
            return m_CompletionFence.Get();
        }

        ID3D12CommandQueue* GetCommandQueue() const {
            return m_CommandQueue.Get();
        }

      private:
        ComPtr<ID3D12Device> m_Device;
        ComPtr<ID3D12CommandQueue> m_CommandQueue;

        std::mutex m_CommandListPoolMutex;
        std::deque<CommandList> m_AvailableCommandList;
        std::deque<CommandList> m_PendingCommandList;
        ComPtr<ID3D12Fence> m_CompletionFence;
        uint64_t m_CompletionFenceValue{0};

        const std::wstring m_DebugName;
    };

    class DescriptorHeap {
      public:
        DescriptorHeap(ID3D12Device* Device,
                       D3D12_DESCRIPTOR_HEAP_TYPE HeapType,
                       UINT NumDescriptors = 128u,
                       const std::wstring& DebugName = L"Unnamed")
            : m_Device(Device) {
            // Create the descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
            descriptorHeapDesc.Type = HeapType;
            descriptorHeapDesc.NumDescriptors = NumDescriptors;
            if (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER || HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
                descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            }
            CHECK_HRCMD(
                m_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));
            m_Heap->SetName((DebugName + L" Descriptor Heap").c_str());

            // Retrieve the heap base.
            m_CPUHandleStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
            m_GPUHandleStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
            m_DescriptorSize = m_Device->GetDescriptorHandleIncrementSize(HeapType);

            // Populate the allocator.
            for (UINT i = 0; i < descriptorHeapDesc.NumDescriptors; i++) {
                m_AvailableDescriptor.push_back(i);
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor() {
            std::unique_lock lock(m_AvailableDescriptorMutex);

            CHECK_MSG(!m_AvailableDescriptor.empty(), "Out of descriptors");

            const UINT allocated = m_AvailableDescriptor.front();
            m_AvailableDescriptor.pop_front();
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUHandleStart, allocated, m_DescriptorSize);
        }

        void ReturnDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& CpuHandle) {
            std::unique_lock lock(m_AvailableDescriptorMutex);

            const SIZE_T offset = (CpuHandle.ptr - m_CPUHandleStart.ptr) / m_DescriptorSize;
            m_AvailableDescriptor.push_back(static_cast<UINT>(offset));
        }

        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& CpuHandle) const {
            const SIZE_T offset = (CpuHandle.ptr - m_CPUHandleStart.ptr) / m_DescriptorSize;
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUHandleStart, static_cast<UINT>(offset), m_DescriptorSize);
        }

        ID3D12DescriptorHeap* GetDescriptorHeap() const {
            return m_Heap.Get();
        }

      private:
        ComPtr<ID3D12Device> m_Device;
        ComPtr<ID3D12DescriptorHeap> m_Heap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandleStart{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandleStart{};
        UINT m_DescriptorSize{0};

        std::mutex m_AvailableDescriptorMutex;
        std::deque<UINT> m_AvailableDescriptor;
    };

} // namespace D3D12Utils