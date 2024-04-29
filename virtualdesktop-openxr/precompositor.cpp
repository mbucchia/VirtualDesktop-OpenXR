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

#include "pch.h"

#include "log.h"
#include "runtime.h"
#include "utils.h"

#include "UpscalingCS.h"
#include "SharpeningCS.h"

#define A_CPU
#include <ffx_a.h>
#include <ffx_cas.h>
#include <ffx_fsr1.h>

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    struct UpscaleCSConstants {
        alignas(8) XrOffset2Df topLeftNormalized;
        alignas(4) bool isSRGB;
        alignas(4) uint32_t padding;
        alignas(16) uint32_t const0[4];
        alignas(16) uint32_t const1[4];
        alignas(16) uint32_t const2[4];
        alignas(16) uint32_t const3[4];
    };

    struct SharpenCSConstants {
        alignas(8) XrOffset2Di topLeft;
        alignas(4) bool isSRGB;
        alignas(4) uint32_t padding;
        alignas(16) uint32_t const0[4];
        alignas(16) uint32_t const1[4];
    };

    void OpenXrRuntime::upscaler(Swapchain** swapchains, const XrSwapchainSubImage** subImages, ovrLayerEyeFov& layer) {
        const bool upscaling = std::abs(m_upscalingMultiplier - 1.f) > FLT_EPSILON;
        const bool sharpening = m_sharpenFactor > 0.f;

        // We will store our stereo projection in the left eye swapchain.
        Swapchain& xrSwapchain = *swapchains[xr::StereoView::Left];
        ovrSizei resolution = ovrSizei{
            (int)xr::math::AlignTo<4>((uint32_t)(subImages[0]->imageRect.extent.width / m_upscalingMultiplier)),
            (int)xr::math::AlignTo<4>((uint32_t)(subImages[0]->imageRect.extent.height / m_upscalingMultiplier))};
        ensureSwapchainPrecompositorResources(xrSwapchain, resolution);
        if (upscaling && sharpening) {
            for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                ovrSizei currentResolution{};
                if (xrSwapchain.intermediate[eye].image) {
                    D3D11_TEXTURE2D_DESC desc{};
                    xrSwapchain.intermediate[eye].image->GetDesc(&desc);
                    currentResolution.w = desc.Width;
                    currentResolution.h = desc.Height;
                }
                if (!xrSwapchain.intermediate[eye].image || currentResolution.w != resolution.w ||
                    currentResolution.h != resolution.h) {
                    {
                        D3D11_TEXTURE2D_DESC desc{};
                        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                        desc.Width = resolution.w;
                        desc.Height = resolution.h;
                        desc.ArraySize = 1;
                        desc.MipLevels = 1;
                        desc.SampleDesc.Count = 1;
                        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
                        CHECK_HRCMD(m_ovrSubmissionDevice->CreateTexture2D(
                            &desc, nullptr, xrSwapchain.intermediate[eye].image.ReleaseAndGetAddressOf()));
                        setDebugName(
                            xrSwapchain.intermediate[eye].uav.Get(),
                            fmt::format("Precompositor Intermediate Texture [{}, {}]", eye, (void*)&xrSwapchain));
                    }
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                        desc.Texture2D.MipLevels = -1;
                        CHECK_HRCMD(m_ovrSubmissionDevice->CreateShaderResourceView(
                            xrSwapchain.intermediate[eye].image.Get(),
                            &desc,
                            xrSwapchain.intermediate[eye].srv.ReleaseAndGetAddressOf()));
                        setDebugName(xrSwapchain.intermediate[eye].srv.Get(),
                                     fmt::format("Precompositor Intermediate SRV [{}, {}]", eye, (void*)&xrSwapchain));
                    }
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
                        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                        CHECK_HRCMD(m_ovrSubmissionDevice->CreateUnorderedAccessView(
                            xrSwapchain.intermediate[eye].image.Get(),
                            &desc,
                            xrSwapchain.intermediate[eye].uav.ReleaseAndGetAddressOf()));
                        setDebugName(xrSwapchain.intermediate[eye].uav.Get(),
                                     fmt::format("Precompositor Intermediate UAV [{}, {}]", eye, (void*)&xrSwapchain));
                    }
                }
            }
        }

        // We are about to do something destructive to the application context. Save the context. It will be
        // restored at the end of xrEndFrame().
        if (m_d3d11Device == m_ovrSubmissionDevice && !m_d3d11ContextState) {
            m_ovrSubmissionContext->SwapDeviceContextState(m_ovrSubmissionContextState.Get(),
                                                           m_d3d11ContextState.ReleaseAndGetAddressOf());
        }

        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            // Prepare swapchain input.
            auto& slice = swapchains[eye]->resolvedSlices[subImages[eye]->imageArrayIndex];
            if (slice.srvs.size() <= slice.lastCommittedIndex) {
                slice.srvs.resize(slice.lastCommittedIndex + 1);
            }
            if (!slice.srvs[slice.lastCommittedIndex]) {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                desc.Format = getShaderResourceViewFormat(xrSwapchain.dxgiFormatForSubmission);
                desc.Texture2D.MipLevels = -1;
                CHECK_HRCMD(m_ovrSubmissionDevice->CreateShaderResourceView(
                    slice.images[slice.lastCommittedIndex].Get(),
                    &desc,
                    slice.srvs[slice.lastCommittedIndex].ReleaseAndGetAddressOf()));
                setDebugName(slice.srvs[slice.lastCommittedIndex].Get(),
                             fmt::format("Runtime Slice Copy SRV[{}, {}, {}]",
                                         subImages[eye]->imageArrayIndex,
                                         slice.lastCommittedIndex,
                                         (void*)swapchains[eye]));
            }

            // Prepare swapchain outputs.
            int imageIndex = 0;
            CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(
                m_ovrSession, xrSwapchain.stereoProjection[eye].ovrSwapchain, &imageIndex));

            if (upscaling) {
                m_ovrSubmissionContext->CSSetShader(m_upscaleShader.Get(), nullptr, 0);
                {
                    UpscaleCSConstants constants{};
                    constants.topLeftNormalized = {
                        (float)subImages[eye]->imageRect.offset.x / swapchains[eye]->ovrDesc.Width,
                        (float)subImages[eye]->imageRect.offset.y / swapchains[eye]->ovrDesc.Height};
                    // If we apply sharpening at the next stage, we use half-precision floats for the intermediate
                    // texture and we will do conversion to sRGB at the sharpening stage.
                    constants.isSRGB = !sharpening ? isSRGBFormat((DXGI_FORMAT)swapchains[eye]->xrDesc.format) : false;

                    FsrEasuCon(constants.const0,
                               constants.const1,
                               constants.const2,
                               constants.const3,
                               (AF1)subImages[eye]->imageRect.extent.width,
                               (AF1)subImages[eye]->imageRect.extent.height,
                               (AF1)swapchains[eye]->ovrDesc.Width,
                               (AF1)swapchains[eye]->ovrDesc.Height,
                               (AF1)resolution.w,
                               (AF1)resolution.h);

                    D3D11_MAPPED_SUBRESOURCE mappedResources;
                    CHECK_HRCMD(m_ovrSubmissionContext->Map(
                        m_upscalerConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                    memcpy(mappedResources.pData, &constants, sizeof(constants));
                    m_ovrSubmissionContext->Unmap(m_upscalerConstants.Get(), 0);
                    m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, m_upscalerConstants.GetAddressOf());
                }
                if (sharpening) {
                    m_ovrSubmissionContext->CSSetUnorderedAccessViews(
                        0, 1, xrSwapchain.intermediate[eye].uav.GetAddressOf(), nullptr);
                } else {
                    m_ovrSubmissionContext->CSSetUnorderedAccessViews(
                        0, 1, xrSwapchain.stereoProjection[eye].uavs[imageIndex].GetAddressOf(), nullptr);
                }
                m_ovrSubmissionContext->CSSetSamplers(0, 1, m_linearClampSampler.GetAddressOf());
                m_ovrSubmissionContext->CSSetShaderResources(0, 1, slice.srvs[slice.lastCommittedIndex].GetAddressOf());

                const uint32_t blockWidth = 16;
                const uint32_t blockHeight = 16;
                m_ovrSubmissionContext->Dispatch(((resolution.w + blockWidth - 1) / blockWidth),
                                                 ((resolution.h + blockHeight - 1) / blockHeight),
                                                 1);
            }

            if (sharpening) {
                m_ovrSubmissionContext->CSSetShader(m_sharpenShader.Get(), nullptr, 0);
                {
                    SharpenCSConstants constants{};
                    // If we upscaled at the previous stage, the image occupies the entire texture.
                    constants.topLeft = !upscaling ? subImages[eye]->imageRect.offset : XrOffset2Di{0, 0};
                    constants.isSRGB = isSRGBFormat((DXGI_FORMAT)swapchains[eye]->xrDesc.format);

                    CasSetup(constants.const0,
                             constants.const1,
                             std::clamp(m_sharpenFactor, 0.f, 1.f),
                             (AF1)resolution.w,
                             (AF1)resolution.h,
                             (AF1)resolution.w,
                             (AF1)resolution.h);

                    D3D11_MAPPED_SUBRESOURCE mappedResources;
                    CHECK_HRCMD(m_ovrSubmissionContext->Map(
                        m_upscalerConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                    memcpy(mappedResources.pData, &constants, sizeof(constants));
                    m_ovrSubmissionContext->Unmap(m_upscalerConstants.Get(), 0);
                    m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, m_upscalerConstants.GetAddressOf());
                }
                m_ovrSubmissionContext->CSSetUnorderedAccessViews(
                    0, 1, xrSwapchain.stereoProjection[eye].uavs[imageIndex].GetAddressOf(), nullptr);
                if (upscaling) {
                    m_ovrSubmissionContext->CSSetShaderResources(
                        0, 1, xrSwapchain.intermediate[eye].srv.GetAddressOf());
                } else {
                    m_ovrSubmissionContext->CSSetShaderResources(
                        0, 1, slice.srvs[slice.lastCommittedIndex].GetAddressOf());
                }

                const uint32_t blockWidth = 16;
                const uint32_t blockHeight = 16;
                m_ovrSubmissionContext->Dispatch(((resolution.w + blockWidth - 1) / blockWidth),
                                                 ((resolution.h + blockHeight - 1) / blockHeight),
                                                 1);
            }

            CHECK_OVRCMD(ovr_CommitTextureSwapChain(m_ovrSession, xrSwapchain.stereoProjection[eye].ovrSwapchain));

            // Patch the layer.
            layer.ColorTexture[eye] = xrSwapchain.stereoProjection[eye].ovrSwapchain;
            layer.Viewport[eye].Pos = {0, 0};
            layer.Viewport[eye].Size = resolution;
        }

        // Unbind all resources to avoid D3D validation errors.
        {
            m_ovrSubmissionContext->CSSetShader(nullptr, nullptr, 0);
            ID3D11Buffer* nullCBV[] = {nullptr};
            m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, nullCBV);
            ID3D11SamplerState* nullSampler[] = {nullptr};
            m_ovrSubmissionContext->CSSetSamplers(0, 1, nullSampler);
            ID3D11ShaderResourceView* nullSRV[] = {nullptr};
            m_ovrSubmissionContext->CSSetShaderResources(0, 1, nullSRV);
            ID3D11UnorderedAccessView* nullUAV[] = {nullptr};
            m_ovrSubmissionContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
        }
    }

    void OpenXrRuntime::initializePrecompositorResources() {
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateComputeShader(
            g_SharpeningCS, sizeof(g_SharpeningCS), nullptr, m_sharpenShader.ReleaseAndGetAddressOf()));
        setDebugName(m_sharpenShader.Get(), "Sharpen CS");
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateComputeShader(
            g_UpscalingCS, sizeof(g_UpscalingCS), nullptr, m_upscaleShader.ReleaseAndGetAddressOf()));
        setDebugName(m_sharpenShader.Get(), "Sharpen CS");
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = (UINT)((std::max(sizeof(SharpenCSConstants), sizeof(UpscaleCSConstants)) + 15) / 16) * 16;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            CHECK_HRCMD(
                m_ovrSubmissionDevice->CreateBuffer(&desc, nullptr, m_upscalerConstants.ReleaseAndGetAddressOf()));
            setDebugName(m_upscalerConstants.Get(), "Upscale/Sharpen Constants");
        }
    }

} // namespace virtualdesktop_openxr
