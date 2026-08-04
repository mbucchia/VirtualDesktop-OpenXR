// MIT License
//
// Copyright(c) 2025-2026 Matthieu Bucchianeri
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

#include "constantsbuffer.h"
#include "log.h"
#include "driver.h"
#include "utils.h"

#include "ReprojectVS.h"
#include "ReprojectPS.h"

using namespace ovrnull::driver;
using namespace ovrnull::log;
using namespace ovrnull::utils;

namespace {

    class NullDriver : public IDriver {
      private:
        static inline const OVR::Posef k_HeadToLeftController = {{0, 0, 0, 1}, {-0.15f, -0.2f, -0.35f}};
        static inline const OVR::Posef k_HeadToRightController = {{0, 0, 0, 1}, {0.15f, -0.2f, -0.35f}};

        static constexpr size_t k_SwapchainLength = 3;
        struct Swapchain {
            ComPtr<ID3D11Texture2D> textures[k_SwapchainLength];
            ComPtr<ID3D11ShaderResourceView> SRVs[k_SwapchainLength];
            ComPtr<ID3D11RenderTargetView> RTVs[k_SwapchainLength];
            ovrTextureSwapChainDesc desc{};
            uint32_t lastCommittedIndex{0};
        };

      public:
        NullDriver() {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_Ctor");

            {
                ComPtr<IDXGIFactory6> factory;
                CreateDXGIFactory2(0, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
                ComPtr<IDXGIAdapter1> adapter;
                factory->EnumAdapterByGpuPreference(
                    0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);
                m_adapterLuid = desc.AdapterLuid;
            }
            m_displayRate = 72.f;
            m_photonsTime = 1.f / m_displayRate;
            m_eyePose[ovrEye_Left].Position.x = -0.0315f;
            m_eyePose[ovrEye_Right].Position.x = 0.0315f;
            m_eyeFov[ovrEye_Left].UpTan = m_eyeFov[ovrEye_Left].DownTan = m_eyeFov[ovrEye_Left].LeftTan =
                m_eyeFov[ovrEye_Left].RightTan = (float)M_PI_2;
            m_eyeFov[ovrEye_Right] = m_eyeFov[ovrEye_Left];
            m_recommendedResolution = {2496, 2688};
            m_controllerButtons.ControllerType = ovrControllerType_Touch;
            m_controllerPose[0].ThePose = OVR::Posef(m_hmdPose.ThePose) * k_HeadToLeftController;
            m_controllerPose[1].ThePose = OVR::Posef(m_hmdPose.ThePose) * k_HeadToRightController;
            m_controllerSides = 0x3;

            LARGE_INTEGER now{};
            QueryPerformanceCounter(&now);
            m_nextFramePredictedDisplayTime = QpcToOvrTime(now);

            TraceLoggingWriteStop(local, "NullDriver_Ctor");
        }

        ~NullDriver() override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_Dtor");

            if (m_serverThread.joinable()) {
                m_terminateServerThread = true;
                m_serverThread.join();
                m_serverThread = {};
            }

            while (!m_swapchains.empty()) {
                Swapchain* swapchain = (Swapchain*)*m_swapchains.begin();
                TraceLoggingWriteTagged(local, "NullDriver_Dtor", TLPArg(swapchain, "DeleteSwapchain"));
                delete swapchain;
                m_swapchains.erase(m_swapchains.begin());
            }

            TraceLoggingWriteStop(local, "NullDriver_Dtor");
        }

        void SetSubmissionDevice(ID3D11Device* device) override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_SetSubmissionDevice", TLPArg(device, "Device"));

