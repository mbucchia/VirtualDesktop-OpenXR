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

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    LRESULT CALLBACK wndProcWrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return static_cast<OpenXrRuntime*>(GetInstance())->mirrorWindowProc(hwnd, msg, wParam, lParam);
    }

    void OpenXrRuntime::createMirrorWindow() {
        m_mirrorWindowReady = false;
        m_mirrorWindowThread = std::thread([&]() {
            // Create the window.
            WNDCLASSEX wndClassEx = {sizeof(wndClassEx)};
            wndClassEx.lpfnWndProc = wndProcWrapper;
            wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
            wndClassEx.lpszClassName = L"PimaxXRMirrorWindow";
            CHECK_MSG(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                         (LPCWSTR)&wndProcWrapper,
                                         &wndClassEx.hInstance),
                      "Failed to get DLL handle");
            // We may let this fail if the class was already registered by a previous start up.
            RegisterClassExW(&wndClassEx);

            const std::string title = "PimaxXR Mirror Window - " + m_applicationName;
            const auto defaultWidth = m_cachedEyeInfo[0].DistortedViewport.Size.w / 2;
            const auto defaultHeight = m_cachedEyeInfo[0].DistortedViewport.Size.h / 2;
            m_mirrorWindowHwnd = CreateWindowW(wndClassEx.lpszClassName,
                                               xr::utf8_to_wide(title).c_str(),
                                               WS_OVERLAPPEDWINDOW,
                                               CW_USEDEFAULT,
                                               CW_USEDEFAULT,
                                               defaultWidth,
                                               defaultHeight,
                                               nullptr,
                                               nullptr,
                                               nullptr,
                                               nullptr);
            CHECK_MSG(m_mirrorWindowHwnd, "Failed to CreateWindowW()");
            m_mirrorWindowReady = true;

            // Create the swapchain.
            ComPtr<IDXGIFactory2> dxgiFactory;
            {
                ComPtr<IDXGIDevice1> dxgiDevice;
                CHECK_HRCMD(m_pvrSubmissionDevice->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

                ComPtr<IDXGIAdapter> dxgiAdapter;
                CHECK_HRCMD(dxgiDevice->GetAdapter(&dxgiAdapter));
                CHECK_HRCMD(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
            }

            RECT rect = {0, 0, defaultWidth, defaultHeight};
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
            const auto width = rect.right - rect.left;
            const auto height = rect.bottom - rect.top;

            DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
            swapchainDesc.Width = width;
            swapchainDesc.Height = height;
            swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            swapchainDesc.SampleDesc.Count = 1;
            swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchainDesc.BufferCount = 2;
            swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            CHECK_HRCMD(dxgiFactory->CreateSwapChainForHwnd(m_pvrSubmissionDevice.Get(),
                                                            m_mirrorWindowHwnd,
                                                            &swapchainDesc,
                                                            nullptr,
                                                            nullptr,
                                                            m_mirrorWindowSwapchain.ReleaseAndGetAddressOf()));

            ShowWindow(m_mirrorWindowHwnd, SW_SHOW);
            UpdateWindow(m_mirrorWindowHwnd);

            // Service the window.
            MSG msg;
            while (GetMessage(&msg, m_mirrorWindowHwnd, 0, 0) > 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // Free resources ASAP.
            {
                std::unique_lock lock(m_mirrorWindowLock);
                m_mirrorWindowSwapchain.Reset();
                m_mirrorTexture.Reset();
                pvr_destroyMirrorTexture(m_pvrSession, m_pvrMirrorSwapChain);
                m_pvrMirrorSwapChain = nullptr;
                m_mirrorWindowHwnd = nullptr;
            }
        });
    }

    void OpenXrRuntime::updateMirrorWindow() {
        std::unique_lock lock(m_mirrorWindowLock);

        if (!m_mirrorWindowSwapchain) {
            return;
        }

        RECT rect{};
        GetClientRect(m_mirrorWindowHwnd, &rect);
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        const auto width = rect.right - rect.left;
        const auto height = rect.bottom - rect.top;

        // Check if visible.
        if (!width || !height) {
            return;
        }

        // Check for resizing or initial creation.
        D3D11_TEXTURE2D_DESC mirrorDesc;
        if (m_mirrorTexture) {
            m_mirrorTexture->GetDesc(&mirrorDesc);
        }
        if (!m_mirrorTexture || mirrorDesc.Width != width || mirrorDesc.Height != height) {
            TraceLoggingWrite(g_traceProvider, "MirrorWindow", TLArg(width, "Width"), TLArg(height, "Height"));

            CHECK_HRCMD(m_mirrorWindowSwapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));

            // Recreate a new PVR swapchain with the correct size.
            if (m_pvrMirrorSwapChain) {
                m_mirrorTexture.Reset();
                pvr_destroyMirrorTexture(m_pvrSession, m_pvrMirrorSwapChain);
            }

            pvrMirrorTextureDesc mirrorDesc;
            mirrorDesc.Format = pvrTextureFormat::PVR_FORMAT_R8G8B8A8_UNORM_SRGB;
            mirrorDesc.Width = width;
            mirrorDesc.Height = height;
            mirrorDesc.SampleCount = 1;
            CHECK_PVRCMD(pvr_createMirrorTextureDX(
                m_pvrSession, m_pvrSubmissionDevice.Get(), &mirrorDesc, &m_pvrMirrorSwapChain));
            CHECK_PVRCMD(pvr_getMirrorTextureBufferDX(
                m_pvrSession, m_pvrMirrorSwapChain, IID_PPV_ARGS(m_mirrorTexture.ReleaseAndGetAddressOf())));
        }

        TraceLocalActivity(presentMirrorWindow);
        TraceLoggingWriteStart(presentMirrorWindow, "PresentMirrorWindow");

        // Let those fail silently below so we do not crash the application.
        ComPtr<ID3D11Texture2D> frameBuffer;
        m_mirrorWindowSwapchain->GetBuffer(0, IID_PPV_ARGS(frameBuffer.ReleaseAndGetAddressOf()));
        m_pvrSubmissionContext->CopyResource(frameBuffer.Get(), m_mirrorTexture.Get());
        m_mirrorWindowSwapchain->Present(0, 0);
        TraceLoggingWriteStop(presentMirrorWindow, "PresentMirrorWindow");
    }

    LRESULT CALLBACK OpenXrRuntime::mirrorWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

} // namespace pimax_openxr
