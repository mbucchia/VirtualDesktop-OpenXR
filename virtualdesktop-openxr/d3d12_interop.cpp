// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "pch.h"

#include "log.h"
#include "runtime.h"
#include "utils.h"

// Implements the necessary support for the XR_KHR_D3D12_enable extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_D3D12_enable

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetD3D12GraphicsRequirementsKHR
    XrResult OpenXrRuntime::xrGetD3D12GraphicsRequirementsKHR(XrInstance instance,
                                                              XrSystemId systemId,
                                                              XrGraphicsRequirementsD3D12KHR* graphicsRequirements) {
        if (graphicsRequirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetD3D12GraphicsRequirementsKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"));

        if (!has_XR_KHR_D3D12_enable) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        memcpy(&graphicsRequirements->adapterLuid, &m_adapterLuid, sizeof(LUID));
        graphicsRequirements->minFeatureLevel = D3D_FEATURE_LEVEL_12_0;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetD3D12GraphicsRequirementsKHR",
                          TraceLoggingCharArray((char*)&graphicsRequirements->adapterLuid, sizeof(LUID), "AdapterLuid"),
                          TLArg((int)graphicsRequirements->minFeatureLevel, "MinFeatureLevel"));

        m_graphicsRequirementQueried = true;

        return XR_SUCCESS;
    }

    // Initialize all the resources needed for D3D12 interoperation with the D3D11 backend.
    XrResult OpenXrRuntime::initializeD3D12(const XrGraphicsBindingD3D12KHR& d3dBindings) {
        if (!d3dBindings.device || !d3dBindings.queue) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        // Check that this is the correct adapter for the HMD.
        ComPtr<IDXGIFactory1> dxgiFactory;
        CHECK_HRCMD(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        const auto adapterLuid = d3dBindings.device->GetAdapterLuid();
        ComPtr<IDXGIAdapter1> dxgiAdapter;
        for (UINT adapterIndex = 0;; adapterIndex++) {
            // EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to
            // enumerate.
            CHECK_HRCMD(dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf()));

            DXGI_ADAPTER_DESC1 desc;
            CHECK_HRCMD(dxgiAdapter->GetDesc1(&desc));
            if (!memcmp(&desc.AdapterLuid, &adapterLuid, sizeof(LUID))) {
                const std::string deviceName = xr::wide_to_utf8(desc.Description);

                TraceLoggingWrite(g_traceProvider,
                                  "xrCreateSession",
                                  TLArg("D3D12", "Api"),
                                  TLArg(deviceName.c_str(), "AdapterName"));
                Log("Using Direct3D 12 on adapter: %s\n", deviceName.c_str());
                break;
            }
        }

        if (memcmp(&adapterLuid, &m_adapterLuid, sizeof(LUID))) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        m_d3d12Device = d3dBindings.device;
        m_d3d12CommandQueue = d3dBindings.queue;

        // Create the interop device and resources that OVR will be using.
        initializeSubmissionDevice("D3D12");

        // We will use a shared fence to synchronize between the D3D12 queue and the D3D11
        // context.
        wil::unique_handle fenceHandle;
        CHECK_HRCMD(m_ovrSubmissionFence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, fenceHandle.put()));
        CHECK_HRCMD(
            m_d3d12Device->OpenSharedHandle(fenceHandle.get(), IID_PPV_ARGS(m_d3d12Fence.ReleaseAndGetAddressOf())));

        // We will need command lists to perform layout transitions.
        CHECK_HRCMD(m_d3d12Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_d3d12CommandAllocator.ReleaseAndGetAddressOf())));
        CHECK_HRCMD(m_d3d12Device->CreateCommandList(0,
                                                     D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     m_d3d12CommandAllocator.Get(),
                                                     nullptr,
                                                     IID_PPV_ARGS(m_d3d12CommandList.ReleaseAndGetAddressOf())));
        CHECK_HRCMD(m_d3d12CommandList->Close());

        // Frame timers.
        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i] = std::make_unique<D3D12GpuTimer>(m_d3d12Device.Get(), m_d3d12CommandQueue.Get());
        }

        return XR_SUCCESS;
    }

    void OpenXrRuntime::cleanupD3D12() {
        flushD3D12CommandQueue();

        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i].reset();
        }
        m_d3d12CommandList.Reset();
        m_d3d12CommandAllocator.Reset();
        m_d3d12Fence.Reset();
        m_d3d12CommandQueue.Reset();
        m_d3d12Device.Reset();
    }

    bool OpenXrRuntime::isD3D12Session() const {
        return !!m_d3d12Device;
    }

    // Retrieve the swapchain images (ID3D12Resource) for the application to use.
    XrResult OpenXrRuntime::getSwapchainImagesD3D12(Swapchain& xrSwapchain,
                                                    XrSwapchainImageD3D12KHR* d3d12Images,
                                                    uint32_t count) {
        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();

        const bool needTransition = xrSwapchain.xrDesc.usageFlags & (XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                                                                     XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        std::vector<HANDLE> textureHandles;
        if (!initialized) {
            // Query the swapchain textures.
            textureHandles = getSwapchainImages(xrSwapchain);

            if (needTransition) {
                // We keep our code simple by only using a single command list, which means we must wait before reusing
                // it.
                flushD3D12CommandQueue();

                // Prepare to execute barriers.
                CHECK_HRCMD(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));
            }
        }

        // Export each D3D11 texture to D3D12.
        for (uint32_t i = 0; i < count; i++) {
            if (d3d12Images[i].type != XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                // Create an imported texture on the D3D12 device.
                ComPtr<ID3D12Resource> d3d12Resource;
                CHECK_HRCMD(m_d3d12Device->OpenSharedHandle(textureHandles[i],
                                                            IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())));
                setDebugName(d3d12Resource.Get(), fmt::format("App Swapchain Texture[{}, {}]", i, (void*)&xrSwapchain));

                xrSwapchain.d3d12Images.push_back(d3d12Resource);

                if (needTransition) {
                    D3D12_RESOURCE_BARRIER barrier{};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Transition.pResource = d3d12Resource.Get();
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                    barrier.Transition.StateAfter =
                        xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT
                            ? D3D12_RESOURCE_STATE_RENDER_TARGET
                            : D3D12_RESOURCE_STATE_DEPTH_WRITE;
                    m_d3d12CommandList->ResourceBarrier(1, &barrier);
                }
            }

            d3d12Images[i].texture = xrSwapchain.d3d12Images[i].Get();

            if (i == 0) {
                const auto& desc = d3d12Images[i].texture->GetDesc();

                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateSwapchainImages",
                                  TLArg("D3D12", "Api"),
                                  TLArg("Runtime", "Type"),
                                  TLArg(desc.Width, "Width"),
                                  TLArg(desc.Height, "Height"),
                                  TLArg(desc.DepthOrArraySize, "ArraySize"),
                                  TLArg(desc.MipLevels, "MipCount"),
                                  TLArg(desc.SampleDesc.Count, "SampleCount"),
                                  TLArg((int)desc.Format, "Format"),
                                  TLArg((int)desc.Flags, "Flags"));
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainImages",
                              TLArg("D3D12", "Api"),
                              TLPArg(d3d12Images[i].texture, "Texture"));
        }

        if (!initialized && needTransition) {
            // Transition all images to the desired state.
            CHECK_HRCMD(m_d3d12CommandList->Close());
            m_d3d12CommandQueue->ExecuteCommandLists(
                1, reinterpret_cast<ID3D12CommandList**>(m_d3d12CommandList.GetAddressOf()));
        }

        return XR_SUCCESS;
    }

    // Wait for all pending commands to finish.
    void OpenXrRuntime::flushD3D12CommandQueue() {
        if (m_d3d12CommandQueue && m_d3d12Fence) {
            wil::unique_handle eventHandle;
            m_fenceValue++;
            TraceLoggingWrite(
                g_traceProvider, "FlushContext_Wait", TLArg("D3D12", "Api"), TLArg(m_fenceValue, "FenceValue"));
            m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), m_fenceValue);
            *eventHandle.put() = CreateEventEx(nullptr, L"Flush Fence", 0, EVENT_ALL_ACCESS);
            CHECK_HRCMD(m_d3d12Fence->SetEventOnCompletion(m_fenceValue, eventHandle.get()));
            WaitForSingleObject(eventHandle.get(), INFINITE);
            ResetEvent(eventHandle.get());
        }
    }

    // Serialize commands from the D3D12 queue to the D3D11 context used by OVR.
    void OpenXrRuntime::serializeD3D12Frame() {
        m_fenceValue++;
        TraceLoggingWrite(g_traceProvider, "xrEndFrame_Sync", TLArg("D3D12", "Api"), TLArg(m_fenceValue, "FenceValue"));
        CHECK_HRCMD(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), m_fenceValue));

        waitOnSubmissionDevice();
    }

} // namespace virtualdesktop_openxr
