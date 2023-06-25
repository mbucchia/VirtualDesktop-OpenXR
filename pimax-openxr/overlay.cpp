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

// Implements the in-VR overlay.

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    // Create overlay resources.
    void OpenXrRuntime::initializeOverlayResources() {
        HRESULT hr;

        // Load the background texture.
        auto image = std::make_unique<DirectX::ScratchImage>();
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        hr = DirectX::LoadFromWICFile((dllHome / L"overlay.png").c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image);

        if (SUCCEEDED(hr)) {
            hr = DirectX::CreateTexture(m_pvrSubmissionDevice.Get(),
                                        image->GetImages(),
                                        1,
                                        image->GetMetadata(),
                                        m_overlayBackground.ReleaseAndGetAddressOf());

            if (SUCCEEDED(hr)) {
                // Create a PVR swapchain for the overlay.
                pvrTextureSwapChainDesc desc{};
                desc.Type = pvrTexture_2D;
                desc.ArraySize = 1;
                desc.Width = m_overlayExtent.width = (int)image->GetMetadata().width;
                desc.Height = m_overlayExtent.height = (int)image->GetMetadata().height;
                desc.MipLevels = 1;
                desc.SampleCount = 1;
                m_overlaySwapchainFormat = image->GetMetadata().format;
                desc.Format = dxgiToPvrTextureFormat(m_overlaySwapchainFormat);
                desc.BindFlags = pvrTextureBind_DX_RenderTarget;

                CHECK_PVRCMD(pvr_createTextureSwapChainDX(
                    m_pvrSession, m_pvrSubmissionDevice.Get(), &desc, &m_overlaySwapchain));
            } else {
                ErrorLog("Failed to create texture from overlay.png: %X\n");
            }
        } else {
            ErrorLog("Failed to load overlay.png: %X\n");
        }
    }

    void OpenXrRuntime::refreshOverlay() {
        const std::time_t now = std::time(nullptr);
        if (now - m_lastOverlayRefresh < 1) {
            return;
        }
        m_lastOverlayRefresh = now;

        // Acquire the next image.
        int imageIndex = -1;
        CHECK_PVRCMD(pvr_getTextureSwapChainCurrentIndex(m_pvrSession, m_overlaySwapchain, &imageIndex));
        ComPtr<ID3D11Texture2D> swapchainTexture;
        CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
            m_pvrSession, m_overlaySwapchain, imageIndex, IID_PPV_ARGS(swapchainTexture.ReleaseAndGetAddressOf())));

        // We are about to do something destructive to the application context. Save the context. It will be
        // restored at the end of xrEndFrame().
        if (m_d3d11Device == m_pvrSubmissionDevice && !m_d3d11ContextState) {
            m_pvrSubmissionContext->SwapDeviceContextState(m_pvrSubmissionContextState.Get(),
                                                           m_d3d11ContextState.ReleaseAndGetAddressOf());
        }

        // Copy the background.
        m_pvrSubmissionContext->CopyResource(swapchainTexture.Get(), m_overlayBackground.Get());
        m_pvrSubmissionContext->Flush();

        // Draw the text.
        const auto getBatteryLevel = [&](pvrTrackedDeviceType device) -> std::wstring {
            const int batteryPercent =
                pvr_getTrackedDeviceIntProperty(m_pvrSession, device, pvrTrackedDeviceProp_BatteryPercent_int, -1);
            if (batteryPercent >= 0) {
                if (batteryPercent > 20) {
                    return fmt::format(L"{}%", batteryPercent);
                } else {
                    return fmt::format(L"{}%  \x26A0", batteryPercent);
                }
            } else {
                const int batteryLevel =
                    pvr_getTrackedDeviceIntProperty(m_pvrSession, device, pvrTrackedDeviceProp_BatteryLevel_int, -1);
                if (batteryLevel != pvrTrackedDeviceBateryLevel_NotSupport) {
                    switch (batteryLevel) {
                    case pvrTrackedDeviceBateryLevel_Low:
                        return L"Low \x26A0";
                    case pvrTrackedDeviceBateryLevel_Middle:
                        return L"Medium";
                    case pvrTrackedDeviceBateryLevel_High:
                        return L"High";
                    }
                }
            }
            return L"???";
        };

        m_pvrSubmissionContext->ClearState();

        ComPtr<ID3D11RenderTargetView> rtv;
        {
            D3D11_RENDER_TARGET_VIEW_DESC desc{};
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            desc.Format = m_overlaySwapchainFormat;
            CHECK_HRCMD(m_pvrSubmissionDevice->CreateRenderTargetView(
                swapchainTexture.Get(), &desc, rtv.ReleaseAndGetAddressOf()));
        }
        m_pvrSubmissionContext->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
        {
            D3D11_VIEWPORT viewport{};
            viewport.Width = (float)m_overlayExtent.width;
            viewport.Height = (float)m_overlayExtent.height;
            viewport.MaxDepth = 1;
            m_pvrSubmissionContext->RSSetViewports(1, &viewport);
        }

        const uint32_t color = 0xffffffff;

        {
            char buf[8]{};
            std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&now));

            m_fontNormal->DrawString(m_pvrSubmissionContext.Get(),
                                     std::wstring(buf, buf + strlen(buf)).c_str(),
                                     200,
                                     600,
                                     12,
                                     color,
                                     FW1_LEFT | FW1_NOFLUSH);
        }

        m_fontNormal->DrawString(m_pvrSubmissionContext.Get(),
                                 getBatteryLevel(pvrTrackedDevice_HMD).c_str(),
                                 150,
                                 726,
                                 744,
                                 color,
                                 FW1_CENTER | FW1_NOFLUSH);

        for (uint32_t side = 0; side < 2; side++) {
            m_fontNormal->DrawString(
                m_pvrSubmissionContext.Get(),
                m_isControllerActive[side]
                    ? getBatteryLevel(side == 0 ? pvrTrackedDevice_LeftController : pvrTrackedDevice_RightController)
                          .c_str()
                    : L"-",
                150,
                side == 0 ? 204 : 1278,
                744,
                color,
                FW1_CENTER | FW1_NOFLUSH);
        }

        const int fps = m_frameTimes.size();
        m_fontNormal->DrawString(m_pvrSubmissionContext.Get(),
                                 fmt::format(L"{}", fps).c_str(),
                                 150,
                                 1400,
                                 1098,
                                 color,
                                 FW1_RIGHT | FW1_NOFLUSH);

        m_fontNormal->DrawString(m_pvrSubmissionContext.Get(),
                                 m_isSmartSmoothingEnabled ? (m_isSmartSmoothingActive ? L"Active" : L"Standby")
                                                           : L"Off",
                                 150,
                                 1400,
                                 1402,
                                 color,
                                 FW1_RIGHT | FW1_NOFLUSH);

        m_fontNormal->DrawString(m_pvrSubmissionContext.Get(),
                                 fmt::format(L"{}x{}", m_proj0Extent.width, m_proj0Extent.height).c_str(),
                                 150,
                                 1400,
                                 1754,
                                 color,
                                 FW1_RIGHT | FW1_NOFLUSH);

        m_fontNormal->Flush(m_pvrSubmissionContext.Get());

        CHECK_PVRCMD(pvr_commitTextureSwapChain(m_pvrSession, m_overlaySwapchain));
    }

} // namespace pimax_openxr
