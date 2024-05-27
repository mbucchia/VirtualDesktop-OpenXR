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

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace DirectX;
    using namespace xr::math;

    struct AlphaBlendingCSConstants {
        alignas(8) XrOffset2Di offset;
        alignas(8) XrExtent2Di dimension;
        alignas(4) bool ignoreAlpha;
        alignas(4) bool isUnpremultipliedAlpha;
        alignas(4) bool isSRGB;
    };

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrWaitFrame
    XrResult OpenXrRuntime::xrWaitFrame(XrSession session,
                                        const XrFrameWaitInfo* frameWaitInfo,
                                        XrFrameState* frameState) {
        if ((frameWaitInfo && frameWaitInfo->type != XR_TYPE_FRAME_WAIT_INFO) ||
            frameState->type != XR_TYPE_FRAME_STATE) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrWaitFrame", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_sessionBegun || m_sessionState == XR_SESSION_STATE_IDLE || m_sessionState == XR_SESSION_STATE_EXITING) {
            return XR_ERROR_SESSION_NOT_RUNNING;
        }

        // Check for user presence and exit conditions.
        CHECK_OVRCMD(ovr_GetSessionStatus(m_ovrSession, &m_hmdStatus));
        TraceLoggingWrite(g_traceProvider,
                          "OVR_SessionStatus",
                          TLArg(!!m_hmdStatus.HmdPresent, "HmdPresent"),
                          TLArg(!!m_hmdStatus.HmdMounted, "HmdMounted"),
                          TLArg(!!m_hmdStatus.IsVisible, "IsVisible"),
                          TLArg(!!m_hmdStatus.DisplayLost, "DisplayLost"),
                          TLArg(!!m_hmdStatus.ShouldRecenter, "ShouldRecenter"),
                          TLArg(!!m_hmdStatus.ShouldQuit, "ShouldQuit"));
        if (!m_sessionLossPending) {
            m_sessionLossPending = !m_hmdStatus.HmdPresent || m_hmdStatus.DisplayLost || m_hmdStatus.ShouldQuit;
        }
        if (!m_shouldRecenter && m_hmdStatus.ShouldRecenter) {
            // We will send 2 events, one for LOCAL and one for STAGE.
            m_shouldRecenter = 2;
            m_recenterTime = ovrTimeToXrTime(ovr_GetTimeInSeconds());

            ovr_ClearShouldRecenterFlag(m_ovrSession);
        }
        updateSessionState();

        // Check for changes in display refresh rate.
        const ovrHmdDesc hmdInfo = ovr_GetHmdDesc(m_ovrSession);
        TraceLoggingWrite(g_traceProvider, "OVR_HmdDesc", TLArg(hmdInfo.DisplayRefreshRate, "DisplayRefreshRate"));
        if (hmdInfo.DisplayRefreshRate != m_displayRefreshRate) {
            m_displayRefreshRateChanged = m_displayRefreshRate;
            m_displayRefreshRate = hmdInfo.DisplayRefreshRate;
            m_idealFrameDuration = m_predictedFrameDuration = 1.0 / hmdInfo.DisplayRefreshRate;
        }

        frameState->shouldRender =
            (!m_isHeadless && !m_sessionStopping && !m_sessionExiting && !m_sessionLossPending && m_hmdStatus.IsVisible)
                ? XR_TRUE
                : XR_FALSE;

        // Critical section.
        {
            CpuTimer waitTimer;
            if (IsTraceEnabled()) {
                waitTimer.start();
            }

            std::unique_lock lock(m_frameMutex);

            m_frameTimerApp.stop();
            m_lastCpuFrameTimeUs = m_frameTimerApp.query();

            if (m_frameCompleted > 0) {
                TraceLoggingWrite(g_traceProvider,
                                  "App_Statistics",
                                  TLArg(m_frameCompleted - 1, "FrameId"),
                                  TLArg(m_lastCpuFrameTimeUs, "AppFrameCpuTime"));
            }

            // Wait for a call to xrBeginFrame() to match the previous call to xrWaitFrame().
            {
                TraceLocalActivity(waitBeginFrame);
                TraceLoggingWriteStart(waitBeginFrame,
                                       "WaitBeginFrame",
                                       TLArg(m_frameWaited, "FrameWaited"),
                                       TLArg(m_frameBegun, "FrameBegun"),
                                       TLArg(m_frameCompleted, "FrameCompleted"));
                m_frameCondVar.wait(lock, [&] { return m_frameBegun == m_frameWaited; });
                TraceLoggingWriteStop(waitBeginFrame, "WaitBeginFrame");
            }

            // Workaround: OVR cannot wait for a frame without having a device. If no swapchain was created up to this
            // point, we must create one to initialize OVR.
            if (m_frameWaited == 0) {
                // Make as small as possible of a memory footprint...
                ovrTextureSwapChainDesc desc{};
                desc.Type = ovrTexture_2D;
                desc.StaticImage = true;
                desc.ArraySize = 1;
                desc.Width = desc.Height = 128;
                desc.MipLevels = 1;
                desc.SampleCount = 1;
                desc.Format = OVR_FORMAT_B8G8R8A8_UNORM;

                CHECK_OVRCMD(ovr_CreateTextureSwapChainDX(
                    m_ovrSession, m_ovrSubmissionDevice.Get(), &desc, &m_headlessSwapchain));
            }

            if (m_needStartAsyncSubmissionThread) {
                m_terminateAsyncThread = false;
                m_asyncSubmissionThread = std::thread([&]() { asyncSubmissionThread(); });
                m_needStartAsyncSubmissionThread = false;
            }

            // Wait for OVR to be ready for the next frame.
            const long long ovrFrameId = m_frameWaited;
            if (!m_useAsyncSubmission) {
                TraceLocalActivity(waitToBeginFrame);
                TraceLoggingWriteStart(waitToBeginFrame, "OVR_WaitToBeginFrame", TLArg(ovrFrameId, "FrameId"));
                lock.unlock();
                CHECK_OVRCMD(ovr_WaitToBeginFrame(m_ovrSession, ovrFrameId));
                lock.lock();
                TraceLoggingWriteStop(waitToBeginFrame, "OVR_WaitToBeginFrame");
            } else {
                if (!m_useDeferredFrameWait) {
                    waitForAsyncSubmissionIdle(m_useRunningStart);
                }
                TraceLoggingWrite(g_traceProvider, "AcquiredFrame", TLArg(ovrFrameId, "FrameId"));
            }

            if (IsTraceEnabled()) {
                waitTimer.stop();
            }

            const double now = ovr_GetTimeInSeconds();
            double predictedDisplayTime = ovr_GetPredictedDisplayTime(m_ovrSession, ovrFrameId);
            TraceLoggingWrite(g_traceProvider,
                              "WaitFrame",
                              TLArg(now, "Now"),
                              TLArg(predictedDisplayTime, "PredictedDisplayTime"),
                              TLArg(predictedDisplayTime - now, "PhotonTime"),
                              TLArg(waitTimer.query(), "WaitDurationUs"));

            // Setup the app frame for use and the next frame for this call.
            frameState->predictedDisplayTime = ovrTimeToXrTime(predictedDisplayTime);

            // Workaround: during early calls, OVR times might violate OpenXR rules.
            if (frameState->predictedDisplayTime <= m_lastPredictedDisplayTime) {
                frameState->predictedDisplayTime = m_lastPredictedDisplayTime + 1;
            }
            m_lastPredictedDisplayTime = frameState->predictedDisplayTime;

            // We always use the native frame duration, regardless of Smart Smoothing.
            frameState->predictedDisplayPeriod = (XrDuration)(m_predictedFrameDuration * 1e9);

            m_frameTimerApp.start();

            m_frameWaited++;

            TraceLoggingWrite(g_traceProvider,
                              "WaitFrame_State",
                              TLArg(m_frameWaited, "FrameWaited"),
                              TLArg(m_frameBegun, "FrameBegun"),
                              TLArg(m_frameCompleted, "FrameCompleted"));
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrWaitFrame",
                          TLArg(!!frameState->shouldRender, "ShouldRender"),
                          TLArg(frameState->predictedDisplayTime, "PredictedDisplayTime"),
                          TLArg(frameState->predictedDisplayPeriod, "PredictedDisplayPeriod"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrBeginFrame
    XrResult OpenXrRuntime::xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
        if (frameBeginInfo && frameBeginInfo->type != XR_TYPE_FRAME_BEGIN_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrBeginFrame", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_sessionBegun || m_sessionState == XR_SESSION_STATE_IDLE || m_sessionState == XR_SESSION_STATE_EXITING) {
            return XR_ERROR_SESSION_NOT_RUNNING;
        }

        bool frameDiscarded = false;

        // Critical section.
        {
            CpuTimer waitTimer;
            if (IsTraceEnabled()) {
                waitTimer.start();
            }

            std::unique_lock lock(m_frameMutex);

            if (m_frameWaited == m_frameCompleted || m_frameBegun == m_frameWaited) {
                return XR_ERROR_CALL_ORDER_INVALID;
            }

            if (m_frameBegun != m_frameWaited && m_frameWaited == m_frameCompleted + 1) {
                // Wait for a call to xrEndFrame() to match the previous call to xrBeginFrame().
                {
                    TraceLocalActivity(waitEndFrame);
                    TraceLoggingWriteStart(waitEndFrame,
                                           "WaitEndFrame",
                                           TLArg(m_frameWaited, "FrameWaited"),
                                           TLArg(m_frameBegun, "FrameBegun"),
                                           TLArg(m_frameCompleted, "FrameCompleted"));
                    m_frameCondVar.wait(lock, [&] { return m_frameCompleted == m_frameBegun; });
                    TraceLoggingWriteStop(waitEndFrame, "WaitEndFrame");
                }
            } else {
                frameDiscarded = true;
            }

            // Tell OVR we are about to begin the frame.
            const long long ovrFrameId = m_frameWaited - 1;
            if (!m_useAsyncSubmission) {
                TraceLocalActivity(beginFrame);
                TraceLoggingWriteStart(beginFrame, "OVR_BeginFrame", TLArg(ovrFrameId, "FrameId"));
                CHECK_OVRCMD(ovr_BeginFrame(m_ovrSession, ovrFrameId));
                TraceLoggingWriteStop(beginFrame, "OVR_BeginFrame");
            }

            // Per spec: "A successful call to xrBeginFrame again with no intervening xrEndFrame call must result in the
            // success code XR_FRAME_DISCARDED being returned from xrBeginFrame. In this case it is assumed that the
            // xrBeginFrame refers to the next frame and the previously begun frame is forfeited by the application."
            // Therefore, we always advance m_frameBegun even upon discard.
            m_frameBegun = m_frameWaited;

            if (IsTraceEnabled()) {
                waitTimer.stop();
            }

            TraceLoggingWrite(g_traceProvider,
                              "BeginFrame",
                              TLArg(frameDiscarded, "FrameDiscarded"),
                              TLArg(waitTimer.query(), "WaitDurationUs"));

            // Statistics for the previous frame.
            // Our principle is to always query() a timer before we start() it. This means that we get measurements
            // with k_numGpuTimers frames latency.
            m_lastGpuFrameTimeUs = m_gpuTimerApp[m_currentTimerIndex] ? m_gpuTimerApp[m_currentTimerIndex]->query() : 0;

            if (m_frameCompleted > 0) {
                TraceLoggingWrite(g_traceProvider,
                                  "App_Statistics",
                                  TLArg(m_frameCompleted - 1, "FrameId"),
                                  TLArg(m_renderTimerApp.query(), "AppRenderCpuTime"));
            }

            if (m_frameCompleted >= k_numGpuTimers) {
                TraceLoggingWrite(g_traceProvider,
                                  "App_Statistics",
                                  TLArg(m_frameCompleted - k_numGpuTimers, "FrameId"),
                                  TLArg(m_lastGpuFrameTimeUs, "AppRenderGpuTime"));
            }

            // Start app timers.
            m_renderTimerApp.start();
            if (m_gpuTimerApp[m_currentTimerIndex]) {
                m_gpuTimerApp[m_currentTimerIndex]->start();
            }

            // Signal xrWaitFrame().
            TraceLoggingWrite(g_traceProvider,
                              "BeginFrame_Signal",
                              TLArg(m_frameWaited, "FrameWaited"),
                              TLArg(m_frameBegun, "FrameBegun"),
                              TLArg(m_frameCompleted, "FrameCompleted"));
            m_frameCondVar.notify_all();

            bool isAsyncReprojectionActive = false;
            ovrPerfStats stats{};
            if (OVR_SUCCESS(ovr_GetPerfStats(m_ovrSession, &stats))) {
                isAsyncReprojectionActive = stats.FrameStatsCount > 0 && stats.FrameStats[0].AswIsActive;
                TraceLoggingWrite(
                    g_traceProvider, "OVR_AswStatus", TLArg(isAsyncReprojectionActive, "AsyncReprojectionActive"));
            }

            if (isAsyncReprojectionActive) {
                m_predictedFrameDuration = m_idealFrameDuration * 2.f;
            } else {
                m_predictedFrameDuration = m_idealFrameDuration;
            }
        }

        return !frameDiscarded ? XR_SUCCESS : XR_FRAME_DISCARDED;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEndFrame
    XrResult OpenXrRuntime::xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
        if (frameEndInfo->type != XR_TYPE_FRAME_END_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrEndFrame",
                          TLXArg(session, "Session"),
                          TLArg(frameEndInfo->displayTime, "DisplayTime"),
                          TLArg(xr::ToCString(frameEndInfo->environmentBlendMode), "EnvironmentBlendMode"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_sessionBegun || m_sessionState == XR_SESSION_STATE_IDLE || m_sessionState == XR_SESSION_STATE_EXITING) {
            return XR_ERROR_SESSION_NOT_RUNNING;
        }

        if (m_isHeadless && frameEndInfo->layerCount) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        if (frameEndInfo->environmentBlendMode != XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
            return XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED;
        }

        if (frameEndInfo->displayTime <= 0) {
            return XR_ERROR_TIME_INVALID;
        }

        if (frameEndInfo->layerCount > ovrMaxLayerCount) {
            return XR_ERROR_LAYER_LIMIT_EXCEEDED;
        }

        // Critical section.
        {
            std::unique_lock lock1(m_swapchainsMutex);
            std::unique_lock lock2(m_frameMutex);

            if (m_frameBegun == m_frameCompleted) {
                return XR_ERROR_CALL_ORDER_INVALID;
            }

            m_renderTimerApp.stop();
            if (m_gpuTimerApp[m_currentTimerIndex]) {
                m_gpuTimerApp[m_currentTimerIndex]->stop();
            }

            // Make sure the previous frame finished submission.
            if (m_useAsyncSubmission) {
                waitForAsyncSubmissionIdle();

                // From this point, we know that the asynchronous thread is waiting, and we may use the submission
                // context.
            }

            // Serializes the app work between D3D12/Vulkan and D3D11.
            if (isD3D12Session()) {
                serializeD3D12Frame();
            } else if (isVulkanSession()) {
                serializeVulkanFrame();
            } else if (isOpenGLSession()) {
                serializeOpenGLFrame();
            } else if (!m_isHeadless) {
                serializeD3D11Frame();
            }

            // Ensure that we always restore the application device context if needed.
            auto scopeGuard = MakeScopeGuard([&] {
                if (m_d3d11ContextState) {
                    m_d3d11Context->SwapDeviceContextState(m_d3d11ContextState.Get(), nullptr);
                    m_d3d11ContextState.Reset();
                }
            });

            const auto lastPrecompositionTime = m_gpuTimerPrecomposition[m_currentTimerIndex]
                                                    ? m_gpuTimerPrecomposition[m_currentTimerIndex]->query()
                                                    : 0;
            if (IsTraceEnabled() && m_gpuTimerPrecomposition) {
                m_gpuTimerPrecomposition[m_currentTimerIndex]->start();
            }

            m_precompositor.displayTime = frameEndInfo->displayTime;
            m_precompositor.isFirstProjectionLayer = true;
            m_precompositor.resolvedSwapchainImages.clear();

            // Construct the list of layers.
            std::vector<ovrLayer_Union> layersAllocator;
            layersAllocator.reserve(frameEndInfo->layerCount + 1);
            for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                if (!frameEndInfo->layers[i]) {
                    return XR_ERROR_LAYER_INVALID;
                }

                std::shared_lock lock3(m_actionsAndSpacesMutex);

                if (!m_spaces.count(frameEndInfo->layers[i]->space)) {
                    return XR_ERROR_HANDLE_INVALID;
                }

                layersAllocator.push_back({});
                auto& layer = layersAllocator.back();
                layer.Header.Flags = 0;

                m_precompositor.layerIndex = i;

                // OpenGL needs to flip the texture vertically, which OVR can conveniently do for us.
                if (isOpenGLSession()) {
                    layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
                }

                if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
                    const XrCompositionLayerProjection* proj =
                        reinterpret_cast<const XrCompositionLayerProjection*>(frameEndInfo->layers[i]);

                    const XrResult result = handleProjectionLayer(*proj, layer);
                    if (XR_FAILED(result)) {
                        return result;
                    }

                    m_precompositor.isFirstProjectionLayer = false;

                } else if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_QUAD ||
                           (has_XR_KHR_composition_layer_cylinder &&
                            frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR)) {
                    const XrCompositionLayerQuad* quad =
                        reinterpret_cast<const XrCompositionLayerQuad*>(frameEndInfo->layers[i]);
                    const XrCompositionLayerCylinderKHR* cylinder =
                        reinterpret_cast<const XrCompositionLayerCylinderKHR*>(frameEndInfo->layers[i]);

                    const XrResult result = handleQuadCylinderLayer(*quad, *cylinder, layer);
                    if (XR_FAILED(result)) {
                        return result;
                    }

                } else if (has_XR_KHR_composition_layer_cube &&
                           frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_CUBE_KHR) {
                    const XrCompositionLayerCubeKHR* cube =
                        reinterpret_cast<const XrCompositionLayerCubeKHR*>(frameEndInfo->layers[i]);

                    const XrResult result = handleCubeLayer(*cube, layer);
                    if (XR_FAILED(result)) {
                        return result;
                    }

                } else {
                    return XR_ERROR_LAYER_INVALID;
                }
            }

            // Mark all swapchain images as clean (aka already pre-processed).
            for (auto resolvedSwapchain : m_precompositor.resolvedSwapchainImages) {
                resolvedSwapchain.first->dirty = false;
            }

            // Add a dummy layer so we can still call ovr_endFrame() for timing purposes.
            if (layersAllocator.empty()) {
                layersAllocator.push_back({});
                layersAllocator.back().Header.Type = ovrLayerType_Disabled;
            }

            if (IsTraceEnabled() && m_gpuTimerPrecomposition) {
                m_gpuTimerPrecomposition[m_currentTimerIndex]->stop();
            }

            // Update the FPS counter.
            const auto now = ovr_GetTimeInSeconds();
            m_frameTimes.push_back(now);
            while (now - m_frameTimes.front() >= 1.0) {
                m_frameTimes.pop_front();
            }

            // Inform Virtual Desktop of the measured application GPU work duration.
            // Ignore return code since this is a non-standard option.
            if (!m_isHeadless && !m_useOculusRuntime) {
                ovr_SetFloat(m_ovrSession, "AppGpuTime", m_lastGpuFrameTimeUs / 1e6f);
            }

            // Submit the layers to OVR.
            const long long ovrFrameId = m_frameBegun - 1;
            if (!m_useAsyncSubmission) {
                std::vector<ovrLayerHeader*> layers;
                for (auto& layer : layersAllocator) {
                    layers.push_back(&layer.Header);

                    if (layers.size() == ovrMaxLayerCount) {
                        ErrorLog("Too many layers in this frame (%u)\n", layersAllocator.size());
                        break;
                    }
                }

                TraceLocalActivity(endFrame);
                TraceLoggingWriteStart(endFrame,
                                       "OVR_EndFrame",
                                       TLArg(ovrFrameId, "FrameId"),
                                       TLArg(layers.size(), "NumLayers"),
                                       TLArg(m_frameTimes.size(), "Fps"),
                                       TLArg(lastPrecompositionTime, "LastPrecompositionTimeUs"));
                ovrViewScaleDesc scaleDesc{};
                scaleDesc.HmdToEyePose[xr::StereoView::Left] = m_cachedEyeInfo[xr::StereoView::Left].HmdToEyePose;
                scaleDesc.HmdToEyePose[xr::StereoView::Right] = m_cachedEyeInfo[xr::StereoView::Right].HmdToEyePose;
                scaleDesc.HmdSpaceToWorldScaleInMeters = 1.f;
                CHECK_OVRCMD(
                    ovr_EndFrame(m_ovrSession, ovrFrameId, &scaleDesc, layers.data(), (unsigned int)layers.size()));
                TraceLoggingWriteStop(endFrame, "OVR_EndFrame");
            }

            // Defer initialization of mirror window resources until they are first needed.
            try {
                if (!m_isHeadless && m_useMirrorWindow && !m_mirrorWindowThread.joinable()) {
                    createMirrorWindow();
                }
                updateMirrorWindow(m_precompositor.isProj0SRGB);
            } catch (std::exception& exc) {
                TraceLoggingWrite(g_traceProvider, "MirrorWindow", TLArg(exc.what(), "Error"));
                ErrorLog("Failed to update the mirror window: %s\n", exc.what());
            }

            // When using RenderDoc, signal a frame through the dummy swapchain.
            if (m_dxgiSwapchain) {
                m_dxgiSwapchain->Present(0, 0);
                m_ovrSubmissionContext->Flush();
            }

            if (m_useAsyncSubmission) {
                TraceLoggingWrite(g_traceProvider,
                                  "SubmitLayers",
                                  TLArg(ovrFrameId, "FrameId"),
                                  TLArg(m_frameTimes.size(), "Fps"),
                                  TLArg(lastPrecompositionTime, "LastPrecompositionTimeUs"));

                std::unique_lock lock(m_asyncSubmissionMutex);
                m_layersForAsyncSubmission = layersAllocator;

                m_asyncSubmissionCondVar.notify_all();

                // From this point, we know that the asynchronous thread may be executing, and we shall not use the
                // submission context.
            }

            m_frameCompleted = m_frameBegun;
            updateSessionState();

            m_currentTimerIndex = (m_currentTimerIndex + 1) % k_numGpuTimers;

            m_sessionTotalFrameCount++;

            // Signal xrBeginFrame().
            TraceLoggingWrite(g_traceProvider,
                              "EndFrame_Signal",
                              TLArg(m_frameWaited, "FrameWaited"),
                              TLArg(m_frameBegun, "FrameBegun"),
                              TLArg(m_frameCompleted, "FrameCompleted"));
            m_frameCondVar.notify_all();
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntime::handleProjectionLayer(const XrCompositionLayerProjection& proj, ovrLayer_Union& layer) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEndFrame_Layer",
                          TLArg("Proj", "Type"),
                          TLArg(proj.layerFlags, "Flags"),
                          TLXArg(proj.space, "Space"));

        if (proj.viewCount != xr::StereoView::Count) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // Make sure that we can use the EyeFov part of EyeFovDepth equivalently.
        static_assert(offsetof(decltype(layer.EyeFov), ColorTexture) ==
                      offsetof(decltype(layer.EyeFovDepth), ColorTexture));
        static_assert(offsetof(decltype(layer.EyeFov), Viewport) == offsetof(decltype(layer.EyeFovDepth), Viewport));
        static_assert(offsetof(decltype(layer.EyeFov), Fov) == offsetof(decltype(layer.EyeFovDepth), Fov));
        static_assert(offsetof(decltype(layer.EyeFov), RenderPose) ==
                      offsetof(decltype(layer.EyeFovDepth), RenderPose));
        static_assert(offsetof(decltype(layer.EyeFov), SensorSampleTime) ==
                      offsetof(decltype(layer.EyeFovDepth), SensorSampleTime));

        // Start without depth. We might change the type to ovrLayerType_EyeFovDepth further below.
        layer.Header.Type = ovrLayerType_EyeFov;

        Swapchain* swapchains[xr::StereoView::Count] = {};
        const XrSwapchainSubImage* subImages[xr::StereoView::Count] = {};

        for (uint32_t viewIndex = 0; viewIndex < xr::StereoView::Count; viewIndex++) {
            TraceLoggingWrite(g_traceProvider,
                              "xrEndFrame_View",
                              TLArg("Proj", "Type"),
                              TLArg(viewIndex, "ViewIndex"),
                              TLXArg(proj.views[viewIndex].subImage.swapchain, "Swapchain"),
                              TLArg(proj.views[viewIndex].subImage.imageArrayIndex, "ImageArrayIndex"),
                              TLArg(xr::ToString(proj.views[viewIndex].subImage.imageRect).c_str(), "ImageRect"),
                              TLArg(xr::ToString(proj.views[viewIndex].pose).c_str(), "Pose"),
                              TLArg(xr::ToString(proj.views[viewIndex].fov).c_str(), "Fov"));

            if (!Quaternion::IsNormalized(proj.views[viewIndex].pose.orientation)) {
                return XR_ERROR_POSE_INVALID;
            }

            if (!m_swapchains.count(proj.views[viewIndex].subImage.swapchain)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            Swapchain& xrSwapchain = *(Swapchain*)proj.views[viewIndex].subImage.swapchain;

            if (xrSwapchain.lastReleasedIndex == -1) {
                return XR_ERROR_LAYER_INVALID;
            }

            if (proj.views[viewIndex].subImage.imageArrayIndex >= xrSwapchain.xrDesc.arraySize ||
                xrSwapchain.xrDesc.faceCount != 1) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (m_precompositor.isFirstProjectionLayer) {
                m_precompositor.isProj0SRGB = isSRGBFormat(xrSwapchain.dxgiFormatForSubmission);
            }

            // We only upscale the bottom projection layer and only the focus view (when applicable).
            const bool canUpscale = std::abs(m_upscalingMultiplier - 1.f) > FLT_EPSILON;
            const bool canSharpen = m_sharpenFactor > 0.f;
            const bool needUpscaling = m_precompositor.isFirstProjectionLayer && (canUpscale || canSharpen);

            // Fill out color buffer information.
            resolveSwapchainImage(xrSwapchain,
                                  proj.views[viewIndex].subImage.imageArrayIndex,
                                  m_precompositor.resolvedSwapchainImages,
                                  needUpscaling /* Skip committing if we will not use the swapchain directly */);
            layer.EyeFov.ColorTexture[viewIndex] =
                xrSwapchain.resolvedSlices[proj.views[viewIndex].subImage.imageArrayIndex].ovrSwapchain;

            if (!isValidSwapchainRect(xrSwapchain.ovrDesc, proj.views[viewIndex].subImage.imageRect)) {
                return XR_ERROR_SWAPCHAIN_RECT_INVALID;
            }

            preprocessSwapchainImage(xrSwapchain,
                                     m_precompositor.layerIndex,
                                     proj.views[viewIndex].subImage.imageArrayIndex,
                                     proj.layerFlags,
                                     proj.views[viewIndex].subImage.imageRect);

            layer.EyeFov.Viewport[viewIndex].Pos.x = proj.views[viewIndex].subImage.imageRect.offset.x;
            layer.EyeFov.Viewport[viewIndex].Pos.y = proj.views[viewIndex].subImage.imageRect.offset.y;
            layer.EyeFov.Viewport[viewIndex].Size.w = proj.views[viewIndex].subImage.imageRect.extent.width;
            layer.EyeFov.Viewport[viewIndex].Size.h = proj.views[viewIndex].subImage.imageRect.extent.height;

            if (needUpscaling) {
                swapchains[viewIndex] = &xrSwapchain;
                subImages[viewIndex] = &proj.views[viewIndex].subImage;
            }

            // Fill out pose and FOV information.
            XrPosef layerPose;
            locateSpace(*(Space*)proj.space, *m_originSpace, m_precompositor.displayTime, layerPose);
            layer.EyeFov.RenderPose[viewIndex] = xrPoseToOvrPose(Pose::Multiply(proj.views[viewIndex].pose, layerPose));

            XrFovf fov = proj.views[viewIndex].fov;
            layer.EyeFov.Fov[viewIndex].DownTan = -tan(fov.angleDown);
            layer.EyeFov.Fov[viewIndex].UpTan = tan(fov.angleUp);
            layer.EyeFov.Fov[viewIndex].LeftTan = -tan(fov.angleLeft);
            layer.EyeFov.Fov[viewIndex].RightTan = tan(fov.angleRight);

            // In the case of OpenXR, we expect the app to use the predictedDisplayTime to query the
            // head pose, and pass that same time as displayTime.
            layer.EyeFov.SensorSampleTime = xrTimeToOvrTime(m_precompositor.displayTime);

            // Submit depth.
            if (has_XR_KHR_composition_layer_depth) {
                const XrBaseInStructure* entry = reinterpret_cast<const XrBaseInStructure*>(proj.views[viewIndex].next);
                while (entry) {
                    if (entry->type == XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR) {
                        const XrCompositionLayerDepthInfoKHR* depth =
                            reinterpret_cast<const XrCompositionLayerDepthInfoKHR*>(entry);

                        TraceLoggingWrite(g_traceProvider,
                                          "xrEndFrame_View",
                                          TLArg("Depth", "Type"),
                                          TLArg(viewIndex, "ViewIndex"),
                                          TLXArg(depth->subImage.swapchain, "Swapchain"),
                                          TLArg(depth->subImage.imageArrayIndex, "ImageArrayIndex"),
                                          TLArg(xr::ToString(depth->subImage.imageRect).c_str(), "ImageRect"),
                                          TLArg(depth->nearZ, "Near"),
                                          TLArg(depth->farZ, "Far"),
                                          TLArg(depth->minDepth, "MinDepth"),
                                          TLArg(depth->maxDepth, "MaxDepth"));

                        // Some games (like WRC) will not properly submit depth. We bypass all the checks if the runtime
                        // does not care about depth.
                        if (m_shouldUseDepth || m_isConformanceTest) {
                            layer.Header.Type = ovrLayerType_EyeFovDepth;

                            if (!m_swapchains.count(depth->subImage.swapchain)) {
                                return XR_ERROR_HANDLE_INVALID;
                            }

                            Swapchain& xrDepthSwapchain = *(Swapchain*)depth->subImage.swapchain;

                            if (xrDepthSwapchain.lastReleasedIndex == -1) {
                                return XR_ERROR_LAYER_INVALID;
                            }

                            if (depth->subImage.imageArrayIndex >= xrDepthSwapchain.xrDesc.arraySize ||
                                xrSwapchain.xrDesc.faceCount != 1) {
                                return XR_ERROR_VALIDATION_FAILURE;
                            }

                            // Fill out depth buffer information.
                            resolveSwapchainImage(xrDepthSwapchain,
                                                  depth->subImage.imageArrayIndex,
                                                  m_precompositor.resolvedSwapchainImages);
                            layer.EyeFovDepth.DepthTexture[viewIndex] =
                                xrDepthSwapchain.resolvedSlices[depth->subImage.imageArrayIndex].ovrSwapchain;

                            // TODO: We don't enforce that the viewport must match the color buffer.
                            if (!isValidSwapchainRect(xrDepthSwapchain.ovrDesc, depth->subImage.imageRect)) {
                                return XR_ERROR_SWAPCHAIN_RECT_INVALID;
                            }

                            // Fill out projection information.
                            layer.EyeFovDepth.ProjectionDesc.Projection22 = depth->farZ / (depth->nearZ - depth->farZ);
                            layer.EyeFovDepth.ProjectionDesc.Projection23 =
                                (depth->farZ * depth->nearZ) / (depth->nearZ - depth->farZ);
                            layer.EyeFovDepth.ProjectionDesc.Projection32 = -1.f;
                        } else {
                            TraceLoggingWrite(g_traceProvider, "xrEndFrame_View_IgnoreDepth");
                        }

                        break;
                    }
                    entry = entry->next;
                }
            }
        }

        // Run the upscaler or sharpening if needed.
        if (swapchains[xr::StereoView::Right]) {
            upscaler(swapchains, subImages, layer.EyeFov);
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntime::handleQuadCylinderLayer(const XrCompositionLayerQuad& quad,
                                                    const XrCompositionLayerCylinderKHR& cylinder,
                                                    ovrLayer_Union& layer) {
        const bool isCylinder = quad.type == XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR;

        // Make sure that we can use the XrCompositionLayerQuad part of XrCompositionLayerCylinderKHR
        // equivalently.
        static_assert(offsetof(XrCompositionLayerQuad, layerFlags) ==
                      offsetof(XrCompositionLayerCylinderKHR, layerFlags));
        static_assert(offsetof(XrCompositionLayerQuad, space) == offsetof(XrCompositionLayerCylinderKHR, space));
        static_assert(offsetof(XrCompositionLayerQuad, eyeVisibility) ==
                      offsetof(XrCompositionLayerCylinderKHR, eyeVisibility));
        static_assert(offsetof(XrCompositionLayerQuad, subImage) == offsetof(XrCompositionLayerCylinderKHR, subImage));
        static_assert(offsetof(XrCompositionLayerQuad, pose) == offsetof(XrCompositionLayerCylinderKHR, pose));

        TraceLoggingWrite(g_traceProvider,
                          "xrEndFrame_Layer",
                          TLArg(!isCylinder ? "Quad" : "Cylinder", "Type"),
                          TLArg(quad.layerFlags, "Flags"),
                          TLXArg(quad.space, "Space"));
        if (!isCylinder) {
            TraceLoggingWrite(g_traceProvider,
                              "xrEndFrame_View",
                              TLArg("Quad", "Type"),
                              TLXArg(quad.subImage.swapchain, "Swapchain"),
                              TLArg(quad.subImage.imageArrayIndex, "ImageArrayIndex"),
                              TLArg(xr::ToString(quad.subImage.imageRect).c_str(), "ImageRect"),
                              TLArg(xr::ToString(quad.pose).c_str(), "Pose"),
                              TLArg(quad.size.width, "Width"),
                              TLArg(quad.size.height, "Height"),
                              TLArg(xr::ToCString(quad.eyeVisibility), "EyeVisibility"));
        } else {
            TraceLoggingWrite(g_traceProvider,
                              "xrEndFrame_View",
                              TLArg("Cylinder", "Type"),
                              TLXArg(cylinder.subImage.swapchain, "Swapchain"),
                              TLArg(cylinder.subImage.imageArrayIndex, "ImageArrayIndex"),
                              TLArg(xr::ToString(cylinder.subImage.imageRect).c_str(), "ImageRect"),
                              TLArg(xr::ToString(cylinder.pose).c_str(), "Pose"),
                              TLArg(cylinder.radius, "Radius"),
                              TLArg(cylinder.centralAngle, "CentralAngle"),
                              TLArg(cylinder.aspectRatio, "AspectRatio"),
                              TLArg(xr::ToCString(cylinder.eyeVisibility), "EyeVisibility"));
        }

        // Make sure that we can use the Quad part of Cylinder equivalently.
        static_assert(offsetof(decltype(layer.Quad), ColorTexture) == offsetof(decltype(layer.Cylinder), ColorTexture));
        static_assert(offsetof(decltype(layer.Quad), Viewport) == offsetof(decltype(layer.Cylinder), Viewport));
        static_assert(offsetof(decltype(layer.Quad), QuadPoseCenter) ==
                      offsetof(decltype(layer.Cylinder), CylinderPoseCenter));

        layer.Header.Type = isCylinder ? ovrLayerType_Cylinder : ovrLayerType_Quad;

        if (!Quaternion::IsNormalized(quad.pose.orientation)) {
            return XR_ERROR_POSE_INVALID;
        }

        if (!m_swapchains.count(quad.subImage.swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)quad.subImage.swapchain;

        if (xrSwapchain.lastReleasedIndex == -1) {
            return XR_ERROR_LAYER_INVALID;
        }

        // CONFORMANCE: We ignore eyeVisibility, since there is no equivalent in the OVR compositor.
        // We cannot achieve conformance for this particular (but uncommon) API usage.

        if (quad.subImage.imageArrayIndex >= xrSwapchain.xrDesc.arraySize || xrSwapchain.xrDesc.faceCount != 1) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // Fill out color buffer information.
        resolveSwapchainImage(xrSwapchain, quad.subImage.imageArrayIndex, m_precompositor.resolvedSwapchainImages);
        layer.Quad.ColorTexture = xrSwapchain.resolvedSlices[quad.subImage.imageArrayIndex].ovrSwapchain;

        if (!isValidSwapchainRect(xrSwapchain.ovrDesc, quad.subImage.imageRect)) {
            return XR_ERROR_SWAPCHAIN_RECT_INVALID;
        }

        preprocessSwapchainImage(xrSwapchain,
                                 m_precompositor.layerIndex,
                                 quad.subImage.imageArrayIndex,
                                 quad.layerFlags,
                                 quad.subImage.imageRect);

        layer.Quad.Viewport.Pos.x = quad.subImage.imageRect.offset.x;
        layer.Quad.Viewport.Pos.y = quad.subImage.imageRect.offset.y;
        layer.Quad.Viewport.Size.w = quad.subImage.imageRect.extent.width;
        layer.Quad.Viewport.Size.h = quad.subImage.imageRect.extent.height;

        if (!m_spaces.count(quad.space)) {
            return XR_ERROR_HANDLE_INVALID;
        }
        Space& xrSpace = *(Space*)quad.space;

        // Fill out pose and quad information.
        if (xrSpace.referenceType != XR_REFERENCE_SPACE_TYPE_VIEW) {
            XrPosef layerPose;
            locateSpace(*(Space*)quad.space, *m_originSpace, m_precompositor.displayTime, layerPose);
            layer.Quad.QuadPoseCenter = xrPoseToOvrPose(Pose::Multiply(quad.pose, layerPose));
        } else {
            layer.Quad.QuadPoseCenter = xrPoseToOvrPose(Pose::Multiply(quad.pose, xrSpace.poseInSpace));
            layer.Header.Flags |= ovrLayerFlag_HeadLocked;
        }

        if (!isCylinder) {
            layer.Quad.QuadSize.x = quad.size.width;
            layer.Quad.QuadSize.y = quad.size.height;
        } else {
            layer.Cylinder.CylinderRadius = cylinder.radius;
            layer.Cylinder.CylinderAngle = cylinder.centralAngle;
            layer.Cylinder.CylinderAspectRatio = cylinder.aspectRatio;
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntime::handleCubeLayer(const XrCompositionLayerCubeKHR& cube, ovrLayer_Union& layer) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEndFrame_Layer",
                          TLArg("Cube", "Type"),
                          TLArg(cube.layerFlags, "Flags"),
                          TLXArg(cube.space, "Space"));
        TraceLoggingWrite(g_traceProvider,
                          "xrEndFrame_View",
                          TLArg("Cube", "Type"),
                          TLXArg(cube.swapchain, "Swapchain"),
                          TLArg(cube.imageArrayIndex, "ImageArrayIndex"),
                          TLArg(xr::ToString(cube.orientation).c_str(), "Pose"),
                          TLArg(xr::ToCString(cube.eyeVisibility), "EyeVisibility"));
        layer.Header.Type = ovrLayerType_Cube;

        if (!Quaternion::IsNormalized(cube.orientation)) {
            return XR_ERROR_POSE_INVALID;
        }

        if (!m_swapchains.count(cube.swapchain)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Swapchain& xrSwapchain = *(Swapchain*)cube.swapchain;

        if (xrSwapchain.lastReleasedIndex == -1) {
            return XR_ERROR_LAYER_INVALID;
        }

        // CONFORMANCE: We ignore eyeVisibility, since there is no equivalent in the OVR compositor.
        // We cannot achieve conformance for this particular (but uncommon) API usage.

        if (cube.imageArrayIndex != 0 || xrSwapchain.xrDesc.faceCount != 6) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // Fill out color buffer information.
        resolveSwapchainImage(xrSwapchain, 0, m_precompositor.resolvedSwapchainImages);
        layer.Cube.CubeMapTexture = xrSwapchain.resolvedSlices[0].ovrSwapchain;

        preprocessSwapchainImage(xrSwapchain,
                                 m_precompositor.layerIndex,
                                 0,
                                 cube.layerFlags,
                                 {{0, 0}, {(int32_t)xrSwapchain.xrDesc.width, (int32_t)xrSwapchain.xrDesc.height}});

        if (!m_spaces.count(cube.space)) {
            return XR_ERROR_HANDLE_INVALID;
        }
        Space& xrSpace = *(Space*)cube.space;

        // Fill out the rotation.
        if (xrSpace.referenceType != XR_REFERENCE_SPACE_TYPE_VIEW) {
            XrPosef layerPose;
            locateSpace(*(Space*)cube.space, *m_originSpace, m_precompositor.displayTime, layerPose);
            layer.Cube.Orientation =
                xrPoseToOvrPose(Pose::Multiply(Pose::MakePose(cube.orientation, XrVector3f{0, 0, 0}), layerPose))
                    .Orientation;
        } else {
            layer.Cube.Orientation =
                xrPoseToOvrPose(
                    Pose::Multiply(Pose::MakePose(cube.orientation, XrVector3f{0, 0, 0}), xrSpace.poseInSpace))
                    .Orientation;
            layer.Header.Flags |= ovrLayerFlag_HeadLocked;
        }

        return XR_SUCCESS;
    }

    void OpenXrRuntime::preprocessSwapchainImage(Swapchain& xrSwapchain,
                                                 uint32_t layerIndex,
                                                 uint32_t slice,
                                                 XrCompositionLayerFlags compositionFlags,
                                                 XrRect2Di viewport) {
        if (!xrSwapchain.dirty) {
            return;
        }

        const bool needClearAlpha =
            (layerIndex > 0 && !(compositionFlags & XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT));
        // Workaround: this is questionable, but an app should always submit layer 0 without alpha-blending (ie: alpha =
        // 1). This avoids needing to run the premultiply alpha shader only do multiply all values by 1...
        const bool needPremultiplyAlpha =
            layerIndex > 0 && (compositionFlags & XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT);

        const int ovrDestIndex = xrSwapchain.resolvedSlices[slice].lastCommittedIndex;

        TraceLoggingWrite(g_traceProvider,
                          "PreprocessSwapchainImage",
                          TLArg(ovrDestIndex, "DestIndex"),
                          TLArg(slice, "Slice"),
                          TLArg(needClearAlpha, "NeedClearAlpha"),
                          TLArg(needPremultiplyAlpha, "needPremultiplyAlpha"));

        if (needClearAlpha || needPremultiplyAlpha) {
            // Circumvent some of OVR's limitations:
            // - For alpha-blended layers, we must pre-process the alpha channel.

            ensurePreprocessResources();

            // We are about to do something destructive to the application context. Save the context. It will be
            // restored at the end of xrEndFrame().
            if (m_d3d11Device == m_ovrSubmissionDevice && !m_d3d11ContextState) {
                m_ovrSubmissionContext->SwapDeviceContextState(m_ovrSubmissionContextState.Get(),
                                                               m_d3d11ContextState.ReleaseAndGetAddressOf());
            }

            m_ovrSubmissionContext->CSSetShader(m_alphaCorrectShader.Get(), nullptr, 0);
            {
                AlphaBlendingCSConstants constants{};
                constants.offset = viewport.offset;
                constants.dimension = viewport.extent;
                constants.ignoreAlpha = needClearAlpha;
                constants.isUnpremultipliedAlpha = needPremultiplyAlpha;
                constants.isSRGB = isSRGBFormat((DXGI_FORMAT)xrSwapchain.xrDesc.format);

                D3D11_MAPPED_SUBRESOURCE mappedResources;
                CHECK_HRCMD(m_ovrSubmissionContext->Map(
                    m_alphaCorrectConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResources));
                memcpy(mappedResources.pData, &constants, sizeof(constants));
                m_ovrSubmissionContext->Unmap(m_alphaCorrectConstants.Get(), 0);
                m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, m_alphaCorrectConstants.GetAddressOf());
            }

            if (xrSwapchain.resolvedSlices[slice].uavs.size() <= ovrDestIndex) {
                xrSwapchain.resolvedSlices[slice].uavs.resize(ovrDestIndex + 1);
            }
            if (!xrSwapchain.resolvedSlices[slice].uavs[ovrDestIndex]) {
                D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                desc.Format = getUnorderedAccessViewFormat(xrSwapchain.dxgiFormatForSubmission);
                CHECK_HRCMD(m_ovrSubmissionDevice->CreateUnorderedAccessView(
                    xrSwapchain.resolvedSlices[slice].images[ovrDestIndex].Get(),
                    &desc,
                    xrSwapchain.resolvedSlices[slice].uavs[ovrDestIndex].ReleaseAndGetAddressOf()));
                setDebugName(xrSwapchain.resolvedSlices[slice].uavs[ovrDestIndex].Get(),
                             fmt::format("Runtime Slice UAV[{}, {}, {}]", slice, ovrDestIndex, (void*)&xrSwapchain));
            }
            m_ovrSubmissionContext->CSSetUnorderedAccessViews(
                0, 1, xrSwapchain.resolvedSlices[slice].uavs[ovrDestIndex].GetAddressOf(), nullptr);

            m_ovrSubmissionContext->Dispatch((viewport.extent.width + 31) / 32, (viewport.extent.height + 31) / 32, 1);

            // Unbind all resources to avoid D3D validation errors.
            {
                m_ovrSubmissionContext->CSSetShader(nullptr, nullptr, 0);
                ID3D11Buffer* nullCBV[] = {nullptr};
                m_ovrSubmissionContext->CSSetConstantBuffers(0, 1, nullCBV);
                ID3D11UnorderedAccessView* nullUAV[] = {nullptr};
                m_ovrSubmissionContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
            }
        }
    }

    void OpenXrRuntime::ensurePreprocessResources() {
        CHECK_HRCMD(m_ovrSubmissionDevice->CreateComputeShader(
            g_AlphaBlendingCS, sizeof(g_AlphaBlendingCS), nullptr, m_alphaCorrectShader.ReleaseAndGetAddressOf()));
        setDebugName(m_alphaCorrectShader.Get(), "AlphaBlending CS");
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = ((sizeof(AlphaBlendingCSConstants) + 15) / 16) * 16;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            CHECK_HRCMD(
                m_ovrSubmissionDevice->CreateBuffer(&desc, nullptr, m_alphaCorrectConstants.ReleaseAndGetAddressOf()));
            setDebugName(m_alphaCorrectConstants.Get(), "AlphaBlending Constants");
        }
    }

    void OpenXrRuntime::asyncSubmissionThread() {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "AsyncSubmissionThread");

        SetThreadPriority(GetCurrentThread(),
                          getSetting("async_submission_priority").value_or(THREAD_PRIORITY_TIME_CRITICAL));

        std::optional<long long> lastWaitedFrameId;
        while (true) {
            const long long ovrFrameId = m_frameCompleted;
            {
                TraceLocalActivity(waitToBeginFrame);
                TraceLoggingWriteStart(waitToBeginFrame, "OVR_WaitToBeginFrame", TLArg(ovrFrameId, "FrameId"));
                const auto result = ovr_WaitToBeginFrame(m_ovrSession, ovrFrameId);
                TraceLoggingWriteStop(waitToBeginFrame, "OVR_WaitToBeginFrame", TLArg((int)result, "Result"));
                if (result == ovrError_Timeout) {
                    ErrorLog("Timeout in async submission thread! This is normal if you have a debugger attached.\n");
                } else if (result == ovrError_NotInitialized) {
                    ErrorLog("Not initialized in async sybmission thread! Retrying...\n");
                    std::this_thread::sleep_for(1ms);
                    continue;
                } else {
                    CHECK_OVRCMD(result);
                }
            }
            m_lastWaitToBeginFrameTime = std::chrono::high_resolution_clock::now();

            {
                TraceLocalActivity(beginFrame);
                TraceLoggingWriteStart(beginFrame, "OVR_BeginFrame", TLArg(ovrFrameId, "FrameId"));
                CHECK_OVRCMD(ovr_BeginFrame(m_ovrSession, ovrFrameId));
                TraceLoggingWriteStop(beginFrame, "OVR_BeginFrame");
            }

            {
                std::unique_lock lock(m_asyncSubmissionMutex);

                // Mark us as ready to accept a new frame.
                m_layersForAsyncSubmission.clear();
                m_asyncSubmissionCondVar.notify_all();

                // Wait for the frame.
                m_asyncSubmissionCondVar.wait(
                    lock, [&] { return m_terminateAsyncThread || !m_layersForAsyncSubmission.empty(); });
            }
            if (m_terminateAsyncThread) {
                break;
            }

            {
                std::vector<ovrLayerHeader*> layers;
                for (auto& layer : m_layersForAsyncSubmission) {
                    layers.push_back(&layer.Header);

                    if (layers.size() == ovrMaxLayerCount) {
                        ErrorLog("Too many layers in this frame (%u)\n", m_layersForAsyncSubmission.size());
                        break;
                    }
                }

                TraceLocalActivity(endFrame);
                TraceLoggingWriteStart(
                    endFrame, "OVR_EndFrame", TLArg(ovrFrameId, "FrameId"), TLArg(layers.size(), "NumLayers"));
                ovrViewScaleDesc scaleDesc{};
                scaleDesc.HmdToEyePose[xr::StereoView::Left] = m_cachedEyeInfo[xr::StereoView::Left].HmdToEyePose;
                scaleDesc.HmdToEyePose[xr::StereoView::Right] = m_cachedEyeInfo[xr::StereoView::Right].HmdToEyePose;
                scaleDesc.HmdSpaceToWorldScaleInMeters = 1.f;
                CHECK_OVRCMD(
                    ovr_EndFrame(m_ovrSession, ovrFrameId, &scaleDesc, layers.data(), (unsigned int)layers.size()));
                TraceLoggingWriteStop(endFrame, "OVR_EndFrame");
            }
        }

        TraceLoggingWriteStop(local, "AsyncSubmissionThread");
    }

    void OpenXrRuntime::waitForAsyncSubmissionIdle(bool doRunningStart) {
        TraceLocalActivity(waitToBeginFrame);
        TraceLoggingWriteStart(waitToBeginFrame, "WaitForAsyncSubmissionIdle", TLArg(doRunningStart, "DoRunningStart"));

        std::unique_lock lock(m_asyncSubmissionMutex);

        bool wokeUpEarly = false;
        if (doRunningStart) {
            constexpr double RunningStart = 0.002;
            const auto timeout =
                m_lastWaitToBeginFrameTime + std::chrono::duration<double>(m_predictedFrameDuration - RunningStart);

            wokeUpEarly =
                !m_asyncSubmissionCondVar.wait_until(lock, timeout, [&] { return m_layersForAsyncSubmission.empty(); });
        } else {
            m_asyncSubmissionCondVar.wait(lock, [&] { return m_layersForAsyncSubmission.empty(); });
        }

        TraceLoggingWriteStop(
            waitToBeginFrame, "WaitForAsyncSubmissionIdle", TLArg(wokeUpEarly, "WokeUpForRunningStart"));
    }

} // namespace virtualdesktop_openxr
