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

#include "SharpeningCS.h"

#define A_CPU
#include <ffx_a.h>
#include <ffx_cas.h>

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    struct SharpenCSConstants {
        alignas(8) XrOffset2Di topLeft;
        alignas(4) bool isSRGB;
        alignas(4) uint32_t padding;
        alignas(16) uint32_t const0[4];
        alignas(16) uint32_t const1[4];
    };

    void OpenXrRuntime::upscaler(Swapchain** swapchains, const XrSwapchainSubImage** subImages, ovrLayerEyeFov& layer) {
        // We will store our stereo projection in the left eye swapchain.
        Swapchain& xrSwapchain = *swapchains[xr::StereoView::Left];
        // TODO: Control output size here. This is needed because we are fovMutable.
        ovrSizei resolution = m_cachedProjectionResolution;
        ensureSwapchainPrecompositorResources(xrSwapchain);

        // We are about to do something destructive to the application context. Save the context. It will be
        // restored at the end of xrEndFrame().
        if (m_d3d11Device == m_ovrSubmissionDevice && !m_d3d11ContextState) {
            m_ovrSubmissionContext->SwapDeviceContextState(m_ovrSubmissionContextState.Get(),
                                                           m_d3d11ContextState.ReleaseAndGetAddressOf());
        }

        m_ovrSubmissionContext->CSSetShader(m_sharpenShader.Get(), nullptr, 0);
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            int imageIndex = 0;
            CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(
                m_ovrSession, xrSwapchain.stereoProjection[eye].ovrSwapchain, &imageIndex));
            {
                SharpenCSConstants constants{};
                constants.topLeft = subImages[eye]->imageRect.offset;
                constants.isSRGB = isSRGBFormat((DXGI_FORMAT)swapchains[eye]->xrDesc.format);

                CasSetup(constants.const0,
                         constants.const1,
                         std::clamp(m_sharpenFactor, 0.f, 1.f),
                         (AF1)subImages[eye]->imageRect.extent.width,
                         (AF1)subImages[eye]->imageRect.extent.height,
                         (AF1)resolution.w,
                         (AF1)resolution.h);

                D3D11_MAPPED_SUBRESOURCE mappedResources;
                CHECK_HRCMD(m_ovrSubmissionContext->Map(
                    m_sharpenConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                memcpy(mappedResources.pData, &constants, sizeof(constants));
                m_ovrSubmissionContext->Unmap(m_sharpenConstants.Get(), 0);
                m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, m_sharpenConstants.GetAddressOf());
            }
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
            m_ovrSubmissionContext->CSSetShaderResources(0, 1, slice.srvs[slice.lastCommittedIndex].GetAddressOf());
            m_ovrSubmissionContext->CSSetUnorderedAccessViews(
                0, 1, xrSwapchain.stereoProjection[eye].uavs[imageIndex].GetAddressOf(), nullptr);

            const uint32_t blockWidth = 16;
            const uint32_t blockHeight = 16;
            m_ovrSubmissionContext->Dispatch(
                ((subImages[eye]->imageRect.extent.width + blockWidth - 1) / blockWidth),
                ((subImages[eye]->imageRect.extent.height + blockHeight - 1) / blockHeight),
                1);

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
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = ((sizeof(SharpenCSConstants) + 15) / 16) * 16;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            CHECK_HRCMD(
                m_ovrSubmissionDevice->CreateBuffer(&desc, nullptr, m_sharpenConstants.ReleaseAndGetAddressOf()));
            setDebugName(m_sharpenConstants.Get(), "Sharpen Constants");
        }
    }

} // namespace virtualdesktop_openxr