            if (!m_submissionDevice) {
                m_submissionDevice = device;
                m_submissionDevice->GetImmediateContext(m_submissionContext.ReleaseAndGetAddressOf());

                winrt::check_hresult(m_submissionDevice->CreateVertexShader(
                    k_ReprojectVS, sizeof(k_ReprojectVS), nullptr, m_reprojectVS.ReleaseAndGetAddressOf()));
                winrt::check_hresult(m_submissionDevice->CreatePixelShader(
                    k_ReprojectPS, sizeof(k_ReprojectPS), nullptr, m_reprojectPS.ReleaseAndGetAddressOf()));
                {
                    D3D11_BUFFER_DESC desc{};
                    desc.ByteWidth = ((sizeof(ConstantsBuffer) + 15) / 16) * 16;
                    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                    desc.Usage = D3D11_USAGE_DYNAMIC;
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                    winrt::check_hresult(
                        m_submissionDevice->CreateBuffer(&desc, nullptr, m_constantsBuffer.ReleaseAndGetAddressOf()));
                }
                {
                    D3D11_DEPTH_STENCIL_DESC desc{};
                    winrt::check_hresult(m_submissionDevice->CreateDepthStencilState(
                        &desc, m_noDepthTestState.ReleaseAndGetAddressOf()));
                }
                {
                    D3D11_BLEND_DESC desc{};
                    desc.RenderTarget[0].BlendEnable = TRUE;
                    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
                    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
                    winrt::check_hresult(
                        m_submissionDevice->CreateBlendState(&desc, m_blendState.ReleaseAndGetAddressOf()));
                }
                {
                    D3D11_SAMPLER_DESC desc{};
                    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
                    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
                    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                    desc.MaxAnisotropy = 1;
                    desc.MinLOD = D3D11_MIP_LOD_BIAS_MIN;
                    desc.MaxLOD = D3D11_MIP_LOD_BIAS_MAX;
                    winrt::check_hresult(
                        m_submissionDevice->CreateSamplerState(&desc, m_linearSampler.ReleaseAndGetAddressOf()));
                }
            }

            if (!m_serverThread.joinable()) {
                m_terminateServerThread = false;
                m_serverThread = std::thread([&]() {
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
                    timeBeginPeriod(1);

                    const LONGLONG ticksToWait = (LONGLONG)(1e6f / m_displayRate * g_qpcFrequency.QuadPart) / 1000000;

                    while (!m_terminateServerThread) {
                        TraceLocalActivity(runFrame);
                        TraceLoggingWriteStart(runFrame, "NullDriver_SubmitFrame_RunFrame");

                        LARGE_INTEGER currentTime;
                        QueryPerformanceCounter(&currentTime);
                        LARGE_INTEGER targetTime;
                        targetTime.QuadPart = currentTime.QuadPart + ticksToWait;

                        const LONGLONG coarseBufferTicks = (1500 * g_qpcFrequency.QuadPart) / 1000000;
                        while (1) {
                            QueryPerformanceCounter(&currentTime);
                            const LONGLONG ticksRemaining = targetTime.QuadPart - currentTime.QuadPart;

                            if (ticksRemaining <= coarseBufferTicks) {
                                break;
                            }
                            Sleep(1);
                        }

                        while (1) {
                            QueryPerformanceCounter(&currentTime);
                            if (currentTime.QuadPart >= targetTime.QuadPart) {
                                break;
                            }
                            _mm_pause();
                        }

                        std::unique_lock lock(m_frameMutex);
                        m_nextFramePredictedDisplayTime =
                            QpcToOvrTime(currentTime) + (1.0 / m_displayRate) + m_photonsTime;
                        m_lastSignaledVsync++;
                        m_frameVsync.notify_all();

                        TraceLoggingWriteStop(runFrame, "NullDriver_SubmitFrame_RunFrame");
                    }

                    timeEndPeriod(1);
                });
            }

            TraceLoggingWriteStop(local, "NullDriver_SetSubmissionDevice");
        }

