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

#include "AlphaBlendingCS.h"
#include "AlphaBlendingTexArrayCS.h"
#include "FullScreenQuadVS.h"
#include "PassthroughPS.h"

// Implements native support to submit swapchains to OVR.
// Implements the necessary support for the XR_KHR_D3D11_enable extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_D3D11_enable

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;

    struct AlphaBlendingCSConstants {
        alignas(4) bool ignoreAlpha;
        alignas(4) bool isUnpremultipliedAlpha;
    };

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetD3D11GraphicsRequirementsKHR
    XrResult OpenXrRuntime::xrGetD3D11GraphicsRequirementsKHR(XrInstance instance,
                                                              XrSystemId systemId,
                                                              XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
        if (graphicsRequirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetD3D11GraphicsRequirementsKHR",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"));

        if (!has_XR_KHR_D3D11_enable) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        memcpy(&graphicsRequirements->adapterLuid, &m_adapterLuid, sizeof(LUID));
        graphicsRequirements->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetD3D11GraphicsRequirementsKHR",
                          TraceLoggingCharArray((char*)&graphicsRequirements->adapterLuid, sizeof(LUID), "AdapterLuid"),
                          TLArg((int)graphicsRequirements->minFeatureLevel, "MinFeatureLevel"));

        m_graphicsRequirementQueried = true;

        return XR_SUCCESS;
    }

    // Initialize all the resources needed for D3D11 support, both on the API frontend and also the runtime/OVR backend.
    XrResult OpenXrRuntime::initializeD3D11(const XrGraphicsBindingD3D11KHR& d3dBindings) {
        if (!d3dBindings.device) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        // Check that this is the correct adapter for the HMD.
        ComPtr<IDXGIDevice> dxgiDevice;
        CHECK_HRCMD(d3dBindings.device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter> dxgiAdapter;
        CHECK_HRCMD(dxgiDevice->GetAdapter(dxgiAdapter.ReleaseAndGetAddressOf()));

        DXGI_ADAPTER_DESC desc;
        CHECK_HRCMD(dxgiAdapter->GetDesc(&desc));

        if (memcmp(&desc.AdapterLuid, &m_adapterLuid, sizeof(LUID))) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        // Query the necessary flavors of device & device context which will let use use fences.
        CHECK_HRCMD(d3dBindings.device->QueryInterface(m_d3d11Device.ReleaseAndGetAddressOf()));
        ComPtr<ID3D11DeviceContext> deviceContext;
        d3dBindings.device->GetImmediateContext(deviceContext.ReleaseAndGetAddressOf());
        CHECK_HRCMD(deviceContext->QueryInterface(m_d3d11Context.ReleaseAndGetAddressOf()));

        if (m_useApplicationDeviceForSubmission) {
            // Try reusing the application device to avoid fence synchronization every frame.
            const std::string deviceName = xr::wide_to_utf8(desc.Description);
            TraceLoggingWrite(
                g_traceProvider, "xrCreateSession", TLArg("D3D11", "Api"), TLArg(deviceName.c_str(), "AdapterName"));
            Log("Using D3D11 on adapter: %s\n", deviceName.c_str());

            m_ovrSubmissionDevice = m_d3d11Device;
            m_ovrSubmissionContext = m_d3d11Context;

            UINT creationFlags = 0;
            if (m_ovrSubmissionDevice->GetCreationFlags() & D3D11_CREATE_DEVICE_SINGLETHREADED) {
                creationFlags |= D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED;
            }
            const D3D_FEATURE_LEVEL featureLevel = m_ovrSubmissionDevice->GetFeatureLevel();

            CHECK_HRCMD(
                m_ovrSubmissionDevice->CreateDeviceContextState(creationFlags,
                                                                &featureLevel,
                                                                1,
                                                                D3D11_SDK_VERSION,
                                                                __uuidof(ID3D11Device),
                                                                nullptr,
                                                                m_ovrSubmissionContextState.ReleaseAndGetAddressOf()));

            initializeSubmissionResources();
        } else {
            // Create the resources that OVR will be using.
            initializeSubmissionDevice("D3D11");
        }

        // We will use a shared fence to synchronize between the application context and the OVR (submission) context
        // context.
        wil::unique_handle fenceHandle;
        CHECK_HRCMD(m_ovrSubmissionFence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, fenceHandle.put()));
        CHECK_HRCMD(
            m_d3d11Device->OpenSharedFence(fenceHandle.get(), IID_PPV_ARGS(m_d3d11Fence.ReleaseAndGetAddressOf())));
        *m_eventForSubmissionFence.put() = CreateEventEx(nullptr, L"Submission Fence", 0, EVENT_ALL_ACCESS);

        // Frame timers.
        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i] = std::make_unique<D3D11GpuTimer>(m_d3d11Device.Get(), m_d3d11Context.Get());
        }

        return XR_SUCCESS;
    }

    // Initialize all the resources for the OVR backend.
    void OpenXrRuntime::initializeSubmissionDevice(const std::string& appGraphicsApi) {
        ComPtr<IDXGIFactory1> dxgiFactory;
        CHECK_HRCMD(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter1> dxgiAdapter;
        for (UINT adapterIndex = 0;; adapterIndex++) {
            // EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to
            // enumerate.
            CHECK_HRCMD(dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf()));

            DXGI_ADAPTER_DESC1 desc;
            CHECK_HRCMD(dxgiAdapter->GetDesc1(&desc));
            if (!memcmp(&desc.AdapterLuid, &m_adapterLuid, sizeof(LUID))) {
                const std::string deviceName = xr::wide_to_utf8(desc.Description);

                TraceLoggingWrite(g_traceProvider,
                                  "xrCreateSession",
                                  TLArg(appGraphicsApi.c_str(), "Api"),
                                  TLArg(deviceName.c_str(), "AdapterName"));
                Log("Using %s on adapter: %s\n", appGraphicsApi.c_str(), deviceName.c_str());
                break;
            }
        }

        // Create the submission device that OVR will be using.
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> deviceContext;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
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

        // Query the necessary flavors of device & device context, which will let use use fences.
        CHECK_HRCMD(device->QueryInterface(m_ovrSubmissionDevice.ReleaseAndGetAddressOf()));
        CHECK_HRCMD(deviceContext->QueryInterface(m_ovrSubmissionContext.ReleaseAndGetAddressOf()));

        initializeSubmissionResources();
    }

    void OpenXrRuntime::initializeSubmissionResources() {
        // Create the synchronization fence to serialize work between the application device and submission device.
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateFence(
            0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(m_ovrSubmissionFence.ReleaseAndGetAddressOf())));
        m_fenceValue = 0;

        // Create the resources for alpha correction.
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateComputeShader(
            g_AlphaBlendingCS, sizeof(g_AlphaBlendingCS), nullptr, m_alphaCorrectShader[0].ReleaseAndGetAddressOf()));
        setDebugName(m_alphaCorrectShader[0].Get(), "AlphaBlending CS");
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateComputeShader(g_AlphaBlendingTexArrayCS,
                                                               sizeof(g_AlphaBlendingTexArrayCS),
                                                               nullptr,
                                                               m_alphaCorrectShader[1].ReleaseAndGetAddressOf()));
        setDebugName(m_alphaCorrectShader[1].Get(), "AlphaBlending CS");
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateVertexShader(
            g_FullScreenQuadVS, sizeof(g_FullScreenQuadVS), nullptr, m_fullQuadVS.ReleaseAndGetAddressOf()));
        setDebugName(m_fullQuadVS.Get(), "FullQuad VS");
        CHECK_HRCMD(m_ovrSubmissionDevice->CreatePixelShader(
            g_PassthroughPS, sizeof(g_PassthroughPS), nullptr, m_colorConversionPS.ReleaseAndGetAddressOf()));
        setDebugName(m_fullQuadVS.Get(), "ColorConversion PS");

        {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MaxAnisotropy = 1;
            desc.MinLOD = D3D11_MIP_LOD_BIAS_MIN;
            desc.MaxLOD = D3D11_MIP_LOD_BIAS_MAX;
            CHECK_HRCMD(
                m_ovrSubmissionDevice->CreateSamplerState(&desc, m_linearClampSampler.ReleaseAndGetAddressOf()));
        }
        {
            D3D11_RASTERIZER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            desc.FrontCounterClockwise = TRUE;
            CHECK_HRCMD(
                m_ovrSubmissionDevice->CreateRasterizerState(&desc, m_noDepthRasterizer.ReleaseAndGetAddressOf()));
        }

        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerPrecomposition[i] =
                std::make_unique<D3D11GpuTimer>(m_ovrSubmissionDevice.Get(), m_ovrSubmissionContext.Get());
        }

        // If RenderDoc is loaded, then create a DXGI swapchain to signal events. Otherwise RenderDoc will
        // not see our OpenXR frames.
        HMODULE renderdocModule;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "renderdoc.dll", &renderdocModule) &&
            renderdocModule) {
            TraceLoggingWrite(g_traceProvider, "xrCreateSession", TLArg("True", "RenderDoc"));
            Log("Detected RenderDoc\n");

            DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
            swapchainDesc.Width = 8;
            swapchainDesc.Height = 8;
            swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapchainDesc.SampleDesc.Count = 1;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.BufferCount = 3;
            swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

            ComPtr<IDXGIDevice> dxgiDevice;
            CHECK_HRCMD(m_ovrSubmissionDevice->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

            ComPtr<IDXGIAdapter> dxgiAdapter;
            CHECK_HRCMD(dxgiDevice->GetAdapter(dxgiAdapter.ReleaseAndGetAddressOf()));

            ComPtr<IDXGIFactory2> dxgiFactory;
            CHECK_HRCMD(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
            CHECK_HRCMD(dxgiFactory->CreateSwapChainForComposition(
                dxgiDevice.Get(), &swapchainDesc, nullptr, m_dxgiSwapchain.ReleaseAndGetAddressOf()));
        }
    }

    void OpenXrRuntime::cleanupD3D11() {
        flushD3D11Context();

        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i].reset();
        }

        m_d3d11ContextState.Reset();
        m_d3d11Context.Reset();
        m_d3d11Device.Reset();
    }

    void OpenXrRuntime::cleanupSubmissionDevice() {
        flushSubmissionContext();

        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerPrecomposition[i].reset();
        }

        m_dxgiSwapchain.Reset();
        for (int i = 0; i < ARRAYSIZE(m_alphaCorrectShader); i++) {
            m_alphaCorrectShader[i].Reset();
        }

        m_ovrSubmissionFence.Reset();
        m_ovrSubmissionContextState.Reset();
        m_ovrSubmissionContext.Reset();
        m_ovrSubmissionDevice.Reset();
        m_eventForSubmissionFence.reset();
    }

    // Retrieve generic handles to the swapchain images to import into the application device.
    std::vector<HANDLE> OpenXrRuntime::getSwapchainImages(Swapchain& xrSwapchain) {
        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();

        // Query the textures for the swapchain.
        std::vector<HANDLE> handles;
        for (int i = 0; i < xrSwapchain.ovrSwapchainLength; i++) {
            if (!initialized) {
                ComPtr<ID3D11Texture2D> swapchainTexture;
                CHECK_OVRCMD(ovr_GetTextureSwapChainBufferDX(m_ovrSession,
                                                             xrSwapchain.ovrSwapchain[0],
                                                             i,
                                                             IID_PPV_ARGS(swapchainTexture.ReleaseAndGetAddressOf())));
                setDebugName(swapchainTexture.Get(),
                             fmt::format("OVR Swapchain Texture[{}, {}]", i, (void*)&xrSwapchain));

                xrSwapchain.slices[0].push_back(swapchainTexture);
                if (i == 0) {
                    D3D11_TEXTURE2D_DESC desc;
                    swapchainTexture->GetDesc(&desc);
                    TraceLoggingWrite(g_traceProvider,
                                      "xrEnumerateSwapchainImages",
                                      TLArg("D3D11", "Api"),
                                      TLArg("OVR", "Type"),
                                      TLArg(desc.Width, "Width"),
                                      TLArg(desc.Height, "Height"),
                                      TLArg(desc.ArraySize, "ArraySize"),
                                      TLArg(desc.MipLevels, "MipCount"),
                                      TLArg(desc.SampleDesc.Count, "SampleCount"),
                                      TLArg((int)desc.Format, "Format"),
                                      TLArg((int)desc.Usage, "Usage"),
                                      TLArg(desc.BindFlags, "BindFlags"),
                                      TLArg(desc.CPUAccessFlags, "CPUAccessFlags"),
                                      TLArg(desc.MiscFlags, "MiscFlags"));
                }

                xrSwapchain.images.push_back(swapchainTexture);
                for (uint32_t i = 0; i < xrSwapchain.xrDesc.arraySize; i++) {
                    xrSwapchain.imagesResourceView[i].push_back({});
                    xrSwapchain.renderTargetView[i].push_back({});
                }
            }

            // Export the HANDLE.
            const auto texture = xrSwapchain.slices[0][i];

            ComPtr<IDXGIResource1> dxgiResource;
            CHECK_HRCMD(texture->QueryInterface(IID_PPV_ARGS(dxgiResource.ReleaseAndGetAddressOf())));

            HANDLE textureHandle;
            CHECK_HRCMD(dxgiResource->GetSharedHandle(&textureHandle));

            handles.push_back(textureHandle);
        }

        return handles;
    }

    // Retrieve the swapchain images (ID3D11Texture2D) for the application to use.
    XrResult OpenXrRuntime::getSwapchainImagesD3D11(Swapchain& xrSwapchain,
                                                    XrSwapchainImageD3D11KHR* d3d11Images,
                                                    uint32_t count) {
        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();
        const bool skipSharing = m_ovrSubmissionDevice == m_d3d11Device;

        std::vector<HANDLE> textureHandles;
        if (!initialized) {
            // Query the swapchain textures.
            textureHandles = getSwapchainImages(xrSwapchain);
        }

        // Export each D3D11 texture from the submission device into the application device.
        for (uint32_t i = 0; i < count; i++) {
            if (d3d11Images[i].type != XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                ComPtr<ID3D11Texture2D> d3d11Texture;
                if (!skipSharing) {
                    // Create an imported texture on the application device.
                    CHECK_HRCMD(m_d3d11Device->OpenSharedResource(textureHandles[i],
                                                                  IID_PPV_ARGS(d3d11Texture.ReleaseAndGetAddressOf())));
                } else {
                    CHECK_OVRCMD(ovr_GetTextureSwapChainBufferDX(m_ovrSession,
                                                                 xrSwapchain.ovrSwapchain[0],
                                                                 i,
                                                                 IID_PPV_ARGS(d3d11Texture.ReleaseAndGetAddressOf())));
                }

                setDebugName(d3d11Texture.Get(), fmt::format("App Swapchain Texture[{}, {}]", i, (void*)&xrSwapchain));

                xrSwapchain.d3d11Images.push_back(d3d11Texture);
            }

            d3d11Images[i].texture = xrSwapchain.d3d11Images[i].Get();

            if (i == 0) {
                D3D11_TEXTURE2D_DESC desc;
                d3d11Images[0].texture->GetDesc(&desc);
                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateSwapchainImages",
                                  TLArg("D3D11", "Api"),
                                  TLArg("Runtime", "Type"),
                                  TLArg(desc.Width, "Width"),
                                  TLArg(desc.Height, "Height"),
                                  TLArg(desc.ArraySize, "ArraySize"),
                                  TLArg(desc.MipLevels, "MipCount"),
                                  TLArg(desc.SampleDesc.Count, "SampleCount"),
                                  TLArg((int)desc.Format, "Format"),
                                  TLArg((int)desc.Usage, "Usage"),
                                  TLArg(desc.BindFlags, "BindFlags"),
                                  TLArg(desc.CPUAccessFlags, "CPUAccessFlags"),
                                  TLArg(desc.MiscFlags, "MiscFlags"));
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainImages",
                              TLArg("D3D11", "Api"),
                              TLPArg(d3d11Images[i].texture, "Texture"));
        }

        return XR_SUCCESS;
    }

    // Prepare an OVR swapchain to be used by OVR.
    void OpenXrRuntime::prepareAndCommitSwapchainImage(Swapchain& xrSwapchain,
                                                       uint32_t layerIndex,
                                                       uint32_t slice,
                                                       XrCompositionLayerFlags compositionFlags,
                                                       std::set<std::pair<ovrTextureSwapChain, uint32_t>>& committed) {
        // If the texture was never used or already committed, do nothing.
        if (xrSwapchain.slices[0].empty() || committed.count(std::make_pair(xrSwapchain.ovrSwapchain[0], slice))) {
            return;
        }

        ensureSwapchainSliceResources(xrSwapchain, slice);

        int ovrDestIndex = -1;
        CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(m_ovrSession, xrSwapchain.ovrSwapchain[slice], &ovrDestIndex));
        const int lastReleasedIndex = xrSwapchain.lastReleasedIndex;

        const bool needClearAlpha =
            layerIndex > 0 && !(compositionFlags & XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT);
        // Workaround: this is questionable, but an app should always submit layer 0 without alpha-blending (ie: alpha =
        // 1). This avoids needing to run the premultiply alpha shader only do multiply all values by 1...
        const bool needPremultiplyAlpha =
            layerIndex > 0 && (compositionFlags & XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT);
        const bool needCopy = xrSwapchain.lastProcessedIndex[slice] == lastReleasedIndex ||
                              (slice > 0 && !(needClearAlpha || needPremultiplyAlpha));

        if (needCopy) {
            // Circumvent some of OVR's limitations:
            // - For texture arrays, we must do a copy to slice 0 into another swapchain.
            // - Committing into a swapchain automatically acquires the next image. When an app renders certain
            //   swapchains (eg: quad layers) at a lower frame rate, we must perform a copy to the current OVR swapchain
            //   image. All the processing needed (eg: alpha correction) was done during initial processing (the first
            //   time we saw the last released image), so no need to redo it.
            m_ovrSubmissionContext->CopySubresourceRegion(xrSwapchain.slices[slice][ovrDestIndex].Get(),
                                                          0,
                                                          0,
                                                          0,
                                                          0,
                                                          xrSwapchain.slices[0][lastReleasedIndex].Get(),
                                                          slice,
                                                          nullptr);
        } else if (needClearAlpha || needPremultiplyAlpha) {
            // Circumvent some of OVR's limitations:
            // - For alpha-blended layers, we must pre-process the alpha channel.
            // For alpha-blended layers with texture arrays, we must also output into slice 0 of
            // another swapchain (see other branch above).
            //
            // One more difficulty: because we use a compute shader, we cannot use an SRGB format as destination. We
            // might need to do a conversion pass at the very end.

            ensureSwapchainIntermediateResources(xrSwapchain);

            // Lazily create SRV.
            if (!xrSwapchain.imagesResourceView[slice][lastReleasedIndex]) {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc{};

                desc.ViewDimension = xrSwapchain.xrDesc.arraySize == 1 ? D3D11_SRV_DIMENSION_TEXTURE2D
                                                                       : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Format = xrSwapchain.dxgiFormatForSubmission;
                desc.Texture2DArray.ArraySize = 1;
                desc.Texture2DArray.MipLevels = xrSwapchain.xrDesc.mipCount;
                desc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, slice, desc.Texture2DArray.MipLevels);

                CHECK_HRCMD(m_ovrSubmissionDevice->CreateShaderResourceView(
                    xrSwapchain.images[lastReleasedIndex].Get(),
                    &desc,
                    xrSwapchain.imagesResourceView[slice][lastReleasedIndex].ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.imagesResourceView[slice][lastReleasedIndex].Get(),
                             fmt::format("Convert SRV[{}, {}, {}]", slice, lastReleasedIndex, (void*)&xrSwapchain));
            }

            // We are about to do something destructive to the application context. Save the context. It will be
            // restored at the end of xrEndFrame().
            if (m_d3d11Device == m_ovrSubmissionDevice && !m_d3d11ContextState) {
                m_ovrSubmissionContext->SwapDeviceContextState(m_ovrSubmissionContextState.Get(),
                                                               m_d3d11ContextState.ReleaseAndGetAddressOf());
            }

            // 0: shader for Tex2D, 1: shader for Tex2DArray.
            const int shaderToUse = xrSwapchain.xrDesc.arraySize == 1 ? 0 : 1;
            {
                AlphaBlendingCSConstants constants{};
                constants.ignoreAlpha = needClearAlpha;
                constants.isUnpremultipliedAlpha = needPremultiplyAlpha;

                D3D11_MAPPED_SUBRESOURCE mappedResources;
                CHECK_HRCMD(m_ovrSubmissionContext->Map(
                    xrSwapchain.convertConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                memcpy(mappedResources.pData, &constants, sizeof(constants));
                m_ovrSubmissionContext->Unmap(xrSwapchain.convertConstants.Get(), 0);
                m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, xrSwapchain.convertConstants.GetAddressOf());

                m_ovrSubmissionContext->CSSetShader(m_alphaCorrectShader[shaderToUse].Get(), nullptr, 0);
            }

            m_ovrSubmissionContext->CSSetShaderResources(
                0, 1, xrSwapchain.imagesResourceView[slice][lastReleasedIndex].GetAddressOf());
            m_ovrSubmissionContext->CSSetUnorderedAccessViews(
                0, 1, xrSwapchain.convertAccessView.GetAddressOf(), nullptr);

            m_ovrSubmissionContext->Dispatch((unsigned int)std::ceil(xrSwapchain.xrDesc.width / 32),
                                             (unsigned int)std::ceil(xrSwapchain.xrDesc.height / 32),
                                             1);

            // Unbind all resources to avoid D3D validation errors.
            {
                m_ovrSubmissionContext->CSSetShader(nullptr, nullptr, 0);
                ID3D11Buffer* nullCBV[] = {nullptr};
                m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, nullCBV);
                ID3D11UnorderedAccessView* nullUAV[] = {nullptr};
                m_ovrSubmissionContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
                ID3D11ShaderResourceView* nullSRV[] = {nullptr};
                m_ovrSubmissionContext->CSSetShaderResources(0, 1, nullSRV);
            }

            // Final copy into the OVR texture.
            if (!isSRGBFormat(xrSwapchain.dxgiFormatForSubmission)) {
                m_ovrSubmissionContext->CopySubresourceRegion(
                    xrSwapchain.slices[slice][ovrDestIndex].Get(), 0, 0, 0, 0, xrSwapchain.resolved.Get(), 0, nullptr);
            } else {
                // Lazily create RTV.
                if (!xrSwapchain.renderTargetView[slice][ovrDestIndex]) {
                    D3D11_RENDER_TARGET_VIEW_DESC desc{};

                    // When rendering to a swapchain with slice > 0, we know the swapchain is always arraySize of 1.
                    desc.ViewDimension = (xrSwapchain.xrDesc.arraySize == 1) || slice > 0
                                             ? D3D11_RTV_DIMENSION_TEXTURE2D
                                             : D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc.Format = xrSwapchain.dxgiFormatForSubmission;
                    desc.Texture2DArray.ArraySize = 1;
                    desc.Texture2DArray.MipSlice = D3D11CalcSubresource(0, 0, xrSwapchain.xrDesc.mipCount);
                    desc.Texture2DArray.FirstArraySlice = slice;

                    CHECK_HRCMD(m_ovrSubmissionDevice->CreateRenderTargetView(
                        xrSwapchain.slices[slice][ovrDestIndex].Get(),
                        &desc,
                        xrSwapchain.renderTargetView[slice][ovrDestIndex].ReleaseAndGetAddressOf()));
                    setDebugName(xrSwapchain.renderTargetView[slice][lastReleasedIndex].Get(),
                                 fmt::format("Convert RTV[{}, {}, {}]", slice, ovrDestIndex, (void*)&xrSwapchain));
                }

                // Use a full quad shader for color conversion to sRGB.
                m_ovrSubmissionContext->ClearState();
                m_ovrSubmissionContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                m_ovrSubmissionContext->OMSetRenderTargets(
                    1, xrSwapchain.renderTargetView[slice][lastReleasedIndex].GetAddressOf(), nullptr);
                m_ovrSubmissionContext->RSSetState(m_noDepthRasterizer.Get());
                D3D11_VIEWPORT viewport{};
                viewport.Width = (float)xrSwapchain.ovrDesc.Width;
                viewport.Height = (float)xrSwapchain.ovrDesc.Height;
                viewport.MaxDepth = 1.f;
                m_ovrSubmissionContext->RSSetViewports(1, &viewport);
                m_ovrSubmissionContext->VSSetShader(m_fullQuadVS.Get(), nullptr, 0);
                m_ovrSubmissionContext->PSSetSamplers(0, 1, m_linearClampSampler.GetAddressOf());
                m_ovrSubmissionContext->PSSetShaderResources(0, 1, xrSwapchain.convertResourceView.GetAddressOf());
                m_ovrSubmissionContext->PSSetShader(m_colorConversionPS.Get(), nullptr, 0);
                m_ovrSubmissionContext->Draw(3, 0);

                // Unbind all resources to avoid D3D validation errors.
                {
                    ID3D11RenderTargetView* nullRTV[] = {nullptr};
                    m_ovrSubmissionContext->OMSetRenderTargets(1, nullRTV, nullptr);
                    ID3D11ShaderResourceView* nullSRV[] = {nullptr};
                    m_ovrSubmissionContext->PSSetShaderResources(0, 1, nullSRV);
                }
            }
        }

        xrSwapchain.lastProcessedIndex[slice] = lastReleasedIndex;

        // Commit the texture to OVR.
        CHECK_OVRCMD(ovr_CommitTextureSwapChain(m_ovrSession, xrSwapchain.ovrSwapchain[slice]));
        committed.insert(std::make_pair(xrSwapchain.ovrSwapchain[0], slice));
    }

    void OpenXrRuntime::ensureSwapchainSliceResources(Swapchain& xrSwapchain, uint32_t slice) const {
        // Ensure necessary resources for texture arrays: lazily create a second swapchain for this slice of the array.
        if (!xrSwapchain.ovrSwapchain[slice]) {
            auto desc = xrSwapchain.ovrDesc;

            // We might use a full quad shader to perform final color conversion.
            if (isSRGBFormat(xrSwapchain.dxgiFormatForSubmission)) {
                desc.BindFlags |= ovrTextureBind_DX_RenderTarget;
            }
            desc.ArraySize = 1;
            CHECK_OVRCMD(ovr_CreateTextureSwapChainDX(
                m_ovrSession, m_ovrSubmissionDevice.Get(), &desc, &xrSwapchain.ovrSwapchain[slice]));

            int count = -1;
            CHECK_OVRCMD(ovr_GetTextureSwapChainLength(m_ovrSession, xrSwapchain.ovrSwapchain[slice], &count));
            if (count != xrSwapchain.slices[0].size()) {
                throw std::runtime_error("Swapchain image count mismatch");
            }

            // Query the textures for the swapchain.
            for (int i = 0; i < count; i++) {
                ComPtr<ID3D11Texture2D> texture;
                CHECK_OVRCMD(ovr_GetTextureSwapChainBufferDX(
                    m_ovrSession, xrSwapchain.ovrSwapchain[slice], i, IID_PPV_ARGS(texture.ReleaseAndGetAddressOf())));
                setDebugName(texture.Get(),
                             fmt::format("Runtime Slice Texture[{}, {}, {}]", slice, i, (void*)&xrSwapchain));

                xrSwapchain.slices[slice].push_back(texture);
            }
        }
    }

    void OpenXrRuntime::ensureSwapchainIntermediateResources(Swapchain& xrSwapchain) const {
        // Lazily create our intermediate buffer and compute shader resources.
        if (!xrSwapchain.resolved) {
            const bool isSRGBDestination = isSRGBFormat(xrSwapchain.dxgiFormatForSubmission);

            {
                D3D11_TEXTURE2D_DESC desc{};
                desc.ArraySize = 1;
                if (!isSRGBDestination) {
                    desc.Format = getTypelessFormat(xrSwapchain.dxgiFormatForSubmission);
                } else {
                    // Use a non-SRGB format that has enough precision to avoid loss of colors.
                    desc.Format = DXGI_FORMAT_R16G16B16A16_TYPELESS;
                }
                desc.Width = xrSwapchain.xrDesc.width;
                desc.Height = xrSwapchain.xrDesc.height;
                desc.MipLevels = xrSwapchain.xrDesc.mipCount;
                desc.SampleDesc.Count = xrSwapchain.xrDesc.sampleCount;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

                CHECK_HRCMD(m_ovrSubmissionDevice->CreateTexture2D(
                    &desc, nullptr, xrSwapchain.resolved.ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.resolved.Get(), fmt::format("Resolved Texture[{}]", (void*)&xrSwapchain));
            }
            {
                D3D11_BUFFER_DESC desc{};
                desc.ByteWidth = 16; // Minimal size. We we only use 4 bytes.
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                CHECK_HRCMD(m_ovrSubmissionDevice->CreateBuffer(
                    &desc, nullptr, xrSwapchain.convertConstants.ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.convertConstants.Get(),
                             fmt::format("Convert Constants[{}]", (void*)&xrSwapchain));
            }
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};

                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                if (!isSRGBDestination) {
                    desc.Format = xrSwapchain.dxgiFormatForSubmission;
                } else {
                    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                }
                desc.Texture2D.MipSlice = 0;

                CHECK_HRCMD(m_ovrSubmissionDevice->CreateUnorderedAccessView(
                    xrSwapchain.resolved.Get(), &desc, xrSwapchain.convertAccessView.ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.convertAccessView.Get(), fmt::format("Convert UAV[{}]", (void*)&xrSwapchain));
            }
            if (isSRGBDestination) {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc{};

                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                // We only every use the SRV for color conversion when destination is SRGB.
                desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                desc.Texture2D.MipLevels = xrSwapchain.xrDesc.mipCount;
                desc.Texture2D.MostDetailedMip = D3D11CalcSubresource(0, 0, desc.Texture2DArray.MipLevels);

                CHECK_HRCMD(m_ovrSubmissionDevice->CreateShaderResourceView(
                    xrSwapchain.resolved.Get(), &desc, xrSwapchain.convertResourceView.ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.convertResourceView.Get(),
                             fmt::format("Convert SRV[{}]", (void*)&xrSwapchain));
            }
        }
    }

    // Flush any pending work in the app context.
    void OpenXrRuntime::flushD3D11Context() {
        if (m_d3d11Context && m_d3d11Fence) {
            wil::unique_handle eventHandle;
            m_fenceValue++;
            TraceLoggingWrite(
                g_traceProvider, "FlushContext_Wait", TLArg("D3D11", "Api"), TLArg(m_fenceValue, "FenceValue"));
            CHECK_HRCMD(m_d3d11Context->Signal(m_d3d11Fence.Get(), m_fenceValue));
            *eventHandle.put() = CreateEventEx(nullptr, L"Flush Fence", 0, EVENT_ALL_ACCESS);
            CHECK_HRCMD(m_d3d11Fence->SetEventOnCompletion(m_fenceValue, eventHandle.get()));
            WaitForSingleObject(eventHandle.get(), INFINITE);
            ResetEvent(eventHandle.get());
        }
    }

    // Flush any pending work in the submission context.
    void OpenXrRuntime::flushSubmissionContext() {
        if (m_ovrSubmissionContext && m_ovrSubmissionFence) {
            wil::unique_handle eventHandle;
            m_fenceValue++;
            TraceLoggingWrite(
                g_traceProvider, "FlushContext_Wait", TLArg("D3D11", "Api"), TLArg(m_fenceValue, "FenceValue"));
            CHECK_HRCMD(m_ovrSubmissionContext->Signal(m_ovrSubmissionFence.Get(), m_fenceValue));
            *eventHandle.put() = CreateEventEx(nullptr, L"Flush Fence", 0, EVENT_ALL_ACCESS);
            CHECK_HRCMD(m_ovrSubmissionFence->SetEventOnCompletion(m_fenceValue, eventHandle.get()));
            WaitForSingleObject(eventHandle.get(), INFINITE);
            ResetEvent(eventHandle.get());
        }
    }

    // Serialize commands from the D3D12 queue to the D3D11 context used by OVR.
    void OpenXrRuntime::serializeD3D11Frame() {
        if (m_ovrSubmissionDevice != m_d3d11Device) {
            m_fenceValue++;
            TraceLoggingWrite(
                g_traceProvider, "xrEndFrame_Sync", TLArg("D3D11", "Api"), TLArg(m_fenceValue, "FenceValue"));
            CHECK_HRCMD(m_d3d11Context->Signal(m_d3d11Fence.Get(), m_fenceValue));

            waitOnSubmissionDevice();
        }
    }

    void OpenXrRuntime::waitOnSubmissionDevice() {
        if (!m_syncGpuWorkInEndFrame) {
            CHECK_HRCMD(m_ovrSubmissionContext->Wait(m_ovrSubmissionFence.Get(), m_fenceValue));
        } else {
            CHECK_HRCMD(m_ovrSubmissionFence->SetEventOnCompletion(m_fenceValue, m_eventForSubmissionFence.get()));
            WaitForSingleObject(m_eventForSubmissionFence.get(), INFINITE);
            ResetEvent(m_eventForSubmissionFence.get());
        }
    }

} // namespace virtualdesktop_openxr
