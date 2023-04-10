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
    using namespace DirectX;
    using namespace xr::math;

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
        CHECK_PVRCMD(pvr_getHmdStatus(m_pvrSession, &m_hmdStatus));
        TraceLoggingWrite(g_traceProvider,
                          "PVR_HmdStatus",
                          TLArg(!!m_hmdStatus.ServiceReady, "ServiceReady"),
                          TLArg(!!m_hmdStatus.HmdPresent, "HmdPresent"),
                          TLArg(!!m_hmdStatus.HmdMounted, "HmdMounted"),
                          TLArg(!!m_hmdStatus.IsVisible, "IsVisible"),
                          TLArg(!!m_hmdStatus.DisplayLost, "DisplayLost"),
                          TLArg(!!m_hmdStatus.ShouldQuit, "ShouldQuit"));
        if (!m_sessionLossPending) {
            m_sessionLossPending = !(m_hmdStatus.ServiceReady && m_hmdStatus.HmdPresent) || m_hmdStatus.DisplayLost ||
                                   m_hmdStatus.ShouldQuit;
        }
        updateSessionState();

        frameState->shouldRender =
            (!m_sessionStopping && !m_sessionExiting && !m_sessionLossPending && m_hmdStatus.IsVisible) ? XR_TRUE
                                                                                                        : XR_FALSE;

        // Critical section.
        {
            CpuTimer waitTimer;
            if (IsTraceEnabled()) {
                waitTimer.start();
            }

            std::unique_lock lock(m_frameLock);

            m_frameTimerApp.stop();
            m_lastCpuFrameTimeUs = m_frameTimerApp.query();

            TraceLoggingWrite(g_traceProvider,
                              "App_Statistics",
                              TLArg(m_frameCompleted, "FrameId"),
                              TLArg(m_lastCpuFrameTimeUs, "AppFrameCpuTime"));

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

            // Workaround: PVR cannot wait for a frame without having a device. If no swapchain was created up to this
            // point, we must create one to initialize PVR.
            if (!m_pvrSession->envh->pvr_dxgl_interface) {
                // Make as small as possible of a memory footprint...
                pvrTextureSwapChainDesc desc{};
                desc.Type = pvrTexture_2D;
                desc.StaticImage = true;
                desc.ArraySize = 1;
                desc.Width = desc.Height = 128;
                desc.MipLevels = 1;
                desc.SampleCount = 1;
                desc.Format = PVR_FORMAT_B8G8R8A8_UNORM;

                pvrTextureSwapChain tempSwapchain;
                CHECK_PVRCMD(
                    pvr_createTextureSwapChainDX(m_pvrSession, m_pvrSubmissionDevice.Get(), &desc, &tempSwapchain));

                // ...and free the memory right away.
                pvr_destroyTextureSwapChain(m_pvrSession, tempSwapchain);
            }

            // Workaround: PVR since Pimax Client 1.10 is not handling frame pipelining correctly.
            // Ensure a single frame in-flight when the prediction is far off.
            if (m_disableFramePipeliningQuirk) {
                TraceLocalActivity(waitEndFrame);
                TraceLoggingWriteStart(waitEndFrame,
                                       "WaitEndFrame",
                                       TLArg(m_frameWaited, "FrameWaited"),
                                       TLArg(m_frameBegun, "FrameBegun"),
                                       TLArg(m_frameCompleted, "FrameCompleted"));
                m_frameCondVar.wait(lock, [&] { return m_frameCompleted == m_frameBegun; });
                TraceLoggingWriteStop(waitEndFrame, "WaitEndFrame");
            }

            // Wait for PVR to be ready for the next frame.
            const long long pvrFrameId = m_frameWaited;
            {
                TraceLocalActivity(waitToBeginFrame);
                TraceLoggingWriteStart(waitToBeginFrame, "PVR_WaitToBeginFrame", TLArg(pvrFrameId, "FrameIndex"));
                // Workaround: PVR will occasionally fail with result code -1 (undocumented) and the following log
                // message:
                //   [PVR] wait rendering complete event failed:258
                // Let's ignore this for now and hope for the best.
                const auto result = pvr_waitToBeginFrame(m_pvrSession, pvrFrameId);
                if (result != pvr_success) {
                    ErrorLog("pvr_waitToBeginFrame() failed with code: %s\n", xr::ToString(result).c_str());
                }
                TraceLoggingWriteStop(
                    waitToBeginFrame, "PVR_WaitToBeginFrame", TLArg(xr::ToString(result).c_str(), "Result"));
            }

            if (IsTraceEnabled()) {
                waitTimer.stop();
            }

            const double now = pvr_getTimeSeconds(m_pvr);
            double predictedDisplayTime = pvr_getPredictedDisplayTime(m_pvrSession, pvrFrameId);
            TraceLoggingWrite(g_traceProvider,
                              "WaitFrame",
                              TLArg(now, "Now"),
                              TLArg(predictedDisplayTime, "PredictedDisplayTime"),
                              TLArg(predictedDisplayTime - now, "PhotonTime"),
                              TLArg(waitTimer.query(), "WaitDurationUs"));

            // Setup the app frame for use and the next frame for this call.
            frameState->predictedDisplayTime = pvrTimeToXrTime(predictedDisplayTime);

            // Workaround: during early calls, PVR times might violate OpenXR rules.
            if (frameState->predictedDisplayTime <= m_lastPredictedDisplayTime) {
                frameState->predictedDisplayTime = m_lastPredictedDisplayTime + 1;
            }
            m_lastPredictedDisplayTime = frameState->predictedDisplayTime;

            // We always use the native frame duration, regardless of Smart Smoothing.
            frameState->predictedDisplayPeriod = pvrTimeToXrTime(m_frameDuration);

            m_frameTimerApp.start();

            m_frameWaited++;

            TraceLoggingWrite(g_traceProvider,
                              "WaitFrame_State",
                              TLArg(m_frameWaited, "FrameWaited"),
                              TLArg(m_frameBegun, "FrameBegun"),
                              TLArg(m_frameCompleted, "FrameCompleted"));
        }

        m_telemetry.tick();

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

            std::unique_lock lock(m_frameLock);

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

            // Tell PVR we are about to begin the frame.
            const long long pvrFrameId = m_frameWaited - 1;
            {
                TraceLocalActivity(beginFrame);
                TraceLoggingWriteStart(beginFrame, "PVR_BeginFrame", TLArg(pvrFrameId, "FrameIndex"));
                // Workaround: PVR will occasionally fail with result code -1 (undocumented) and the following log
                // message:
                //   [PVR] wait rendering complete event failed:258
                // Let's ignore this for now and hope for the best.
                const auto result = pvr_beginFrame(m_pvrSession, pvrFrameId);
                if (result != pvr_success) {
                    ErrorLog("pvr_beginFrame() failed with code: %s\n", xr::ToString(result).c_str());
                }
                TraceLoggingWriteStop(beginFrame, "PVR_BeginFrame", TLArg(xr::ToString(result).c_str(), "Result"));
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
            if (m_useFrameTimingOverride || IsTraceEnabled()) {
                // Our principle is to always query() a timer before we start() it. This means that we get measurements
                // with k_numGpuTimers frames latency.
                m_lastGpuFrameTimeUs =
                    m_gpuTimerApp[m_currentTimerIndex] ? m_gpuTimerApp[m_currentTimerIndex]->query() : 0;

                TraceLoggingWrite(g_traceProvider,
                                  "App_Statistics",
                                  TLArg(m_frameCompleted, "FrameId"),
                                  TLArg(m_renderTimerApp.query(), "AppRenderCpuTime"));

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
            }

            // Signal xrWaitFrame().
            TraceLoggingWrite(g_traceProvider,
                              "BeginFrame_Signal",
                              TLArg(m_frameWaited, "FrameWaited"),
                              TLArg(m_frameBegun, "FrameBegun"),
                              TLArg(m_frameCompleted, "FrameCompleted"));
            m_frameCondVar.notify_one();

            TraceLoggingWrite(
                g_traceProvider,
                "PVR_Status",
                TLArg(!!pvr_getIntConfig(m_pvrSession, "dbg_asw_enable", 0), "EnableSmartSmoothing"),
                TLArg(pvr_getIntConfig(m_pvrSession, "dbg_force_framerate_divide_by", 1), "CompulsiveSmoothingRate"),
                TLArg(!!pvr_getIntConfig(m_pvrSession, "asw_available", 0), "SmartSmoothingAvailable"),
                TLArg(!!pvr_getIntConfig(m_pvrSession, "asw_active", 0), "SmartSmoothingActive"));
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

        if (frameEndInfo->environmentBlendMode != XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
            return XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED;
        }

        if (frameEndInfo->displayTime <= 0) {
            return XR_ERROR_TIME_INVALID;
        }

        if (frameEndInfo->layerCount > pvrMaxLayerCount) {
            return XR_ERROR_LAYER_LIMIT_EXCEEDED;
        }

        // Critical section.
        {
            std::unique_lock lock1(m_swapchainsLock);
            std::unique_lock lock2(m_frameLock);

            if (m_frameBegun == m_frameCompleted) {
                return XR_ERROR_CALL_ORDER_INVALID;
            }

            if (m_useFrameTimingOverride || IsTraceEnabled()) {
                m_renderTimerApp.stop();
                if (m_gpuTimerApp[m_currentTimerIndex]) {
                    m_gpuTimerApp[m_currentTimerIndex]->stop();
                }
            }

            // Serializes the app work between D3D12/Vulkan and D3D11.
            if (isD3D12Session()) {
                serializeD3D12Frame();
            } else if (isVulkanSession()) {
                serializeVulkanFrame();
            } else if (isOpenGLSession()) {
                serializeOpenGLFrame();
            } else {
                serializeD3D11Frame();
            }

            // Handle recentering via keyboard input when the app does not poll for motion controllers.
            if (!m_actionsSyncedThisFrame) {
                handleBuiltinActions();
            }
            m_actionsSyncedThisFrame = false;

            const auto lastPrecompositionTime = m_gpuTimerPrecomposition[m_currentTimerIndex]->query();
            if (IsTraceEnabled()) {
                m_gpuTimerPrecomposition[m_currentTimerIndex]->start();
            }

            std::set<std::pair<pvrTextureSwapChain, uint32_t>> committedSwapchainImages;

            // Construct the list of layers.
            std::vector<pvrLayer_Union> layersAllocator(frameEndInfo->layerCount + 1);
            std::vector<pvrLayerHeader*> layers;
            for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                if (!frameEndInfo->layers[i]) {
                    return XR_ERROR_LAYER_INVALID;
                }

                auto& layer = layersAllocator[i];
                layer.Header.Flags = 0;

                // OpenGL needs to flip the texture vertically, which PVR can conveniently do for us.
                if (isOpenGLSession()) {
                    layer.Header.Flags = pvrLayerFlag_TextureOriginAtBottomLeft;
                }

                if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
                    const XrCompositionLayerProjection* proj =
                        reinterpret_cast<const XrCompositionLayerProjection*>(frameEndInfo->layers[i]);

                    TraceLoggingWrite(g_traceProvider,
                                      "xrEndFrame_Layer",
                                      TLArg("Proj", "Type"),
                                      TLArg(proj->layerFlags, "Flags"),
                                      TLXArg(proj->space, "Space"));

                    if (proj->viewCount != xr::StereoView::Count) {
                        return XR_ERROR_VALIDATION_FAILURE;
                    }

                    // Make sure that we can use the EyeFov part of EyeFovDepth equivalently.
                    static_assert(offsetof(decltype(layer.EyeFov), ColorTexture) ==
                                  offsetof(decltype(layer.EyeFovDepth), ColorTexture));
                    static_assert(offsetof(decltype(layer.EyeFov), Viewport) ==
                                  offsetof(decltype(layer.EyeFovDepth), Viewport));
                    static_assert(offsetof(decltype(layer.EyeFov), Fov) == offsetof(decltype(layer.EyeFovDepth), Fov));
                    static_assert(offsetof(decltype(layer.EyeFov), RenderPose) ==
                                  offsetof(decltype(layer.EyeFovDepth), RenderPose));
                    static_assert(offsetof(decltype(layer.EyeFov), SensorSampleTime) ==
                                  offsetof(decltype(layer.EyeFovDepth), SensorSampleTime));

                    // Start without depth. We might change the type to pvrLayerType_EyeFovDepth further below.
                    layer.Header.Type = pvrLayerType_EyeFov;

                    for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                        TraceLoggingWrite(g_traceProvider,
                                          "xrEndFrame_View",
                                          TLArg("Proj", "Type"),
                                          TLArg(eye, "Index"),
                                          TLXArg(proj->views[eye].subImage.swapchain, "Swapchain"),
                                          TLArg(proj->views[eye].subImage.imageArrayIndex, "ImageArrayIndex"),
                                          TLArg(xr::ToString(proj->views[eye].subImage.imageRect).c_str(), "ImageRect"),
                                          TLArg(xr::ToString(proj->views[eye].pose).c_str(), "Pose"),
                                          TLArg(xr::ToString(proj->views[eye].fov).c_str(), "Fov"));

                        if (!Quaternion::IsNormalized(proj->views[eye].pose.orientation)) {
                            return XR_ERROR_POSE_INVALID;
                        }

                        if (!m_swapchains.count(proj->views[eye].subImage.swapchain)) {
                            return XR_ERROR_HANDLE_INVALID;
                        }

                        Swapchain& xrSwapchain = *(Swapchain*)proj->views[eye].subImage.swapchain;

                        if (xrSwapchain.lastReleasedIndex == -1) {
                            return XR_ERROR_LAYER_INVALID;
                        }

                        if (proj->views[eye].subImage.imageArrayIndex >= xrSwapchain.xrDesc.arraySize) {
                            return XR_ERROR_VALIDATION_FAILURE;
                        }

                        // Fill out color buffer information.
                        prepareAndCommitSwapchainImage(xrSwapchain,
                                                       i,
                                                       proj->views[eye].subImage.imageArrayIndex,
                                                       frameEndInfo->layers[i]->layerFlags,
                                                       committedSwapchainImages);
                        layer.EyeFov.ColorTexture[eye] =
                            xrSwapchain.pvrSwapchain[proj->views[eye].subImage.imageArrayIndex];

                        if (!isValidSwapchainRect(xrSwapchain.pvrDesc, proj->views[eye].subImage.imageRect)) {
                            return XR_ERROR_SWAPCHAIN_RECT_INVALID;
                        }
                        layer.EyeFov.Viewport[eye].x = proj->views[eye].subImage.imageRect.offset.x;
                        layer.EyeFov.Viewport[eye].y = proj->views[eye].subImage.imageRect.offset.y;
                        layer.EyeFov.Viewport[eye].width = proj->views[eye].subImage.imageRect.extent.width;
                        layer.EyeFov.Viewport[eye].height = proj->views[eye].subImage.imageRect.extent.height;

                        // Fill out pose and FOV information.
                        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
                        CHECK_XRCMD(xrLocateSpace(proj->space, m_originSpace, frameEndInfo->displayTime, &location));
                        layer.EyeFov.RenderPose[eye] =
                            xrPoseToPvrPose(Pose::Multiply(proj->views[eye].pose, location.pose));

                        layer.EyeFov.Fov[eye].DownTan = -tan(proj->views[eye].fov.angleDown);
                        layer.EyeFov.Fov[eye].UpTan = tan(proj->views[eye].fov.angleUp);
                        layer.EyeFov.Fov[eye].LeftTan = -tan(proj->views[eye].fov.angleLeft);
                        layer.EyeFov.Fov[eye].RightTan = tan(proj->views[eye].fov.angleRight);

                        // Per Pimax: this value is currently unused, but should be set to the timestamp of the head
                        // pose. In the case of OpenXR, we expect the app to use the predictedDisplayTime to query the
                        // head pose, and pass that same time as displayTime.
                        layer.EyeFov.SensorSampleTime = xrTimeToPvrTime(frameEndInfo->displayTime);

                        // Submit depth.
                        if (has_XR_KHR_composition_layer_depth) {
                            const XrBaseInStructure* entry =
                                reinterpret_cast<const XrBaseInStructure*>(proj->views[eye].next);
                            while (entry) {
                                if (entry->type == XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR) {
                                    const XrCompositionLayerDepthInfoKHR* depth =
                                        reinterpret_cast<const XrCompositionLayerDepthInfoKHR*>(entry);

                                    layer.Header.Type = pvrLayerType_EyeFovDepth;

                                    TraceLoggingWrite(
                                        g_traceProvider,
                                        "xrEndFrame_View",
                                        TLArg("Depth", "Type"),
                                        TLArg(eye, "Index"),
                                        TLXArg(depth->subImage.swapchain, "Swapchain"),
                                        TLArg(depth->subImage.imageArrayIndex, "ImageArrayIndex"),
                                        TLArg(xr::ToString(depth->subImage.imageRect).c_str(), "ImageRect"),
                                        TLArg(depth->nearZ, "Near"),
                                        TLArg(depth->farZ, "Far"),
                                        TLArg(depth->minDepth, "MinDepth"),
                                        TLArg(depth->maxDepth, "MaxDepth"));
                                    LOG_TELEMETRY_ONCE(logFeature("Depth"));

                                    if (!m_swapchains.count(depth->subImage.swapchain)) {
                                        return XR_ERROR_HANDLE_INVALID;
                                    }

                                    Swapchain& xrDepthSwapchain = *(Swapchain*)depth->subImage.swapchain;

                                    if (xrDepthSwapchain.lastReleasedIndex == -1) {
                                        return XR_ERROR_LAYER_INVALID;
                                    }

                                    if (depth->subImage.imageArrayIndex >= xrDepthSwapchain.xrDesc.arraySize) {
                                        return XR_ERROR_VALIDATION_FAILURE;
                                    }

                                    // Fill out depth buffer information.
                                    prepareAndCommitSwapchainImage(xrDepthSwapchain,
                                                                   i,
                                                                   depth->subImage.imageArrayIndex,
                                                                   0,
                                                                   committedSwapchainImages);
                                    layer.EyeFovDepth.DepthTexture[eye] =
                                        xrDepthSwapchain.pvrSwapchain[depth->subImage.imageArrayIndex];

                                    if (!isValidSwapchainRect(xrDepthSwapchain.pvrDesc, depth->subImage.imageRect)) {
                                        return XR_ERROR_SWAPCHAIN_RECT_INVALID;
                                    }

                                    // Fill out projection information.
                                    layer.EyeFovDepth.DepthProjectionDesc.Projection22 =
                                        depth->farZ / (depth->nearZ - depth->farZ);
                                    layer.EyeFovDepth.DepthProjectionDesc.Projection23 =
                                        (depth->farZ * depth->nearZ) / (depth->nearZ - depth->farZ);
                                    layer.EyeFovDepth.DepthProjectionDesc.Projection32 = -1.f;

                                    break;
                                }
                                entry = entry->next;
                            }
                        }
                    }
                } else if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_QUAD) {
                    const XrCompositionLayerQuad* quad =
                        reinterpret_cast<const XrCompositionLayerQuad*>(frameEndInfo->layers[i]);

                    TraceLoggingWrite(g_traceProvider,
                                      "xrEndFrame_Layer",
                                      TLArg("Quad", "Type"),
                                      TLArg(quad->layerFlags, "Flags"),
                                      TLXArg(quad->space, "Space"));
                    TraceLoggingWrite(g_traceProvider,
                                      "xrEndFrame_View",
                                      TLArg("Quad", "Type"),
                                      TLXArg(quad->subImage.swapchain, "Swapchain"),
                                      TLArg(quad->subImage.imageArrayIndex, "ImageArrayIndex"),
                                      TLArg(xr::ToString(quad->subImage.imageRect).c_str(), "ImageRect"),
                                      TLArg(xr::ToString(quad->pose).c_str(), "Pose"),
                                      TLArg(quad->size.width, "Width"),
                                      TLArg(quad->size.height, "Height"),
                                      TLArg(xr::ToCString(quad->eyeVisibility), "EyeVisibility"));

                    layer.Header.Type = pvrLayerType_Quad;

                    if (!Quaternion::IsNormalized(quad->pose.orientation)) {
                        return XR_ERROR_POSE_INVALID;
                    }

                    if (!m_swapchains.count(quad->subImage.swapchain)) {
                        return XR_ERROR_HANDLE_INVALID;
                    }

                    Swapchain& xrSwapchain = *(Swapchain*)quad->subImage.swapchain;

                    if (xrSwapchain.lastReleasedIndex == -1) {
                        return XR_ERROR_LAYER_INVALID;
                    }

                    // CONFORMANCE: We ignore eyeVisibility, since there is no equivalent in the PVR compositor.
                    // We cannot achieve conformance for this particular (but uncommon) API usage.
                    if (quad->eyeVisibility != XR_EYE_VISIBILITY_BOTH) {
                        LOG_TELEMETRY_ONCE(logUnimplemented("QuadEyeVisibilityNotSupported"));
                    }

                    if (quad->subImage.imageArrayIndex >= xrSwapchain.xrDesc.arraySize) {
                        return XR_ERROR_VALIDATION_FAILURE;
                    }

                    // Fill out color buffer information.
                    prepareAndCommitSwapchainImage(xrSwapchain,
                                                   i,
                                                   quad->subImage.imageArrayIndex,
                                                   frameEndInfo->layers[i]->layerFlags,
                                                   committedSwapchainImages);
                    layer.Quad.ColorTexture = xrSwapchain.pvrSwapchain[quad->subImage.imageArrayIndex];

                    if (!isValidSwapchainRect(xrSwapchain.pvrDesc, quad->subImage.imageRect)) {
                        return XR_ERROR_SWAPCHAIN_RECT_INVALID;
                    }
                    layer.Quad.Viewport.x = quad->subImage.imageRect.offset.x;
                    layer.Quad.Viewport.y = quad->subImage.imageRect.offset.y;
                    layer.Quad.Viewport.width = quad->subImage.imageRect.extent.width;
                    layer.Quad.Viewport.height = quad->subImage.imageRect.extent.height;

                    if (!m_spaces.count(quad->space)) {
                        return XR_ERROR_HANDLE_INVALID;
                    }
                    Space& xrSpace = *(Space*)quad->space;

                    // Fill out pose and quad information.
                    if (xrSpace.referenceType != XR_REFERENCE_SPACE_TYPE_VIEW) {
                        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
                        if (!m_needWorldLockedQuadLayerQuirk) {
                            CHECK_XRCMD(
                                xrLocateSpace(quad->space, m_originSpace, frameEndInfo->displayTime, &location));
                        } else {
                            // Workaround: use head-locked quads, otherwise PVR seems to misplace them in space.
                            CHECK_XRCMD(xrLocateSpace(quad->space, m_viewSpace, frameEndInfo->displayTime, &location));
                            layer.Header.Flags |= pvrLayerFlag_HeadLocked;
                        }
                        layer.Quad.QuadPoseCenter = xrPoseToPvrPose(Pose::Multiply(quad->pose, location.pose));
                    } else {
                        layer.Quad.QuadPoseCenter = xrPoseToPvrPose(Pose::Multiply(quad->pose, xrSpace.poseInSpace));
                        layer.Header.Flags |= pvrLayerFlag_HeadLocked;
                    }

                    layer.Quad.QuadSize.x = quad->size.width;
                    layer.Quad.QuadSize.y = quad->size.height;
                } else {
                    return XR_ERROR_LAYER_INVALID;
                }

                layers.push_back(&layer.Header);
            }

            {
                // Defer initialization of guardian resources until they are first needed.
                if (m_guardianSpace == XR_NULL_HANDLE) {
                    initializeGuardianResources();
                }

                // Measure the floor distance between the center of the guardian and the headset.
                XrSpaceLocation viewToBase{XR_TYPE_SPACE_LOCATION};
                CHECK_XRCMD(xrLocateSpace(m_viewSpace, m_originSpace, frameEndInfo->displayTime, &viewToBase));
                XrSpaceLocation guardianToBase{XR_TYPE_SPACE_LOCATION};
                CHECK_XRCMD(xrLocateSpace(m_guardianSpace, m_originSpace, frameEndInfo->displayTime, &guardianToBase));
                if (Pose::IsPoseValid(viewToBase.locationFlags) && Pose::IsPoseValid(guardianToBase.locationFlags) &&
                    Length(XrVector3f{guardianToBase.pose.position.x, 0.f, guardianToBase.pose.position.z} -
                           XrVector3f{viewToBase.pose.position.x, 0.f, viewToBase.pose.position.z}) >
                        m_guardianThreshold) {
                    // Draw the guardian on top of everything.
                    auto& layer = layersAllocator[layers.size()];
                    layer.Header.Type = pvrLayerType_Quad;
                    layer.Header.Flags = 0;
                    layer.Quad.ColorTexture = m_guardianSwapchain;
                    layer.Quad.Viewport.x = layer.Quad.Viewport.y = 0;
                    layer.Quad.Viewport.width = m_guardianExtent.width;
                    layer.Quad.Viewport.height = m_guardianExtent.height;

                    // Place the guardian in 3D space as a 2D overlay.
                    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
                    if (!m_needWorldLockedQuadLayerQuirk) {
                        layer.Quad.QuadPoseCenter = xrPoseToPvrPose(guardianToBase.pose);
                    } else {
                        // Workaround: use head-locked quads, otherwise PVR seems to misplace them in space.
                        layer.Quad.QuadPoseCenter =
                            xrPoseToPvrPose(Pose::Multiply(guardianToBase.pose, Pose::Invert(viewToBase.pose)));
                        layer.Header.Flags |= pvrLayerFlag_HeadLocked;
                    }
                    layer.Quad.QuadSize.x = layer.Quad.QuadSize.y = m_guardianRadius * 2;

                    // If there are too many layer, prioritize the guardian to be safe.
                    if (layers.size() < pvrMaxLayerCount) {
                        layers.push_back(&layer.Header);
                    } else {
                        layers[pvrMaxLayerCount - 1] = &layer.Header;
                    }
                }
            }

            if (IsTraceEnabled()) {
                m_gpuTimerPrecomposition[m_currentTimerIndex]->stop();
            }

            // Update the FPS counter.
            const auto now = pvr_getTimeSeconds(m_pvr);
            m_frameTimes.push_back(now);
            while (now - m_frameTimes.front() >= 1.0) {
                m_frameTimes.pop_front();
            }

            // Add a dummy layer so we can still call pvr_endFrame() for timing purposes.
            pvrLayerHeader dummyLayer{};
            if (layers.empty()) {
                dummyLayer.Type = pvrLayerType_Disabled;
                layers.push_back(&dummyLayer);
            }

            // Submit the layers to PVR.
            if (m_useFrameTimingOverride) {
                float renderMs = 0.f;
                if (!m_frameTimeOverrideUs) {
                    // No inherent biasing today. Might change in the future.
                    // TODO: We should account for m_lastCpuFrameTimeUs here.
                    const auto biasedCpuFrameTimeUs = 0ll;
                    const auto biasedGpuFrameTimeUs = (int64_t)m_lastGpuFrameTimeUs + 0;

                    const auto latestFrameTimeUs = std::max(
                        0ll, std::max(biasedCpuFrameTimeUs, biasedGpuFrameTimeUs) + m_frameTimeOverrideOffsetUs);

                    // Simple median filter to smooth out the values.
                    m_frameTimeFilter.push_back(latestFrameTimeUs);
                    while (m_frameTimeFilter.size() > m_frameTimeFilterLength) {
                        m_frameTimeFilter.pop_front();
                    }
                    auto sortedFrameTimes = m_frameTimeFilter;
                    std::sort(sortedFrameTimes.begin(), sortedFrameTimes.end());

                    const auto filteredFrameTimeUs = sortedFrameTimes[sortedFrameTimes.size() / 2];
                    renderMs = filteredFrameTimeUs / 1e3f;
                } else {
                    m_frameTimeFilter.clear();

                    renderMs = std::max(0ll, (int64_t)m_frameTimeOverrideUs + m_frameTimeOverrideOffsetUs) / 1e3f;
                }

                TraceLoggingWrite(g_traceProvider, "PVR_ClientRenderMs", TLArg(renderMs, "RenderMs"));

                // pi_server requires to set this config value to hint the frame time of the application. This call
                // always seems to fail, in spite of having side effects.
                // According to Pimax, this value must be set to the last GPU frame time.
                pvr_setFloatConfig(m_pvrSession, "openvr_client_render_ms", renderMs);
            }

            const long long pvrFrameId = m_frameBegun - 1;
            TraceLocalActivity(endFrame);
            TraceLoggingWriteStart(endFrame,
                                   "PVR_EndFrame",
                                   TLArg(pvrFrameId, "FrameIndex"),
                                   TLArg(layers.size(), "NumLayers"),
                                   TLArg(m_frameTimes.size(), "MeasuredFps"),
                                   TLArg(pvr_getFloatConfig(m_pvrSession, "client_fps", 0), "ClientFps"),
                                   TLArg(lastPrecompositionTime, "LastPrecompositionTimeUs"));
            CHECK_PVRCMD(pvr_endFrame(m_pvrSession, pvrFrameId, layers.data(), (unsigned int)layers.size()));
            TraceLoggingWriteStop(endFrame, "PVR_EndFrame");

            // Defer initialization of mirror window resources until they are first needed.
            if (m_useMirrorWindow && !m_mirrorWindowThread.joinable()) {
                createMirrorWindow();
            }
            updateMirrorWindow();

            // When using RenderDoc, signal a frame through the dummy swapchain.
            if (m_dxgiSwapchain) {
                m_dxgiSwapchain->Present(0, 0);
                m_pvrSubmissionContext->Flush();
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
            m_frameCondVar.notify_one();
        }

        return XR_SUCCESS;
    }

} // namespace pimax_openxr