        void* CreateSwapchain(const ovrTextureSwapChainDesc& ovrDesc) override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local,
                                   "NullDriver_CreateSwapchain",
                                   TLArg(ToString(ovrDesc.Type), "Type"),
                                   TLArg(ToString(ovrDesc.Format), "Format"),
                                   TLArg(ovrDesc.ArraySize, "ArraySize"),
                                   TLArg(ovrDesc.Width, "Width"),
                                   TLArg(ovrDesc.Height, "Height"),
                                   TLArg(ovrDesc.MipLevels, "MipLevels"),
                                   TLArg(ovrDesc.SampleCount, "SampleCount"),
                                   TLArg(!!ovrDesc.StaticImage, "StaticImage"),
                                   TLArg(ovrDesc.MiscFlags, "MiscFlags"),
                                   TLArg(ovrDesc.BindFlags, "BindFlags"));

            Swapchain* swapchain = nullptr;

            const DXGI_FORMAT format = ToDxgiTextureFormat(ovrDesc.Format);
            const bool isDepthSwapchain = (ovrDesc.BindFlags & ovrTextureBind_DX_DepthStencil) || IsDepthFormat(format);

            D3D11_TEXTURE2D_DESC desc{};
            desc.Format = format;
            // Per OVR documentation, depth swapchains are always typeless.
            if ((ovrDesc.MiscFlags & ovrTextureMisc_DX_Typeless) || isDepthSwapchain) {
                desc.Format = GetTypelessFormat(desc.Format);
            }
            desc.Width = ovrDesc.Width;
            desc.Height = ovrDesc.Height;
            desc.SampleDesc.Count = ovrDesc.SampleCount;
            desc.MipLevels = ovrDesc.MipLevels;
            desc.ArraySize = ovrDesc.ArraySize;
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            if (ovrDesc.BindFlags & ovrTextureBind_DX_RenderTarget) {
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            }
            if (ovrDesc.BindFlags & ovrTextureBind_DX_UnorderedAccess) {
                desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }
            if (ovrDesc.BindFlags & ovrTextureBind_DX_DepthStencil) {
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }
            desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
            if (ovrDesc.Type == ovrTexture_Cube) {
                desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
            }
            if (ovrDesc.MiscFlags & ovrTextureMisc_AllowGenerateMips) {
                desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
            }

            swapchain = new Swapchain;
            swapchain->desc = ovrDesc;

            for (size_t i = 0; i < (ovrDesc.StaticImage ? 1 : k_SwapchainLength); i++) {
                // Create the typeless, app swapchain images.
                winrt::check_hresult(m_submissionDevice->CreateTexture2D(
                    &desc, nullptr, swapchain->textures[i].ReleaseAndGetAddressOf()));

                if (!isDepthSwapchain) {
                    // RTV for mirror rendering.
                    if (ovrDesc.BindFlags & ovrTextureBind_DX_RenderTarget) {
                        D3D11_RENDER_TARGET_VIEW_DESC desc{};
                        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                        desc.Format = format;
                        winrt::check_hresult(m_submissionDevice->CreateRenderTargetView(
                            swapchain->textures[i].Get(), &desc, swapchain->RTVs[i].ReleaseAndGetAddressOf()));
                    }
                    // SRV for compositing.
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        desc.Format = format;
                        desc.Texture2D.MipLevels = -1;
                        winrt::check_hresult(m_submissionDevice->CreateShaderResourceView(
                            swapchain->textures[i].Get(), &desc, swapchain->SRVs[i].ReleaseAndGetAddressOf()));
                    }
                }
            }
            {
                std::unique_lock lock(m_swapchainMutex);
                m_swapchains.insert(swapchain);
            }

            TraceLoggingWriteStop(local, "NullDriver_CreateSwapchain", TLPArg(swapchain, "Swapchain"));

            return swapchain;
        }

        void DestroySwapchain(void* swapchain) override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_DestroySwapchain", TLPArg(swapchain, "Swapchain"));

            std::unique_lock lock(m_swapchainMutex);
            if (m_swapchains.erase(swapchain)) {
                Swapchain* swapchainObject = (Swapchain*)swapchain;
                delete swapchainObject;
            }

            TraceLoggingWriteStop(local, "NullDriver_DestroySwapchain");
        }

        ovrTextureSwapChainDesc GetSwapchainDesc(void* swapchain) const override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_GetSwapchainDesc", TLPArg(swapchain, "Swapchain"));

            std::shared_lock lock(m_swapchainMutex);
            ovrTextureSwapChainDesc desc{};
            if (m_swapchains.count(swapchain)) {
                Swapchain* swapchainObject = (Swapchain*)swapchain;
                desc = swapchainObject->desc;
            }

            TraceLoggingWriteStop(local, "NullDriver_GetSwapchainDesc");

            return desc;
        }

        ID3D11Texture2D* GetSwapchainImage(void* swapchain, int index) const override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(
                local, "NullDriver_GetSwapchainImage", TLPArg(swapchain, "Swapchain"), TLArg(index, "Index"));

            ID3D11Texture2D* image = nullptr;

            std::shared_lock lock(m_swapchainMutex);
            if (m_swapchains.count(swapchain) && index < k_SwapchainLength) {
                Swapchain* swapchainObject = (Swapchain*)swapchain;
                image = swapchainObject->textures[index].Get();
            }

            TraceLoggingWriteStop(local, "NullDriver_GetSwapchainImage", TLPArg(image, "Image"));

            return image;
        }

        int GetSwapchainImageIndex(void* swapchain) const override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_GetSwapchainImageIndex", TLPArg(swapchain, "Swapchain"));

            int index = -1;
            std::shared_lock lock(m_swapchainMutex);
            if (m_swapchains.count(swapchain)) {
                Swapchain* swapchainObject = (Swapchain*)swapchain;
                index = swapchainObject->lastCommittedIndex == 0 ? (k_SwapchainLength - 1)
                                                                 : (swapchainObject->lastCommittedIndex - 1);
            }

            TraceLoggingWriteStop(local, "NullDriver_GetSwapchainImageIndex", TLArg(index, "Index"));

            return index;
        }

        bool CommitSwapchainImage(void* swapchain) const override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_CommitSwapchainImage", TLPArg(swapchain, "Swapchain"));

            bool success = false;

            std::shared_lock lock(m_swapchainMutex);
            if (m_swapchains.count(swapchain)) {
                Swapchain* swapchainObject = (Swapchain*)swapchain;
                swapchainObject->lastCommittedIndex++;
                if (swapchainObject->lastCommittedIndex >= k_SwapchainLength) {
                    swapchainObject->lastCommittedIndex = 0;
                }
                TraceLoggingWriteTagged(
                    local, "NullDriver_CommitSwapchainImage", TLArg(swapchainObject->lastCommittedIndex, "Index"));
                success = true;
            }

            TraceLoggingWriteStop(local, "NullDriver_CommitSwapchainImage", TLArg(success, "Success"));

            return success;
        }

        void SetMirrorTexture(void* swapchain) override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_SetMirrorTexture", TLPArg(swapchain, "Swapchain"));

            if (swapchain) {
                std::shared_lock lock(m_swapchainMutex);
                if (m_swapchains.count(swapchain)) {
                    Swapchain* swapchainObject = (Swapchain*)swapchain;
                    m_mirrorRTV = swapchainObject->RTVs[0];
                    m_mirrorWidth = swapchainObject->desc.Width;
                    m_mirrorHeight = swapchainObject->desc.Height;
                }
            } else {
                m_mirrorRTV.Reset();
            }

            TraceLoggingWriteStop(local, "NullDriver_SetMirrorTexture");
        }

        double GetPredictedDisplayTime(long long frameIndex) const override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_GetPredictedDisplayTime", TLArg(frameIndex, "Frame"));

            double predictedDisplayTime;
            // Base our prediction on the last successfully waited frame.
            if (frameIndex <= m_lastWaitedFrame) {
                predictedDisplayTime = m_nextFramePredictedDisplayTime;
            } else {
                const auto numFramesInTheFuture = frameIndex - m_lastWaitedFrame;
                predictedDisplayTime = m_nextFramePredictedDisplayTime + numFramesInTheFuture * (1.0 / m_displayRate);
            }

            TraceLoggingWriteStop(
                local, "NullDriver_GetPredictedDisplayTime", TLArg(predictedDisplayTime, "PredictedDisplayTime"));

            return predictedDisplayTime;
        }

        void WaitForVsync(long long frameIndex) override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_WaitForVsync", TLArg(frameIndex, "Frame"));

            std::unique_lock lock(m_frameMutex);
            // Only wait when a future frame is being waited on. We assume the app will never wait for any frame
            // later than N+1.
            if (frameIndex > m_lastWaitedFrame) {
                using namespace std::chrono_literals;

                TraceLocalActivity(wait);
                TraceLoggingWriteStart(wait,
                                       "NullDriver_WaitForVsync_DirectModeComponent",
                                       TLArg(m_lastSignaledVsync, "LastSignaledVsync"));

                // TODO: If the app fell behind, we shouldn't be waiting here.
                const auto lastSignaledVsync = m_lastSignaledVsync;
                m_frameVsync.wait_for(lock, 100ms, [&]() { return m_lastSignaledVsync > lastSignaledVsync; });
                m_lastWaitedFrame = frameIndex;

                TraceLoggingWriteStop(wait,
                                      "NullDriver_WaitForVsync_DirectModeComponent",
                                      TLArg(m_lastSignaledVsync, "LastSignaledVsync"));
            }

            TraceLoggingWriteStop(local,
                                  "NullDriver_WaitForVsync",
                                  TLArg(m_nextFramePredictedDisplayTime, "NextFramePredictedDisplayTime"));
        }

        bool SubmitFrame(const std::vector<const ovrLayerHeader*>& layers) override {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "NullDriver_SubmitFrame");

            if (m_mirrorRTV) {
                const float clearColor[] = {0.f, 0.f, 0.f, 1.f};
                m_submissionContext->ClearRenderTargetView(m_mirrorRTV.Get(), clearColor);
            }

            m_submissionContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_submissionContext->OMSetRenderTargets(1, m_mirrorRTV.GetAddressOf(), nullptr);
            m_submissionContext->VSSetConstantBuffers(0, 1, m_constantsBuffer.GetAddressOf());
            m_submissionContext->PSSetSamplers(0, 1, m_linearSampler.GetAddressOf());
            m_submissionContext->PSSetConstantBuffers(0, 1, m_constantsBuffer.GetAddressOf());
            m_submissionContext->PSSetShader(m_reprojectPS.Get(), nullptr, 0);
            m_submissionContext->OMSetDepthStencilState(m_noDepthTestState.Get(), 0xff);
            m_submissionContext->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);

            {
                std::shared_lock lock(m_swapchainMutex);

                for (const ovrLayerHeader* layerHeader : layers) {
                    const ovrLayer_Union* layer = (ovrLayer_Union*)layerHeader;

                    // Per OVR documentation, this is legal and equivalent to ovrLayerType_Disabled.
                    if (!layer) {
                        // Nothing to do here.
                        continue;
                    }

                    if (layer->Header.Type == ovrLayerType_EyeFov || layer->Header.Type == ovrLayerType_EyeFovDepth) {
                        // Make sure that we can use the EyeFov part of EyeFovDepth equivalently.
                        static_assert(offsetof(decltype(layer->EyeFov), ColorTexture) ==
                                      offsetof(decltype(layer->EyeFovDepth), ColorTexture));
                        static_assert(offsetof(decltype(layer->EyeFov), Viewport) ==
                                      offsetof(decltype(layer->EyeFovDepth), Viewport));
                        static_assert(offsetof(decltype(layer->EyeFov), Fov) ==
                                      offsetof(decltype(layer->EyeFovDepth), Fov));
                        static_assert(offsetof(decltype(layer->EyeFov), RenderPose) ==
                                      offsetof(decltype(layer->EyeFovDepth), RenderPose));
                        static_assert(offsetof(decltype(layer->EyeFov), SensorSampleTime) ==
                                      offsetof(decltype(layer->EyeFovDepth), SensorSampleTime));

                        const auto eyeFov = &layer->EyeFov;

                        m_submissionContext->VSSetShader(m_reprojectVS.Get(), nullptr, 0);

                        ConstantsBuffer constants{};
                        constants.flipY = layer->Header.Flags & ovrLayerFlag_TextureOriginAtBottomLeft;
                        for (uint32_t eye = 0; eye < ovrEye_Count; eye++) {
                            const auto baseProjection = LoadOvrProjection(m_eyeFov[eye], 0.01f, 1000.f);
                            const auto projection = LoadOvrProjection(eyeFov->Fov[eye], 0.01f, 1000.f);
                            const auto view = LoadInvertedOvrPose(eyeFov->RenderPose[eye]);

                            // TODO: Reproject for mismatched eye poses (not just mutable FOV).
                            DirectX::XMStoreFloat4x4(
                                &constants.reprojectionMatrix,
                                DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, baseProjection) *
                                                           projection));

                            // OVR allows specifying null texture for the right eye.
                            const auto swapchain = eye == 0 || !eyeFov->ColorTexture[eye] ? eyeFov->ColorTexture[0]
                                                                                          : eyeFov->ColorTexture[eye];
                            if (!m_swapchains.count(swapchain)) {
                                return false;
                            }
                            Swapchain* swapchainObject = (Swapchain*)swapchain;

                            constants.imageRectNormalized.offset = {
                                (float)eyeFov->Viewport[eye].Pos.x / swapchainObject->desc.Width,
                                (float)eyeFov->Viewport[eye].Pos.y / swapchainObject->desc.Height};
                            constants.imageRectNormalized.extent = {
                                (float)eyeFov->Viewport[eye].Size.w / swapchainObject->desc.Width,
                                (float)eyeFov->Viewport[eye].Size.h / swapchainObject->desc.Height};

                            D3D11_MAPPED_SUBRESOURCE mappedResources;
                            winrt::check_hresult(m_submissionContext->Map(
                                m_constantsBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                            memcpy(mappedResources.pData, &constants, sizeof(constants));
                            m_submissionContext->Unmap(m_constantsBuffer.Get(), 0);

                            D3D11_VIEWPORT viewport{};
                            viewport.Width = m_mirrorWidth / 2.f;
                            viewport.TopLeftX = eye * viewport.Width;
                            viewport.Height = (float)m_mirrorHeight;
                            viewport.MaxDepth = 1.f;
                            m_submissionContext->RSSetViewports(1, &viewport);
                            m_submissionContext->PSSetShaderResources(
                                0, 1, swapchainObject->SRVs[swapchainObject->lastCommittedIndex].GetAddressOf());

                            if (m_mirrorRTV) {
                                m_submissionContext->Draw(3, 0);
                            }
                        }

                    } else if (layer->Header.Type == ovrLayerType_Quad) {
                        // TODO: Compose quad layers to mirror.
                    } else if (layer->Header.Type == ovrLayerType_Disabled) {
                        // Nothing to do here.
                    } else {
                        // This type of layer (eg: cylinder, cube) is not supported. Skip it.
                    }
                }
            }

            ProcessActionKeys();

            // "Maintain" the pose time.
            {
                LARGE_INTEGER now{};
                QueryPerformanceCounter(&now);

                std::unique_lock lock(m_hmdPoseMutex);
                m_hmdPose.TimeInSeconds = QpcToOvrTime(now);
                std::unique_lock lock2(m_controllerMutex);
                m_controllerPose[0].TimeInSeconds = m_controllerPose[1].TimeInSeconds =
                    m_controllerButtons.TimeInSeconds = m_hmdPose.TimeInSeconds;
            }

            TraceLoggingWriteStop(local, "NullDriver_SubmitFrame");

            return true;
        }

        void ProcessActionKeys() {
#define ACTION_KEY(label, key, action)                                                                                 \
    static bool wasCtrl##label##Pressed = false;                                                                       \
    const bool isCtrl##label##Pressed = GetAsyncKeyState(key) < 0;                                                     \
    if (!wasCtrl##label##Pressed && isCtrl##label##Pressed) {                                                          \
        action();                                                                                                      \
    }                                                                                                                  \
    wasCtrl##label##Pressed = isCtrl##label##Pressed;

            OVR::Posef transform = OVR::Posef::Identity();
            ACTION_KEY(Reset, 'R', [&] {
                std::unique_lock lock(m_hmdPoseMutex);
                m_hmdPose.ThePose = OVR::Posef::Identity();
            });
            ACTION_KEY(Forward, 'W', [&] { transform.Translation.z = -0.1f; });
            ACTION_KEY(Left, 'A', [&] { transform.Translation.x = -0.1f; });
            ACTION_KEY(Backward, 'S', [&] { transform.Translation.z = 0.1f; });
            ACTION_KEY(Right, 'D', [&] { transform.Translation.x = 0.1f; });
            ACTION_KEY(TurnLeft, 'Q', [&] { transform.Rotation = OVR::Quatf::FastFromRotationVector({0, 0.1f, 0}); });
            ACTION_KEY(TurnRight, 'E', [&] { transform.Rotation = OVR::Quatf::FastFromRotationVector({0, -0.1f, 0}); });
            std::unique_lock lock(m_hmdPoseMutex);
            m_hmdPose.ThePose = OVR::Posef(m_hmdPose.ThePose) * transform;

            std::unique_lock lock2(m_controllerMutex);
            m_controllerButtons.IndexTriggerRaw[0] = GetAsyncKeyState('U') < 0 ? 1.f : 0.f;
            if (GetAsyncKeyState('I') < 0) {
                m_controllerButtons.Buttons |= ovrButton_Enter;
            } else {
                m_controllerButtons.Buttons &= ~ovrButton_Enter;
            }
            m_controllerButtons.IndexTriggerRaw[1] = GetAsyncKeyState('O') < 0 ? 1.f : 0.f;
            if (GetAsyncKeyState('P') < 0) {
                m_controllerButtons.Buttons |= ovrButton_A;
            } else {
                m_controllerButtons.Buttons &= ~ovrButton_A;
            }
            m_controllerPose[0].ThePose = OVR::Posef(m_hmdPose.ThePose) * k_HeadToLeftController;
            m_controllerPose[1].ThePose = OVR::Posef(m_hmdPose.ThePose) * k_HeadToRightController;
        }

        ovrPoseStatef PropagatePose(const ovrPoseStatef& pose, double time) const {
            const float deltaTime = (float)(time - pose.TimeInSeconds);
            if (deltaTime < FLT_EPSILON) {
                return pose;
            }

            ovrPoseStatef predictedPose;

            // Integrate linear velocity over time.
            OVR::Vector3f linearVelocity = pose.LinearVelocity;
            OVR::Vector3f position = pose.ThePose.Position;
            position += (linearVelocity * deltaTime);
            predictedPose.ThePose.Position = position;
            predictedPose.LinearAcceleration = pose.LinearAcceleration;
            predictedPose.LinearVelocity = pose.LinearVelocity;

            // Integrate angular velocity and acceleration over time.
            OVR::Quatf orientation = pose.ThePose.Orientation;
            predictedPose.ThePose.Orientation =
                orientation.TimeIntegrate(pose.AngularVelocity, pose.AngularAcceleration, deltaTime);
            predictedPose.AngularAcceleration = pose.AngularAcceleration;
            predictedPose.AngularVelocity = pose.AngularVelocity;

            predictedPose.TimeInSeconds = time;

            return predictedPose;
        }

        std::string GetManufacturerName() const override {
            return "Virtual Desktop";
        }

        std::string GetModelNumber() const override {
            return "Emulated Device";
        }

        LUID GetAdapterLuid() const override {
            return m_adapterLuid;
        }

        float GetDisplayRate() const override {
            return m_displayRate;
        }

        ovrSizei GetRecommendedResolution() const override {
            return m_recommendedResolution;
        }

        ovrFovPort GetEyeFov(ovrEyeType eye) const override {
            return m_eyeFov[eye];
        }

        ovrPosef GetEyePose(ovrEyeType eye) const override {
            std::shared_lock lock(m_hmdPoseMutex);
            return m_eyePose[eye];
        }

        void SetStageTracking(bool useStage) override {
            m_useStageTracking = useStage;
        }

        bool IsStageTracking() const override {
            return m_useStageTracking;
        }

        float GetEyeHeight() const override {
            return OVR_DEFAULT_EYE_HEIGHT;
        }

        bool IsAswActive() const override {
            return false;
        }

        ovrPoseStatef GetHmdPose(double absTime) const override {
            ovrPoseStatef latched;
            {
                std::shared_lock lock(m_hmdPoseMutex);
                latched = m_hmdPose;
            }
            return PropagatePose(latched, absTime);
        }

        bool HasController(ovrHandType side) const override {
            std::shared_lock lock(m_controllerMutex);
            return m_controllerSides & (1 << side);
        }

        ovrPoseStatef GetControllerPose(ovrHandType side, double absTime) const override {
            ovrPoseStatef latched;
            {
                std::shared_lock lock(m_controllerMutex);
                latched = m_controllerPose[side];
            }
            return PropagatePose(latched, absTime);
        }

        ovrInputState GetControllerButtons() const override {
            ovrInputState inputState = m_controllerButtons;
            for (uint32_t side = 0; side < ovrHand_Count; side++) {
                inputState.IndexTrigger[side] = inputState.IndexTriggerNoDeadzone[side] =
                    inputState.IndexTriggerRaw[side];
                inputState.HandTrigger[side] = inputState.HandTriggerNoDeadzone[side] = inputState.HandTriggerRaw[side];
                inputState.Thumbstick[side] = inputState.ThumbstickNoDeadzone[side] = inputState.ThumbstickRaw[side];
            }
            return inputState;
        }

        void SetControllerVibration(ovrHandType side, float frequency, float amplitude) {
            // Just a placeholder for now.
        }

      private:
        std::thread m_serverThread;
        std::atomic<bool> m_terminateServerThread = false;

        LUID m_adapterLuid{};
        ComPtr<ID3D11Device> m_submissionDevice;
        ComPtr<ID3D11DeviceContext> m_submissionContext;
        ComPtr<ID3D11VertexShader> m_reprojectVS;
        ComPtr<ID3D11PixelShader> m_reprojectPS;
        ComPtr<ID3D11Buffer> m_constantsBuffer;
        ComPtr<ID3D11DepthStencilState> m_noDepthTestState;
        ComPtr<ID3D11BlendState> m_blendState;
        ComPtr<ID3D11SamplerState> m_linearSampler;

        float m_displayRate{};
        float m_photonsTime{0};
        ovrSizei m_recommendedResolution{};
        ovrFovPort m_eyeFov[ovrEye_Count]{};
        bool m_useStageTracking{false};

        mutable std::shared_mutex m_swapchainMutex;
        std::unordered_set<void*> m_swapchains;
        ComPtr<ID3D11RenderTargetView> m_mirrorRTV;
        uint32_t m_mirrorWidth{0};
        uint32_t m_mirrorHeight{0};

        mutable std::shared_mutex m_controllerMutex;
        uint32_t m_controllerSides{0};
        ovrPoseStatef m_controllerPose[ovrHand_Count]{{OVR::Posef::Identity()}, {OVR::Posef::Identity()}};
        ovrInputState m_controllerButtons{};

        mutable std::shared_mutex m_hmdPoseMutex;
        ovrPoseStatef m_hmdPose{OVR::Posef::Identity()};
        ovrPosef m_eyePose[ovrEye_Count]{OVR::Posef::Identity(), OVR::Posef::Identity()};

        mutable std::mutex m_frameMutex;
        long long m_lastSignaledVsync{};
        long long m_lastWaitedFrame{};
        double m_nextFramePredictedDisplayTime{0.0};
        std::condition_variable m_frameVsync;
    };

} // namespace

namespace ovrnull::driver {

    IDriver* CreateDriver() {
        return new NullDriver();
    }

} // namespace ovrnull::driver
