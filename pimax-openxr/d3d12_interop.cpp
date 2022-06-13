// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
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

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

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

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (!m_isD3D12Supported) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        // Get the display device LUID.
        fillDisplayDeviceInfo();

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
                std::string deviceName;
                const std::wstring wadapterDescription(desc.Description);
                std::transform(wadapterDescription.begin(),
                               wadapterDescription.end(),
                               std::back_inserter(deviceName),
                               [](wchar_t c) { return (char)c; });

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

        // Create the interop device that PVR will be using.
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> deviceContext;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        CHECK_HRCMD(D3D11CreateDevice(dxgiAdapter.Get(),
                                      D3D_DRIVER_TYPE_UNKNOWN,
                                      0,
                                      flags,
                                      &featureLevel,
                                      1,
                                      D3D11_SDK_VERSION,
                                      device.ReleaseAndGetAddressOf(),
                                      nullptr,
                                      deviceContext.ReleaseAndGetAddressOf()));

        ComPtr<ID3D11Device5> device5;
        CHECK_HRCMD(device->QueryInterface(m_d3d11Device.ReleaseAndGetAddressOf()));

        // Create the Direct3D 11 resources.
        XrGraphicsBindingD3D11KHR d3d11Bindings{};
        d3d11Bindings.device = device.Get();
        const auto result = initializeD3D11(d3d11Bindings, true);
        if (XR_FAILED(result)) {
            return result;
        }

        // We will use a shared fence to synchronize between the D3D12 queue and the D3D11
        // context.
        CHECK_HRCMD(m_d3d12Device->CreateFence(
            0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(m_d3d12Fence.ReleaseAndGetAddressOf())));
        wil::unique_handle fenceHandle = nullptr;
        CHECK_HRCMD(
            m_d3d12Device->CreateSharedHandle(m_d3d12Fence.Get(), nullptr, GENERIC_ALL, nullptr, fenceHandle.put()));
        m_d3d11Device->OpenSharedFence(fenceHandle.get(), IID_PPV_ARGS(m_d3d11Fence.ReleaseAndGetAddressOf()));
        m_fenceValue = 0;

        return XR_SUCCESS;
    }

    void OpenXrRuntime::cleanupD3D12() {
        // Wait for all the queued work to complete.
        if (m_d3d12CommandQueue && m_d3d12Fence) {
            wil::unique_handle eventHandle;
            m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), ++m_fenceValue);
            *eventHandle.put() = CreateEventEx(nullptr, L"Flush Fence", 0, EVENT_ALL_ACCESS);
            CHECK_HRCMD(m_d3d12Fence->SetEventOnCompletion(m_fenceValue, eventHandle.get()));
            WaitForSingleObject(eventHandle.get(), INFINITE);
            ResetEvent(eventHandle.get());
        }

        m_d3d12Fence.Reset();
        m_d3d11Fence.Reset();
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

        std::vector<XrSwapchainImageD3D11KHR> d3d11Images(count, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        if (!initialized) {
            // Query the D3D11 textures.
            const auto result = getSwapchainImagesD3D11(xrSwapchain, d3d11Images.data(), count, true);
            if (XR_FAILED(result)) {
                return result;
            }
        }

        // Export each D3D11 texture to D3D12.
        for (uint32_t i = 0; i < count; i++) {
            if (d3d12Images[i].type != XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                // Create an imported texture on the D3D12 device.
                HANDLE textureHandle;
                ComPtr<IDXGIResource1> dxgiResource;
                CHECK_HRCMD(
                    d3d11Images[i].texture->QueryInterface(IID_PPV_ARGS(dxgiResource.ReleaseAndGetAddressOf())));
                CHECK_HRCMD(dxgiResource->GetSharedHandle(&textureHandle));

                ComPtr<ID3D12Resource> d3d12Resource;
                CHECK_HRCMD(m_d3d12Device->OpenSharedHandle(textureHandle,
                                                            IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())));
                setDebugName(d3d12Resource.Get(), fmt::format("App Interop Texture[{}, {}]", i, (void*)&xrSwapchain));

                xrSwapchain.d3d12Images.push_back(d3d12Resource);
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

        return XR_SUCCESS;
    }

    // Serialize commands from the D3D12 queue to the D3D11 context used by PVR.
    void OpenXrRuntime::serializeD3D12Frame() {
        m_fenceValue++;
        TraceLoggingWrite(g_traceProvider, "xrEndFrame_Sync", TLArg("D3D12", "Api"), TLArg(m_fenceValue, "FenceValue"));
        CHECK_HRCMD(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), m_fenceValue));
        CHECK_HRCMD(m_d3d11DeviceContext->Wait(m_d3d11Fence.Get(), m_fenceValue));
    }

} // namespace pimax_openxr
