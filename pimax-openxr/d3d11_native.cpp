// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
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

    // Compute shaders for correcting alpha channel.
    // Clear the alpha channel or premultiply each component.
    const std::string_view AlphaCorrectShaderHlsl =
        R"_(
cbuffer config : register(b0) {
    int mode; // bit 0 = clear alpha, bit 1 = premultiply alpha.
};
Texture2D in_texture : register(t0);
Texture2DArray in_texture_array : register(t0);
RWTexture2D<float4> out_texture : register(u0);

float4 processAlpha(float4 input)
{
    float4 output = input;
    if (mode & 1) {
      output.a = 1;
    }
    if (mode & 2) {
      output.rgb = output.rgb * output.a;
    }
    return output;
}

[numthreads(8, 8, 1)]
void main(uint2 pos : SV_DispatchThreadID)
{
    out_texture[pos] = processAlpha(in_texture[pos]);
}

[numthreads(8, 8, 1)]
void mainForArray(uint2 pos : SV_DispatchThreadID)
{
    out_texture[pos] = processAlpha(in_texture_array[float3(pos, 0)]);
}
    )_";

    // Vertex and pixel shader for color conversion.
    const std::string_view FullQuadColorConversionShader = R"_(
void vsMain(in uint id : SV_VertexID, out float4 position : SV_POSITION, out float2 texcoord : TEXCOORD0)
{
    texcoord = float2((id == 1) ? 2.0 : 0.0, (id == 2) ? 2.0 : 0.0);
    position = float4(texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

SamplerState sourceSampler : register(s0);
Texture2D sourceTexture : register(t0);

float4 psMain(in float4 position : SV_POSITION, in float2 texcoord : TEXCOORD0) : SV_TARGET {
    return sourceTexture.Sample(sourceSampler, texcoord);
}
)_";

    DXGI_FORMAT getTypelessFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_TYPELESS;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_TYPELESS;
        }

        return format;
    }

    bool isSRGBFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return true;
        }

        return false;
    }

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

        if (!has_XR_KHR_D3D11_enable) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        // Get the display device LUID.
        fillDisplayDeviceInfo();

        memcpy(&graphicsRequirements->adapterLuid, &m_adapterLuid, sizeof(LUID));
        graphicsRequirements->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetD3D11GraphicsRequirementsKHR",
                          TraceLoggingCharArray((char*)&graphicsRequirements->adapterLuid, sizeof(LUID), "AdapterLuid"),
                          TLArg((int)graphicsRequirements->minFeatureLevel, "MinFeatureLevel"));

        m_graphicsRequirementQueried = true;

        return XR_SUCCESS;
    }

    // Initialize all the resources needed for D3D11 support, both on the API frontend and also the runtime/PVR backend.
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

        // Create the resources that PVR will be using.
        initializeSubmissionDevice("D3D11");

        // We will use a shared fence to synchronize between the application context and the PVR (submission) context
        // context.
        wil::unique_handle fenceHandle;
        CHECK_HRCMD(m_pvrSubmissionFence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, fenceHandle.put()));
        CHECK_HRCMD(
            m_d3d11Device->OpenSharedFence(fenceHandle.get(), IID_PPV_ARGS(m_d3d11Fence.ReleaseAndGetAddressOf())));

        // Frame timers.
        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerApp[i] = std::make_unique<D3D11GpuTimer>(m_d3d11Device.Get(), m_d3d11Context.Get());
        }

        return XR_SUCCESS;
    }

    // Initialize all the resources for the PVR backend.
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

        // Create the submission device that PVR will be using.
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
        CHECK_HRCMD(device->QueryInterface(m_pvrSubmissionDevice.ReleaseAndGetAddressOf()));
        CHECK_HRCMD(deviceContext->QueryInterface(m_pvrSubmissionContext.ReleaseAndGetAddressOf()));

        ComPtr<IDXGIDevice> dxgiDevice;
        CHECK_HRCMD(device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        // Create the synchronization fence to serialize work between the application device and submission device.
        CHECK_HRCMD(m_pvrSubmissionDevice->CreateFence(
            0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(m_pvrSubmissionFence.ReleaseAndGetAddressOf())));
        m_fenceValue = 0;

        // Create the resources for alpha correction.
        const auto compileShader = [&](const std::string_view& code,
                                       const std::string_view& type,
                                       const std::string_view& entry,
                                       ID3DBlob** shaderBytes) {
            ComPtr<ID3DBlob> errMsgs;
            DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
            flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

            const HRESULT hr = D3DCompile(code.data(),
                                          code.size(),
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          entry.data(),
                                          type.data(),
                                          flags,
                                          0,
                                          shaderBytes,
                                          errMsgs.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                std::string errMsg((const char*)errMsgs->GetBufferPointer(), errMsgs->GetBufferSize());
                ErrorLog("D3DCompile failed %X: %s\n", hr, errMsg.c_str());
                CHECK_HRESULT(hr, "D3DCompile failed");
            }
        };

        ComPtr<ID3DBlob> shaderBytes;
        {
            compileShader(AlphaCorrectShaderHlsl, "cs_5_0", "main", shaderBytes.ReleaseAndGetAddressOf());
            CHECK_HRCMD(m_pvrSubmissionDevice->CreateComputeShader(shaderBytes->GetBufferPointer(),
                                                                   shaderBytes->GetBufferSize(),
                                                                   nullptr,
                                                                   m_alphaCorrectShader[0].ReleaseAndGetAddressOf()));
            setDebugName(m_alphaCorrectShader[0].Get(), "AlphaCorrect CS");
        }
        {
            compileShader(AlphaCorrectShaderHlsl, "cs_5_0", "mainForArray", shaderBytes.ReleaseAndGetAddressOf());
            CHECK_HRCMD(m_pvrSubmissionDevice->CreateComputeShader(shaderBytes->GetBufferPointer(),
                                                                   shaderBytes->GetBufferSize(),
                                                                   nullptr,
                                                                   m_alphaCorrectShader[1].ReleaseAndGetAddressOf()));
            setDebugName(m_alphaCorrectShader[1].Get(), "AlphaCorrect CS");
        }
        {
            compileShader(FullQuadColorConversionShader, "vs_5_0", "vsMain", shaderBytes.ReleaseAndGetAddressOf());
            CHECK_HRCMD(m_pvrSubmissionDevice->CreateVertexShader(shaderBytes->GetBufferPointer(),
                                                                  shaderBytes->GetBufferSize(),
                                                                  nullptr,
                                                                  m_fullQuadVS.ReleaseAndGetAddressOf()));
            setDebugName(m_fullQuadVS.Get(), "FullQuad VS");
        }
        {
            compileShader(FullQuadColorConversionShader, "ps_5_0", "psMain", shaderBytes.ReleaseAndGetAddressOf());
            CHECK_HRCMD(m_pvrSubmissionDevice->CreatePixelShader(shaderBytes->GetBufferPointer(),
                                                                 shaderBytes->GetBufferSize(),
                                                                 nullptr,
                                                                 m_colorConversionPS.ReleaseAndGetAddressOf()));
            setDebugName(m_fullQuadVS.Get(), "ColorConversion PS");
        }

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
                m_pvrSubmissionDevice->CreateSamplerState(&desc, m_linearClampSampler.ReleaseAndGetAddressOf()));
        }
        {
            D3D11_RASTERIZER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            desc.FrontCounterClockwise = TRUE;
            CHECK_HRCMD(
                m_pvrSubmissionDevice->CreateRasterizerState(&desc, m_noDepthRasterizer.ReleaseAndGetAddressOf()));
        }

        for (uint32_t i = 0; i < k_numGpuTimers; i++) {
            m_gpuTimerPrecomposition[i] =
                std::make_unique<D3D11GpuTimer>(m_pvrSubmissionDevice.Get(), m_pvrSubmissionContext.Get());
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

        m_pvrSubmissionFence.Reset();
        m_pvrSubmissionContext.Reset();
        m_pvrSubmissionDevice.Reset();
    }

    // Retrieve generic handles to the swapchain images to import into the application device.
    std::vector<HANDLE> OpenXrRuntime::getSwapchainImages(Swapchain& xrSwapchain) {
        // Detect whether this is the first call for this swapchain.
        const bool initialized = !xrSwapchain.slices[0].empty();

        // Query the textures for the swapchain.
        std::vector<HANDLE> handles;
        for (int i = 0; i < xrSwapchain.pvrSwapchainLength; i++) {
            if (!initialized) {
                ID3D11Texture2D* swapchainTexture;
                CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
                    m_pvrSession, xrSwapchain.pvrSwapchain[0], i, IID_PPV_ARGS(&swapchainTexture)));
                setDebugName(swapchainTexture, fmt::format("PVR Swapchain Texture[{}, {}]", i, (void*)&xrSwapchain));

                xrSwapchain.slices[0].push_back(swapchainTexture);
                if (i == 0) {
                    D3D11_TEXTURE2D_DESC desc;
                    swapchainTexture->GetDesc(&desc);
                    TraceLoggingWrite(g_traceProvider,
                                      "xrEnumerateSwapchainImages",
                                      TLArg("D3D11", "Api"),
                                      TLArg("PVR", "Type"),
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
                // Create an imported texture on the application device.
                ComPtr<ID3D11Texture2D> d3d11Texture;
                CHECK_HRCMD(m_d3d11Device->OpenSharedResource(textureHandles[i],
                                                              IID_PPV_ARGS(d3d11Texture.ReleaseAndGetAddressOf())));
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

    // Prepare a PVR swapchain to be used by PVR.
    void
    OpenXrRuntime::prepareAndCommitSwapchainImage(Swapchain& xrSwapchain,
                                                  uint32_t layerIndex,
                                                  uint32_t slice,
                                                  XrCompositionLayerFlags compositionFlags,
                                                  std::set<std::pair<pvrTextureSwapChain, uint32_t>>& committed) const {
        // If the texture was never used or already committed, do nothing.
        if (xrSwapchain.slices[0].empty() || committed.count(std::make_pair(xrSwapchain.pvrSwapchain[0], slice))) {
            return;
        }

        ensureSwapchainSliceResources(xrSwapchain, slice);

        int pvrDestIndex = -1;
        CHECK_PVRCMD(pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain.pvrSwapchain[slice], &pvrDestIndex));
        const int lastReleasedIndex = xrSwapchain.lastReleasedIndex;

        const bool needClearAlpha =
            layerIndex > 0 && !(compositionFlags & XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT);
        const bool needPremultiplyAlpha = (compositionFlags & XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT);
        const bool needCopy = xrSwapchain.lastProcessedIndex[slice] == lastReleasedIndex ||
                              (slice > 0 && !(needClearAlpha || needPremultiplyAlpha));

        if (needCopy) {
            // Circumvent some of PVR's limitations:
            // - For texture arrays, we must do a copy to slice 0 into another swapchain.
            // - Committing into a swapchain automatically acquires the next image. When an app renders certain
            //   swapchains (eg: quad layers) at a lower frame rate, we must perform a copy to the current PVR swapchain
            //   image. All the processing needed (eg: alpha correction) was done during initial processing (the first
            //   time we saw the last released image), so no need to redo it.
            m_pvrSubmissionContext->CopySubresourceRegion(xrSwapchain.slices[slice][pvrDestIndex],
                                                          0,
                                                          0,
                                                          0,
                                                          0,
                                                          xrSwapchain.slices[0][lastReleasedIndex],
                                                          slice,
                                                          nullptr);
        } else if (needClearAlpha || needPremultiplyAlpha) {
            // Circumvent some of PVR's limitations:
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

                CHECK_HRCMD(m_pvrSubmissionDevice->CreateShaderResourceView(
                    xrSwapchain.images[lastReleasedIndex].Get(),
                    &desc,
                    xrSwapchain.imagesResourceView[slice][lastReleasedIndex].ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.imagesResourceView[slice][lastReleasedIndex].Get(),
                             fmt::format("Convert SRV[{}, {}, {}]", slice, lastReleasedIndex, (void*)&xrSwapchain));
            }

            // 0: shader for Tex2D, 1: shader for Tex2DArray.
            const int shaderToUse = xrSwapchain.xrDesc.arraySize == 1 ? 0 : 1;
            {
                D3D11_MAPPED_SUBRESOURCE mappedResources;
                CHECK_HRCMD(m_pvrSubmissionContext->Map(
                    xrSwapchain.convertConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                *(uint32_t*)mappedResources.pData = (needClearAlpha ? 1 : 0) | (needPremultiplyAlpha ? 2 : 0);
                m_pvrSubmissionContext->Unmap(xrSwapchain.convertConstants.Get(), 0);
                m_pvrSubmissionContext->CSSetConstantBuffers(0, 1, xrSwapchain.convertConstants.GetAddressOf());

                m_pvrSubmissionContext->CSSetShader(m_alphaCorrectShader[shaderToUse].Get(), nullptr, 0);
            }

            m_pvrSubmissionContext->CSSetShaderResources(
                0, 1, xrSwapchain.imagesResourceView[slice][lastReleasedIndex].GetAddressOf());
            m_pvrSubmissionContext->CSSetUnorderedAccessViews(
                0, 1, xrSwapchain.convertAccessView.GetAddressOf(), nullptr);

            m_pvrSubmissionContext->Dispatch((unsigned int)std::ceil(xrSwapchain.xrDesc.width / 8),
                                             (unsigned int)std::ceil(xrSwapchain.xrDesc.height / 8),
                                             1);

            // Unbind all resources to avoid D3D validation errors.
            {
                m_pvrSubmissionContext->CSSetShader(nullptr, nullptr, 0);
                ID3D11Buffer* nullCBV[] = {nullptr};
                m_pvrSubmissionContext->CSSetConstantBuffers(0, 1, nullCBV);
                ID3D11UnorderedAccessView* nullUAV[] = {nullptr};
                m_pvrSubmissionContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
                ID3D11ShaderResourceView* nullSRV[] = {nullptr};
                m_pvrSubmissionContext->CSSetShaderResources(0, 1, nullSRV);
            }

            // Final copy into the PVR texture.
            if (!isSRGBFormat(xrSwapchain.dxgiFormatForSubmission)) {
                m_pvrSubmissionContext->CopySubresourceRegion(
                    xrSwapchain.slices[slice][pvrDestIndex], 0, 0, 0, 0, xrSwapchain.resolved.Get(), 0, nullptr);
            } else {
                // Lazily create RTV.
                if (!xrSwapchain.renderTargetView[slice][pvrDestIndex]) {
                    D3D11_RENDER_TARGET_VIEW_DESC desc{};

                    // When rendering to a swapchain with slice > 0, we know the swapchain is always arraySize of 1.
                    desc.ViewDimension = (xrSwapchain.xrDesc.arraySize == 1) || slice > 0
                                             ? D3D11_RTV_DIMENSION_TEXTURE2D
                                             : D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc.Format = xrSwapchain.dxgiFormatForSubmission;
                    desc.Texture2DArray.ArraySize = 1;
                    desc.Texture2DArray.MipSlice = D3D11CalcSubresource(0, 0, xrSwapchain.xrDesc.mipCount);
                    desc.Texture2DArray.FirstArraySlice = slice;

                    CHECK_HRCMD(m_pvrSubmissionDevice->CreateRenderTargetView(
                        xrSwapchain.slices[slice][pvrDestIndex],
                        &desc,
                        xrSwapchain.renderTargetView[slice][pvrDestIndex].ReleaseAndGetAddressOf()));
                    setDebugName(xrSwapchain.renderTargetView[slice][lastReleasedIndex].Get(),
                                 fmt::format("Convert RTV[{}, {}, {}]", slice, pvrDestIndex, (void*)&xrSwapchain));
                }

                // Use a full quad shader for color conversion to sRGB.
                m_pvrSubmissionContext->ClearState();
                m_pvrSubmissionContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                m_pvrSubmissionContext->OMSetRenderTargets(
                    1, xrSwapchain.renderTargetView[slice][lastReleasedIndex].GetAddressOf(), nullptr);
                m_pvrSubmissionContext->RSSetState(m_noDepthRasterizer.Get());
                D3D11_VIEWPORT viewport{};
                viewport.Width = (float)xrSwapchain.pvrDesc.Width;
                viewport.Height = (float)xrSwapchain.pvrDesc.Height;
                viewport.MaxDepth = 1.f;
                m_pvrSubmissionContext->RSSetViewports(1, &viewport);
                m_pvrSubmissionContext->VSSetShader(m_fullQuadVS.Get(), nullptr, 0);
                m_pvrSubmissionContext->PSSetSamplers(0, 1, m_linearClampSampler.GetAddressOf());
                m_pvrSubmissionContext->PSSetShaderResources(0, 1, xrSwapchain.convertResourceView.GetAddressOf());
                m_pvrSubmissionContext->PSSetShader(m_colorConversionPS.Get(), nullptr, 0);
                m_pvrSubmissionContext->Draw(3, 0);

                // Unbind all resources to avoid D3D validation errors.
                {
                    ID3D11RenderTargetView* nullRTV[] = {nullptr};
                    m_pvrSubmissionContext->OMSetRenderTargets(1, nullRTV, nullptr);
                    ID3D11ShaderResourceView* nullSRV[] = {nullptr};
                    m_pvrSubmissionContext->PSSetShaderResources(0, 1, nullSRV);
                }
            }
        }

        xrSwapchain.lastProcessedIndex[slice] = lastReleasedIndex;

        // Commit the texture to PVR.
        CHECK_PVRCMD(pvr_commitTextureSwapChain(m_pvrSession, xrSwapchain.pvrSwapchain[slice]));
        committed.insert(std::make_pair(xrSwapchain.pvrSwapchain[0], slice));
    }

    void OpenXrRuntime::ensureSwapchainSliceResources(Swapchain& xrSwapchain, uint32_t slice) const {
        // Ensure necessary resources for texture arrays: lazily create a second swapchain for this slice of the array.
        if (!xrSwapchain.pvrSwapchain[slice]) {
            auto desc = xrSwapchain.pvrDesc;

            // We might use a full quad shader to perform final color conversion.
            if (isSRGBFormat(xrSwapchain.dxgiFormatForSubmission)) {
                desc.BindFlags |= pvrTextureBind_DX_RenderTarget;
            }
            desc.ArraySize = 1;
            CHECK_PVRCMD(pvr_createTextureSwapChainDX(
                m_pvrSession, m_pvrSubmissionDevice.Get(), &desc, &xrSwapchain.pvrSwapchain[slice]));

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
                setDebugName(texture, fmt::format("Runtime Sliced Texture[{}, {}, {}]", slice, i, (void*)&xrSwapchain));

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

                CHECK_HRCMD(m_pvrSubmissionDevice->CreateTexture2D(
                    &desc, nullptr, xrSwapchain.resolved.ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.resolved.Get(), fmt::format("Resolved Texture[{}]", (void*)&xrSwapchain));
            }
            {
                D3D11_BUFFER_DESC desc{};
                desc.ByteWidth = 16; // Minimal size. We we only use 4 bytes.
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                CHECK_HRCMD(m_pvrSubmissionDevice->CreateBuffer(
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

                CHECK_HRCMD(m_pvrSubmissionDevice->CreateUnorderedAccessView(
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

                CHECK_HRCMD(m_pvrSubmissionDevice->CreateShaderResourceView(
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
        wil::unique_handle eventHandle;
        m_fenceValue++;
        TraceLoggingWrite(
            g_traceProvider, "FlushContext_Wait", TLArg("D3D11", "Api"), TLArg(m_fenceValue, "FenceValue"));
        CHECK_HRCMD(m_pvrSubmissionContext->Signal(m_pvrSubmissionFence.Get(), m_fenceValue));
        *eventHandle.put() = CreateEventEx(nullptr, L"Flush Fence", 0, EVENT_ALL_ACCESS);
        CHECK_HRCMD(m_pvrSubmissionFence->SetEventOnCompletion(m_fenceValue, eventHandle.get()));
        WaitForSingleObject(eventHandle.get(), INFINITE);
        ResetEvent(eventHandle.get());
    }

    // Serialize commands from the D3D12 queue to the D3D11 context used by PVR.
    void OpenXrRuntime::serializeD3D11Frame() {
        m_fenceValue++;
        TraceLoggingWrite(g_traceProvider, "xrEndFrame_Sync", TLArg("D3D11", "Api"), TLArg(m_fenceValue, "FenceValue"));
        CHECK_HRCMD(m_d3d11Context->Signal(m_d3d11Fence.Get(), m_fenceValue));

        waitOnSubmissionDevice();
    }

    void OpenXrRuntime::waitOnSubmissionDevice() {
        CHECK_HRCMD(m_pvrSubmissionContext->Wait(m_pvrSubmissionFence.Get(), m_fenceValue));
    }

} // namespace pimax_openxr
