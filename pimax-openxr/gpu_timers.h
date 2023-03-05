#pragma once

#include "pch.h"

namespace pimax_openxr::utils {

    // An asynchronous GPU timer for Direct3D 11.
    struct D3D11GpuTimer : public ITimer {
        D3D11GpuTimer(ID3D11Device* device, ID3D11DeviceContext* context) : m_context(context) {
            D3D11_QUERY_DESC queryDesc;
            ZeroMemory(&queryDesc, sizeof(D3D11_QUERY_DESC));
            queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
            CHECK_HRCMD(device->CreateQuery(&queryDesc, m_timeStampDis.ReleaseAndGetAddressOf()));
            queryDesc.Query = D3D11_QUERY_TIMESTAMP;
            CHECK_HRCMD(device->CreateQuery(&queryDesc, m_timeStampStart.ReleaseAndGetAddressOf()));
            CHECK_HRCMD(device->CreateQuery(&queryDesc, m_timeStampEnd.ReleaseAndGetAddressOf()));
        }

        void start() override {
            m_context->Begin(m_timeStampDis.Get());
            m_context->End(m_timeStampStart.Get());
        }

        void stop() override {
            m_context->End(m_timeStampEnd.Get());
            m_context->End(m_timeStampDis.Get());
            m_valid = true;
        }

        uint64_t query(bool reset = true) const override {
            uint64_t duration = 0;
            if (m_valid) {
                UINT64 startime = 0, endtime = 0;
                D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disData = {0};

                if (m_context->GetData(m_timeStampStart.Get(), &startime, sizeof(UINT64), 0) == S_OK &&
                    m_context->GetData(m_timeStampEnd.Get(), &endtime, sizeof(UINT64), 0) == S_OK &&
                    m_context->GetData(
                        m_timeStampDis.Get(), &disData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) == S_OK &&
                    !disData.Disjoint) {
                    duration = static_cast<uint64_t>(((endtime - startime) * 1e6) / disData.Frequency);
                }
                m_valid = !reset;
            }
            return duration;
        }

      private:
        const ComPtr<ID3D11DeviceContext> m_context;
        ComPtr<ID3D11Query> m_timeStampDis;
        ComPtr<ID3D11Query> m_timeStampStart;
        ComPtr<ID3D11Query> m_timeStampEnd;

        // Can the timer be queried (it might still only read 0).
        mutable bool m_valid{false};
    };

