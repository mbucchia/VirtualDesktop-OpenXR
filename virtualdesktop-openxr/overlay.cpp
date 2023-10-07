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

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;

    // Create overlay resources.
    void OpenXrRuntime::initializeOverlayResources() {
        HRESULT hr;

        // Load the background texture.
        auto image = std::make_unique<DirectX::ScratchImage>();
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        hr = DirectX::LoadFromWICFile((dllHome / L"overlay.png").c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image);

        if (SUCCEEDED(hr)) {
            hr = DirectX::CreateTexture(m_ovrSubmissionDevice.Get(),
                                        image->GetImages(),
                                        1,
                                        image->GetMetadata(),
                                        m_overlayBackground.ReleaseAndGetAddressOf());

            if (SUCCEEDED(hr)) {
                // Create an OVR swapchain for the overlay.
                ovrTextureSwapChainDesc desc{};
                desc.Type = ovrTexture_2D;
                desc.ArraySize = 1;
                desc.Width = m_overlayExtent.width = (int)image->GetMetadata().width;
                desc.Height = m_overlayExtent.height = (int)image->GetMetadata().height;
                desc.MipLevels = 1;
                desc.SampleCount = 1;
                m_overlaySwapchainFormat = image->GetMetadata().format;
                desc.Format = dxgiToOvrTextureFormat(m_overlaySwapchainFormat);
                desc.BindFlags = ovrTextureBind_DX_RenderTarget;

                CHECK_OVRCMD(ovr_CreateTextureSwapChainDX(
                    m_ovrSession, m_ovrSubmissionDevice.Get(), &desc, &m_overlaySwapchain));
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
        CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(m_ovrSession, m_overlaySwapchain, &imageIndex));
        ComPtr<ID3D11Texture2D> swapchainTexture;
        CHECK_OVRCMD(ovr_GetTextureSwapChainBufferDX(
            m_ovrSession, m_overlaySwapchain, imageIndex, IID_PPV_ARGS(swapchainTexture.ReleaseAndGetAddressOf())));

        // We are about to do something destructive to the application context. Save the context. It will be
        // restored at the end of xrEndFrame().
        if (m_d3d11Device == m_ovrSubmissionDevice && !m_d3d11ContextState) {
            m_ovrSubmissionContext->SwapDeviceContextState(m_ovrSubmissionContextState.Get(),
                                                           m_d3d11ContextState.ReleaseAndGetAddressOf());
        }

        // Copy the background.
        m_ovrSubmissionContext->CopyResource(swapchainTexture.Get(), m_overlayBackground.Get());
        m_ovrSubmissionContext->Flush();

        // Draw the text.
        m_ovrSubmissionContext->ClearState();

        ComPtr<ID3D11RenderTargetView> rtv;
        {
            D3D11_RENDER_TARGET_VIEW_DESC desc{};
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            desc.Format = m_overlaySwapchainFormat;
            CHECK_HRCMD(m_ovrSubmissionDevice->CreateRenderTargetView(
                swapchainTexture.Get(), &desc, rtv.ReleaseAndGetAddressOf()));
        }
        m_ovrSubmissionContext->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
        {
            D3D11_VIEWPORT viewport{};
            viewport.Width = (float)m_overlayExtent.width;
            viewport.Height = (float)m_overlayExtent.height;
            viewport.MaxDepth = 1;
            m_ovrSubmissionContext->RSSetViewports(1, &viewport);
        }

        const uint32_t color = 0xffffffff;

        {
            char buf[8]{};
            std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&now));

            m_fontNormal->DrawString(m_ovrSubmissionContext.Get(),
                                     std::wstring(buf, buf + strlen(buf)).c_str(),
                                     200.f,
                                     600.f,
                                     12.f,
                                     color,
                                     FW1_LEFT | FW1_NOFLUSH);
        }

        const auto fps = m_frameTimes.size();
        m_fontNormal->DrawString(m_ovrSubmissionContext.Get(),
                                 fmt::format(L"{}", fps).c_str(),
                                 150.f,
                                 1400.f,
                                 1098.f,
                                 color,
                                 FW1_RIGHT | FW1_NOFLUSH);

        m_fontNormal->DrawString(m_ovrSubmissionContext.Get(),
                                 m_isAsyncReprojectionEnabled ? (m_isAsyncReprojectionActive ? L"Active" : L"Standby")
                                                           : L"Off",
                                 150.f,
                                 1400.f,
                                 1402.f,
                                 color,
                                 FW1_RIGHT | FW1_NOFLUSH);

        m_fontNormal->DrawString(m_ovrSubmissionContext.Get(),
                                 fmt::format(L"{}x{}", m_proj0Extent.width, m_proj0Extent.height).c_str(),
                                 150.f,
                                 1400.f,
                                 1754.f,
                                 color,
                                 FW1_RIGHT | FW1_NOFLUSH);

        m_fontNormal->Flush(m_ovrSubmissionContext.Get());

        CHECK_OVRCMD(ovr_CommitTextureSwapChain(m_ovrSession, m_overlaySwapchain));
    }

} // namespace virtualdesktop_openxr
