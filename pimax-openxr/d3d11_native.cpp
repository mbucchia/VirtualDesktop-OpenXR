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

// Implements native support to submit swapchains to PVR.
// Implements the necessary support for the XR_KHR_D3D11_enable extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_D3D11_enable

namespace {
    // Compute shaders for converting D32_S8 to D32 depth formats.
    const std::string_view ResolveShaderHlsl[] = {
        R"_(
Texture2D in_texture : register(t0);
RWTexture2D<float> out_texture : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 pos : SV_DispatchThreadID)
{
    // Only keep the depth component.
    out_texture[pos] = in_texture[pos].x;
}
    )_",
        R"_(
Texture2DArray in_texture : register(t0);
RWTexture2D<float> out_texture : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 pos : SV_DispatchThreadID)
{
    // Only keep the depth component.
    out_texture[pos] = in_texture[float3(pos, 0)].x;
}
    )_"};
} // namespace

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

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

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (!m_isD3D11Supported) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        // Get the display device LUID.
        fillDisplayDeviceInfo();

        memcpy(&graphicsRequirements->adapterLuid, &m_adapterLuid, sizeof(LUID));
        graphicsRequirements->minFeatureLevel = D3D_FEATURE_LEVEL_11_1;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetD3D11GraphicsRequirementsKHR",
                          TraceLoggingCharArray((char*)&graphicsRequirements->adapterLuid, sizeof(LUID), "AdapterLuid"),
                          TLArg((int)graphicsRequirements->minFeatureLevel, "MinFeatureLevel"));

        m_graphicsRequirementQueried = true;

        return XR_SUCCESS;
    }

    // Initialize all the resources needed for D3D11 support, both on the API frontend and also the runtime/PVR backend.
    XrResult OpenXrRuntime::initializeD3D11(const XrGraphicsBindingD3D11KHR& d3dBindings, bool interop) {
        // Check that this is the correct adapter for the HMD.
        ComPtr<IDXGIDevice> dxgiDevice;
        CHECK_HRCMD(d3dBindings.device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter> dxgiAdapter;
        CHECK_HRCMD(dxgiDevice->GetAdapter(dxgiAdapter.ReleaseAndGetAddressOf()));

        DXGI_ADAPTER_DESC desc;
        CHECK_HRCMD(dxgiAdapter->GetDesc(&desc));

        if (!interop) {
            std::string deviceName;
            const std::wstring wadapterDescription(desc.Description);
            std::transform(wadapterDescription.begin(),
                           wadapterDescription.end(),
                           std::back_inserter(deviceName),
                           [](wchar_t c) { return (char)c; });

            TraceLoggingWrite(
                g_traceProvider, "xrCreateSession", TLArg("D3D11", "Api"), TLArg(deviceName.c_str(), "AdapterName"));
            Log("Using Direct3D 11 on adapter: %s\n", deviceName.c_str());
        }

        if (memcmp(&desc.AdapterLuid, &m_adapterLuid, sizeof(LUID))) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        ComPtr<ID3D11DeviceContext> deviceContext;
        d3dBindings.device->GetImmediateContext(deviceContext.ReleaseAndGetAddressOf());

        // Query the necessary flavors of device & device context, which will let use use fences. We only really
        // need those for D3D12 support, but using the same flavor keeps the code common.
        CHECK_HRCMD(d3dBindings.device->QueryInterface(m_d3d11Device.ReleaseAndGetAddressOf()));
        CHECK_HRCMD(deviceContext->QueryInterface(m_d3d11DeviceContext.ReleaseAndGetAddressOf()));

        // Create the resources for depth resolve.
        for (int i = 0; i < ARRAYSIZE(m_resolveShader); i++) {
            ComPtr<ID3DBlob> shaderBytes;
            ComPtr<ID3DBlob> errMsgs;
            DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;

#ifdef _DEBUG
            flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

            HRESULT hr = D3DCompile(ResolveShaderHlsl[i].data(),
                                    ResolveShaderHlsl[i].size(),
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    "main",
                                    "cs_5_0",
                                    flags,
                                    0,
                                    shaderBytes.ReleaseAndGetAddressOf(),
                                    errMsgs.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                std::string errMsg((const char*)errMsgs->GetBufferPointer(), errMsgs->GetBufferSize());
                Log("D3DCompile failed %X: %s", hr, errMsg.c_str());
                CHECK_HRESULT(hr, "D3DCompile failed");
            }
            CHECK_HRCMD(m_d3d11Device->CreateComputeShader(shaderBytes->GetBufferPointer(),
                                                           shaderBytes->GetBufferSize(),
                                                           nullptr,
                                                           m_resolveShader[i].ReleaseAndGetAddressOf()));
            setDebugName(m_resolveShader[i].Get(), "DepthResolve CS");
        }

        // If RenderDoc is loaded, then create a DXGI swapchain to signal events. Otherwise RenderDoc will
        // not see our OpenXR frames.
        if (GetModuleHandleA("renderdoc.dll")) {
            DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
            swapchainDesc.Width = 8;
            swapchainDesc.Height = 8;
            swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapchainDesc.SampleDesc.Count = 1;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.BufferCount = 3;
            swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

            ComPtr<IDXGIFactory2> dxgiFactory;
            CHECK_HRCMD(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
            CHECK_HRCMD(dxgiFactory->CreateSwapChainForComposition(
                dxgiDevice.Get(), &swapchainDesc, nullptr, m_dxgiSwapchain.ReleaseAndGetAddressOf()));
        }

        return XR_SUCCESS;
    }

    void OpenXrRuntime::cleanupD3D11() {
        // Flush any pending work.
        wil::unique_handle eventHandle;
        m_d3d11DeviceContext->Flush1(D3D11_CONTEXT_TYPE_ALL, eventHandle.get());
        WaitForSingleObject(eventHandle.get(), INFINITE);

        m_dxgiSwapchain.Reset();
        for (int i = 0; i < ARRAYSIZE(m_resolveShader); i++) {
            m_resolveShader[i].Reset();
        }
        m_d3d11DeviceContext.Reset();
        m_d3d11Device.Reset();
    }

    // Retrieve the swapchain images (ID3D11Texture2D) for the application to use.
    XrResult OpenXrRuntime::getSwapchainImagesD3D11(Swapchain& xrSwapchain,
                                                    XrSwapchainImageD3D11KHR* d3d11Images,
                                                    uint32_t count,
                                                    bool interop) {
        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();

        // PVR does not properly support certain depth format, and we will need an intermediate texture for the
        // app to use, then perform additional conversion steps during xrEndFrame().
        D3D11_TEXTURE2D_DESC desc{};
        if (!initialized && xrSwapchain.needDepthResolve) {
            // FIXME: Today we only do resolve for D32_FLOAT_S8X24 to D32_FLOAT, so we hard-code the
            // corresponding formats below.

            desc.ArraySize = xrSwapchain.xrDesc.arraySize;
            desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
            desc.Width = xrSwapchain.xrDesc.width;
            desc.Height = xrSwapchain.xrDesc.height;
            desc.MipLevels = xrSwapchain.xrDesc.mipCount;
            desc.SampleDesc.Count = xrSwapchain.xrDesc.sampleCount;

            // PVR does not support creating a depth texture with the RTV/UAV capability. We must use another
            // intermediate texture to run our shader.
            D3D11_TEXTURE2D_DESC resolvedDesc = desc;
            resolvedDesc.ArraySize = 1;
            resolvedDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            resolvedDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            CHECK_HRCMD(
                m_d3d11Device->CreateTexture2D(&resolvedDesc, nullptr, xrSwapchain.resolved.ReleaseAndGetAddressOf()));
            setDebugName(xrSwapchain.resolved.Get(), fmt::format("DepthResolve Texture[{}]", (void*)&xrSwapchain));

            // The texture will be sampled by our resolve shader.
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

            if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) {
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            }
            if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }
            if (xrSwapchain.xrDesc.usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
                desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            // Make the texture shareable in case the application needs to share it and since we need to support
            // D3D12 interop.
            // We don't use NT handles since they are less permissive in terms of formats.
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        }

        auto traceTexture = [](ID3D11Texture2D* texture, const char* type) {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainImages",
                              TLArg("D3D11", "Api"),
                              TLArg(type, "Type"),
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
        };

        // Query the textures for the swapchain.
        for (uint32_t i = 0; i < count; i++) {
            if (d3d11Images[i].type != XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (!initialized) {
                ID3D11Texture2D* swapchainTexture;
                CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
                    m_pvrSession, xrSwapchain.pvrSwapchain[0], i, IID_PPV_ARGS(&swapchainTexture)));
                setDebugName(swapchainTexture, fmt::format("Runtime Texture[{}, {}]", i, (void*)&xrSwapchain));

                xrSwapchain.slices[0].push_back(swapchainTexture);
                if (i == 0) {
                    traceTexture(swapchainTexture, "PVR");
                }

                if (xrSwapchain.needDepthResolve) {
                    // Create the intermediate texture if needed.
                    ComPtr<ID3D11Texture2D> intermediateTexture;
                    CHECK_HRCMD(
                        m_d3d11Device->CreateTexture2D(&desc, nullptr, intermediateTexture.ReleaseAndGetAddressOf()));
                    setDebugName(intermediateTexture.Get(), fmt::format("App Texture[{}, {}]", i, (void*)&xrSwapchain));

                    xrSwapchain.images.push_back(intermediateTexture);
                    for (uint32_t i = 0; i < xrSwapchain.xrDesc.arraySize; i++) {
                        xrSwapchain.imagesResourceView[i].push_back({});
                    }
                }
            }

            d3d11Images[i].texture =
                !xrSwapchain.needDepthResolve ? xrSwapchain.slices[0][i] : xrSwapchain.images[i].Get();

            if (!interop) {
                if (i == 0) {
                    traceTexture(d3d11Images[0].texture, "Runtime");
                }

                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateSwapchainImages",
                                  TLArg("D3D11", "Api"),
                                  TLPArg(d3d11Images[i].texture, "Texture"));
            }
        }

        return XR_SUCCESS;
    }

    // Prepare a PVR swapchain to be used by PVR.
    void OpenXrRuntime::prepareAndCommitSwapchainImage(
        Swapchain& xrSwapchain, uint32_t slice, std::set<std::pair<pvrTextureSwapChain, uint32_t>>& committed) const {
        // If the texture was already committed, do nothing.
        if (committed.count(std::make_pair(xrSwapchain.pvrSwapchain[0], slice))) {
            return;
        }

        // Circumvent some of PVR's limitations:
        // - For texture arrays, we must do a copy to slice 0 into another swapchain.
        // - For unsupported depth format, we must do a conversion.
        // For unsupported depth formats with texture arrays, we must do both!
        if (slice > 0 || xrSwapchain.needDepthResolve) {
            // Lazily create a second swapchain for this slice of the array.
            if (!xrSwapchain.pvrSwapchain[slice]) {
                auto desc = xrSwapchain.pvrDesc;
                desc.ArraySize = 1;
                CHECK_PVRCMD(pvr_createTextureSwapChainDX(
                    m_pvrSession, m_d3d11Device.Get(), &desc, &xrSwapchain.pvrSwapchain[slice]));

                int count = -1;
                CHECK_PVRCMD(pvr_getTextureSwapChainLength(m_pvrSession, xrSwapchain.pvrSwapchain[slice], &count));
                if (count != xrSwapchain.slices[0].size()) {
                    throw std::runtime_error("Swapchain image count mismatch");
                }

                // Query the textures for the swapchain.
                for (int i = 0; i < count; i++) {
                    ID3D11Texture2D* texture = nullptr;
                    CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
                        m_pvrSession, xrSwapchain.pvrSwapchain[slice], i, IID_PPV_ARGS(&texture)));
                    setDebugName(texture,
                                 fmt::format("Runtime Sliced Texture[{}, {}, {}]", slice, i, (void*)&xrSwapchain));

                    xrSwapchain.slices[slice].push_back(texture);
                }
            }

            // Copy or convert into the PVR swapchain.
            int pvrDestIndex = -1;
            CHECK_PVRCMD(
                pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain.pvrSwapchain[slice], &pvrDestIndex));

            if (!xrSwapchain.needDepthResolve) {
                int pvrSourceIndex = -1;
                CHECK_PVRCMD(
                    pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain.pvrSwapchain[0], &pvrSourceIndex));

                m_d3d11DeviceContext->CopySubresourceRegion(xrSwapchain.slices[slice][pvrDestIndex],
                                                            0,
                                                            0,
                                                            0,
                                                            0,
                                                            xrSwapchain.slices[0][pvrSourceIndex],
                                                            slice,
                                                            nullptr);
            } else {
                // FIXME: Today we only do resolve for D32_FLOAT_S8X24 to D32_FLOAT, so we hard-code the
                // corresponding formats below.

                // Lazily create SRV/UAV.
                if (!xrSwapchain.imagesResourceView[slice][xrSwapchain.currentIndex]) {
                    D3D11_SHADER_RESOURCE_VIEW_DESC desc{};

                    desc.ViewDimension = xrSwapchain.xrDesc.arraySize == 1 ? D3D11_SRV_DIMENSION_TEXTURE2D
                                                                           : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                    desc.Texture2DArray.ArraySize = 1;
                    desc.Texture2DArray.MipLevels = xrSwapchain.xrDesc.mipCount;
                    desc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, slice, desc.Texture2DArray.MipLevels);

                    CHECK_HRCMD(m_d3d11Device->CreateShaderResourceView(
                        xrSwapchain.images[xrSwapchain.currentIndex].Get(),
                        &desc,
                        xrSwapchain.imagesResourceView[slice][xrSwapchain.currentIndex].ReleaseAndGetAddressOf()));
                    setDebugName(
                        xrSwapchain.imagesResourceView[slice][xrSwapchain.currentIndex].Get(),
                        fmt::format(
                            "DepthResolve SRV[{}, {}, {}]", slice, xrSwapchain.currentIndex, (void*)&xrSwapchain));
                }
                if (!xrSwapchain.resolvedAccessView) {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};

                    desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    desc.Format = DXGI_FORMAT_R32_FLOAT;
                    desc.Texture2D.MipSlice = 0;

                    CHECK_HRCMD(m_d3d11Device->CreateUnorderedAccessView(
                        xrSwapchain.resolved.Get(), &desc, xrSwapchain.resolvedAccessView.ReleaseAndGetAddressOf()));
                    setDebugName(xrSwapchain.resolvedAccessView.Get(),
                                 fmt::format("DepthResolve UAV[{}]", (void*)&xrSwapchain));
                }

                // 0: shader for Tex2D, 1: shader for Tex2DArray.
                const int shaderToUse = xrSwapchain.xrDesc.arraySize == 1 ? 0 : 1;
                m_d3d11DeviceContext->CSSetShader(m_resolveShader[shaderToUse].Get(), nullptr, 0);

                m_d3d11DeviceContext->CSSetShaderResources(
                    0, 1, xrSwapchain.imagesResourceView[slice][xrSwapchain.currentIndex].GetAddressOf());
                m_d3d11DeviceContext->CSSetUnorderedAccessViews(
                    0, 1, xrSwapchain.resolvedAccessView.GetAddressOf(), nullptr);

                m_d3d11DeviceContext->Dispatch((unsigned int)std::ceil(xrSwapchain.xrDesc.width / 8),
                                               (unsigned int)std::ceil(xrSwapchain.xrDesc.height / 8),
                                               1);

                // Unbind all resources to avoid D3D validation errors.
                m_d3d11DeviceContext->CSSetShader(nullptr, nullptr, 0);
                ID3D11UnorderedAccessView* nullUAV[] = {nullptr};
                m_d3d11DeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
                ID3D11ShaderResourceView* nullSRV[] = {nullptr};
                m_d3d11DeviceContext->CSSetShaderResources(0, 1, nullSRV);

                // Final copy into the PVR texture.
                m_d3d11DeviceContext->CopySubresourceRegion(
                    xrSwapchain.slices[slice][pvrDestIndex], 0, 0, 0, 0, xrSwapchain.resolved.Get(), 0, nullptr);
            }
        }

        // Commit the texture to PVR.
        CHECK_PVRCMD(pvr_commitTextureSwapChain(m_pvrSession, xrSwapchain.pvrSwapchain[slice]));
        committed.insert(std::make_pair(xrSwapchain.pvrSwapchain[0], slice));
    }

} // namespace pimax_openxr