    // An asynchronous GPU timer for Direct3D 12.
    struct D3D12GpuTimer : public ITimer {
        D3D12GpuTimer(ID3D12Device* device, ID3D12CommandQueue* queue) : m_queue(queue) {
            // Create the command context.
            for (uint32_t i = 0; i < 2; i++) {
                CHECK_HRCMD(device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator[i].ReleaseAndGetAddressOf())));
                m_commandAllocator[i]->SetName(L"Timer Command Allocator");
                CHECK_HRCMD(device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      m_commandAllocator[i].Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(m_commandList[i].ReleaseAndGetAddressOf())));
                m_commandList[i]->SetName(L"Timer Command List");
                CHECK_HRCMD(m_commandList[i]->Close());
            }
            CHECK_HRCMD(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
            m_fence->SetName(L"Timer Readback Fence");

            // Create the query heap and readback resources.
            D3D12_QUERY_HEAP_DESC heapDesc{};
            heapDesc.Count = 2;
            heapDesc.NodeMask = 0;
            heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            CHECK_HRCMD(device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(m_queryHeap.ReleaseAndGetAddressOf())));
            m_queryHeap->SetName(L"Timestamp Query Heap");

            D3D12_HEAP_PROPERTIES heapType{};
            heapType.Type = D3D12_HEAP_TYPE_READBACK;
            heapType.CreationNodeMask = heapType.VisibleNodeMask = 1;
            D3D12_RESOURCE_DESC readbackDesc{};
            readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            readbackDesc.Width = heapDesc.Count * sizeof(uint64_t);
            readbackDesc.Height = readbackDesc.DepthOrArraySize = readbackDesc.MipLevels =
                readbackDesc.SampleDesc.Count = 1;
            readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            CHECK_HRCMD(device->CreateCommittedResource(&heapType,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &readbackDesc,
                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                        nullptr,
                                                        IID_PPV_ARGS(m_queryReadbackBuffer.ReleaseAndGetAddressOf())));
            m_queryReadbackBuffer->SetName(L"Query Readback Buffer");
        }

        void start() override {
            CHECK_HRCMD(m_commandAllocator[0]->Reset());
            CHECK_HRCMD(m_commandList[0]->Reset(m_commandAllocator[0].Get(), nullptr));
            m_commandList[0]->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
            CHECK_HRCMD(m_commandList[0]->Close());
            ID3D12CommandList* const lists[] = {m_commandList[0].Get()};
            m_queue->ExecuteCommandLists(1, lists);
        }

        void stop() override {
            CHECK_HRCMD(m_commandAllocator[1]->Reset());
            CHECK_HRCMD(m_commandList[1]->Reset(m_commandAllocator[1].Get(), nullptr));
            m_commandList[1]->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
            m_commandList[1]->ResolveQueryData(
                m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, m_queryReadbackBuffer.Get(), 0);
            CHECK_HRCMD(m_commandList[1]->Close());
            ID3D12CommandList* const lists[] = {m_commandList[1].Get()};
            m_queue->ExecuteCommandLists(1, lists);

            // Signal a fence for completion.
            m_queue->Signal(m_fence.Get(), ++m_fenceValue);
            m_valid = true;
        }

        uint64_t query(bool reset = true) const override {
            uint64_t duration = 0;
            if (m_valid) {
                uint64_t gpuTickFrequency;
                if (m_fence->GetCompletedValue() >= m_fenceValue &&
                    SUCCEEDED(m_queue->GetTimestampFrequency(&gpuTickFrequency))) {
                    uint64_t* mappedBuffer;
                    D3D12_RANGE range{0, 2 * sizeof(uint64_t)};
                    CHECK_HRCMD(m_queryReadbackBuffer->Map(0, &range, reinterpret_cast<void**>(&mappedBuffer)));
                    duration = ((mappedBuffer[1] - mappedBuffer[0]) * 1000000) / gpuTickFrequency;
                    m_queryReadbackBuffer->Unmap(0, nullptr);
                }
                m_valid = !reset;
            }
            return duration;
        }

      private:
        const ComPtr<ID3D12CommandQueue> m_queue;
        ComPtr<ID3D12CommandAllocator> m_commandAllocator[2];
        ComPtr<ID3D12GraphicsCommandList> m_commandList[2];
        ComPtr<ID3D12Fence> m_fence;
        uint64_t m_fenceValue{0};
        ComPtr<ID3D12QueryHeap> m_queryHeap;
        ComPtr<ID3D12Resource> m_queryReadbackBuffer;

        // Can the timer be queried (it might still only read 0).
        mutable bool m_valid{false};
    };

    // An asynchronous GPU timer for Vulkan.
    struct VulkanGpuTimer : public ITimer {
        VulkanGpuTimer(const VulkanDispatch& dispatch,
                       VkPhysicalDevice physicalDevice,
                       VkDevice device,
                       VkQueue queue,
                       uint32_t queueFamilyIndex,
                       const std::optional<VkAllocationCallbacks>& allocator)
            : m_dispatch(dispatch), m_device(device), m_queue(queue), m_allocator(allocator) {
            // Query the timestamp period.
            VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            m_dispatch.vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
            m_timestampPeriod = properties.properties.limits.timestampPeriod;

            // Create the command context.
            VkCommandPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolCreateInfo.queueFamilyIndex = queueFamilyIndex;
            CHECK_VKCMD(m_dispatch.vkCreateCommandPool(
                m_device, &poolCreateInfo, m_allocator ? &m_allocator.value() : nullptr, &m_cmdPool));
            VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            allocateInfo.commandPool = m_cmdPool;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 2;
            CHECK_VKCMD(m_dispatch.vkAllocateCommandBuffers(m_device, &allocateInfo, m_cmdBuffer));

            // Create the query pool.
            VkQueryPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
            createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
            createInfo.queryCount = 2;
            CHECK_VKCMD(m_dispatch.vkCreateQueryPool(
                m_device, &createInfo, m_allocator ? &m_allocator.value() : nullptr, &m_queryPool));
        }

        ~VulkanGpuTimer() override {
            if (m_queryPool != VK_NULL_HANDLE) {
                m_dispatch.vkDestroyQueryPool(m_device, m_queryPool, m_allocator ? &m_allocator.value() : nullptr);
            }
            if (m_cmdBuffer[0] != VK_NULL_HANDLE) {
                m_dispatch.vkFreeCommandBuffers(m_device, m_cmdPool, 2, m_cmdBuffer);
            }
            if (m_cmdPool != VK_NULL_HANDLE) {
                m_dispatch.vkDestroyCommandPool(m_device, m_cmdPool, m_allocator ? &m_allocator.value() : nullptr);
            }
        }

        void start() override {
            VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CHECK_VKCMD(m_dispatch.vkBeginCommandBuffer(m_cmdBuffer[0], &beginInfo));
            m_dispatch.vkCmdResetQueryPool(m_cmdBuffer[0], m_queryPool, 0, 2);
            m_dispatch.vkCmdWriteTimestamp(m_cmdBuffer[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPool, 0);
            CHECK_VKCMD(m_dispatch.vkEndCommandBuffer(m_cmdBuffer[0]));
            VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_cmdBuffer[0];
            CHECK_VKCMD(m_dispatch.vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
        }

        void stop() override {
            VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CHECK_VKCMD(m_dispatch.vkBeginCommandBuffer(m_cmdBuffer[1], &beginInfo));
            m_dispatch.vkCmdWriteTimestamp(m_cmdBuffer[1], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, 1);
            CHECK_VKCMD(m_dispatch.vkEndCommandBuffer(m_cmdBuffer[1]));
            VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_cmdBuffer[1];
            CHECK_VKCMD(m_dispatch.vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
            m_valid = true;
        }

        uint64_t query(bool reset = true) const override {
            uint64_t duration = 0;
            if (m_valid) {
                uint64_t buffer[2];
                VkResult result = m_dispatch.vkGetQueryPoolResults(m_device,
                                                                   m_queryPool,
                                                                   0,
                                                                   2,
                                                                   sizeof(uint64_t) * 2,
                                                                   buffer,
                                                                   sizeof(uint64_t),
                                                                   VK_QUERY_RESULT_64_BIT);
                if (result == VK_SUCCESS) {
                    duration = static_cast<uint64_t>(((buffer[1] - buffer[0]) * m_timestampPeriod) / 1000);
                }

                m_valid = !reset;
            }
            return duration;
        }

      private:
        const VulkanDispatch& m_dispatch;
        const VkDevice m_device;
        const VkQueue m_queue;
        const std::optional<VkAllocationCallbacks> m_allocator;
        float m_timestampPeriod{};
        VkCommandPool m_cmdPool{VK_NULL_HANDLE};
        VkCommandBuffer m_cmdBuffer[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
        VkQueryPool m_queryPool{VK_NULL_HANDLE};

        // Can the timer be queried (it might still only read 0).
        mutable bool m_valid{false};
    };

    // An asynchronous GPU timer for OpenGL.
    struct GlGpuTimer : public ITimer {
        GlGpuTimer(const GlDispatch& dispatch, const GlContext& context) : m_dispatch(dispatch), m_context(context) {
            GlContextSwitch context_(m_context);

            m_dispatch.glGenQueries(2, m_queries);
        }

        ~GlGpuTimer() override {
            GlContextSwitch context(m_context);

            m_dispatch.glDeleteQueries(2, m_queries);
        }

        void start() override {
            GlContextSwitch context(m_context);

            m_dispatch.glQueryCounter(m_queries[0], GL_TIMESTAMP);
        }

        void stop() override {
            GlContextSwitch context(m_context);

            m_dispatch.glQueryCounter(m_queries[1], GL_TIMESTAMP);
            m_valid = true;
        }

        uint64_t query(bool reset = true) const override {
            uint64_t duration = 0;
            if (m_valid) {
                GlContextSwitch context(m_context);

                GLint stopTimerAvailable = 0;
                m_dispatch.glGetQueryObjectiv(m_queries[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
                if (stopTimerAvailable) {
                    uint64_t startTime, stopTime;
                    m_dispatch.glGetQueryObjectui64v(m_queries[0], GL_QUERY_RESULT, &startTime);
                    m_dispatch.glGetQueryObjectui64v(m_queries[1], GL_QUERY_RESULT, &stopTime);
                    duration = (stopTime - startTime) / 1000;
                }
                m_valid = !reset;
            }
            return duration;
        }

      private:
        const GlDispatch& m_dispatch;
        const GlContext& m_context;

        GLuint m_queries[2]{};

        // Can the timer be queried (it might still only read 0).
        mutable bool m_valid{false};
    };

} // namespace pimax_openxr::utils
