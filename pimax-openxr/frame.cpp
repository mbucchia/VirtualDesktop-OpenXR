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

        // Clear the platform SDK message queue just in case it would unnecessarily leak memory.
        pvrMessageHandle message;
        while (m_pvrPlatformReady && (message = pvr_PollMessage())) {
            const auto messageType = pvr_Message_GetType(message);
            TraceLoggingWrite(g_traceProvider, "PVR_Platform", TLXArg((void*)messageType, "Message"));

            // Trace errors for good measure.
            if (pvr_Message_IsError(message)) {
                TraceLoggingWrite(g_traceProvider,
                                  "PVR_Platform",
                                  TLArg(pvr_Message_GetErrorInfo(pvr_Message_GetError(message)), "Error"));
            }

            // Shutdown PVR platform on runtime error to avoid unwanted side effects.
            switch (messageType) {
            case pvrMessage_Notify_RuntimeError:
                // The platform SDK does not seem to export this on 32-bit. It is misnamed "RunningError" instead.
#ifdef _WIN64
                TraceLoggingWrite(
                    g_traceProvider, "PVR_Platform", TLArg((int)pvr_RuntimeError_GetError(message), "RuntimeError"));
#endif
                m_pvrPlatformReady = false;
                break;

            case pvrMessage_Notify_Logout:
                TraceLoggingWrite(g_traceProvider, "PVR_Platform", TLArg("Logout", "Action"));
                m_pvrPlatformReady = false;
                break;

            default:
                break;
            }

            if (!m_pvrPlatformReady) {
                pvr_PlatformShutdown();
            }
        }

        // Check for user presence and exit conditions. Emit events accordingly.
        pvrHmdStatus status{};
        CHECK_PVRCMD(pvr_getHmdStatus(m_pvrSession, &status));
        TraceLoggingWrite(g_traceProvider,
                          "PVR_HmdStatus",
                          TLArg(!!status.ServiceReady, "ServiceReady"),
                          TLArg(!!status.HmdPresent, "HmdPresent"),
                          TLArg(!!status.HmdMounted, "HmdMounted"),
                          TLArg(!!status.IsVisible, "IsVisible"),
                          TLArg(!!status.DisplayLost, "DisplayLost"),
                          TLArg(!!status.ShouldQuit, "ShouldQuit"));
        if (!(status.ServiceReady && status.HmdPresent) || status.DisplayLost || status.ShouldQuit) {
            m_sessionState = XR_SESSION_STATE_LOSS_PENDING;
            m_sessionStateDirty = true;
            m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

            return XR_SESSION_LOSS_PENDING;
        }

        // Important: for state transitions, we must wait for the application to poll the session state to make sure
        // that it sees every single state.

        bool wasSessionStateDirty = m_sessionStateDirty;
        if (!wasSessionStateDirty && status.IsVisible && !m_sessionExiting) {
            if (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED) {
                m_sessionState = XR_SESSION_STATE_VISIBLE;
                m_sessionStateDirty = true;
            }

            if (!m_sessionStateDirty) {
                if (status.HmdMounted) {
                    if (m_sessionState == XR_SESSION_STATE_VISIBLE) {
                        m_sessionState = XR_SESSION_STATE_FOCUSED;
                        m_sessionStateDirty = true;
                    }
                } else {
                    if (m_sessionState == XR_SESSION_STATE_FOCUSED) {
                        m_sessionState = XR_SESSION_STATE_VISIBLE;
                        m_sessionStateDirty = true;
                    }
                }
            }

            frameState->shouldRender = XR_TRUE;
        } else {
            if (m_sessionState != XR_SESSION_STATE_SYNCHRONIZED && m_sessionState != XR_SESSION_STATE_STOPPING &&
                !m_sessionExiting) {
                m_sessionState = XR_SESSION_STATE_SYNCHRONIZED;
                m_sessionStateDirty = true;
            }

            frameState->shouldRender = XR_FALSE;
        }

        if (!wasSessionStateDirty && m_sessionStateDirty) {
            m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);
        }

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

            // Wait for possibly deferred pvr_endFrame() to complete.
            if (m_asyncEndFrame.valid()) {
                TraceLocalActivity(waitDeferredEndFrame);
                TraceLoggingWriteStart(waitDeferredEndFrame, "WaitDeferredEndFrame");
                m_asyncEndFrame.wait();
                TraceLoggingWriteStop(waitDeferredEndFrame, "WaitDeferredEndFrame");

                m_asyncEndFrame = {};
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

            // Wait for PVR to be ready for the next frame.
            // Workaround: No idea why, but waiting for the first frame with OpenComposite always causes a crash inside
            // PVR. Skip it.
            if (!m_isOpenComposite || m_frameWaited) {
                TraceLocalActivity(waitToBeginFrame);
                TraceLoggingWriteStart(waitToBeginFrame, "PVR_WaitToBeginFrame");
                // The PVR sample is using frame index 0 for every frame and I am observing strange behaviors when using
                // a monotonically increasing frame index. Let's follow the example.
                CHECK_PVRCMD(pvr_waitToBeginFrame(m_pvrSession, 0));
                TraceLoggingWriteStop(waitToBeginFrame, "PVR_WaitToBeginFrame");
            }

            if (IsTraceEnabled()) {
                waitTimer.stop();
            }

            const double now = pvr_getTimeSeconds(m_pvr);
            double predictedDisplayTime = pvr_getPredictedDisplayTime(m_pvrSession, 0);
            TraceLoggingWrite(g_traceProvider,
                              "WaitFrame",
                              TLArg(now, "Now"),
                              TLArg(predictedDisplayTime, "PredictedDisplayTime"),
                              TLArg(predictedDisplayTime - now, "PhotonTime"),
                              TLArg(waitTimer.query(), "WaitDurationUs"));

            // Setup the app frame for use and the next frame for this call.
            frameState->predictedDisplayTime = pvrTimeToXrTime(predictedDisplayTime);

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

        bool frameDiscarded = false;

        // Critical section.
        {
            CpuTimer waitTimer;
            if (IsTraceEnabled()) {
                waitTimer.start();
            }

            std::unique_lock lock(m_frameLock);

            if (m_frameWaited == m_frameCompleted) {
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

                // Tell PVR we are about to begin the frame.
                // Workaround: No idea why, but waiting for the first frame with OpenComposite always causes a crash
                // inside PVR. Skip it.
                if (!m_isOpenComposite || m_frameBegun) {
                    TraceLocalActivity(beginFrame);
                    TraceLoggingWriteStart(beginFrame, "PVR_BeginFrame");
                    CHECK_PVRCMD(pvr_beginFrame(m_pvrSession, 0));
                    TraceLoggingWriteStop(beginFrame, "PVR_BeginFrame");
                }
            } else {
                frameDiscarded = true;
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
                m_lastGpuFrameTimeUs = m_gpuTimerApp[m_currentTimerIndex]->query();

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
                m_gpuTimerApp[m_currentTimerIndex]->start();
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

        if (frameEndInfo->environmentBlendMode != XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
            return XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED;
        }

        // Count extra layer for the guardian.
        if (frameEndInfo->layerCount + 1 > pvrMaxLayerCount) {
            return XR_ERROR_LAYER_LIMIT_EXCEEDED;
        }

        // Critical section.
        {
            std::unique_lock lock1(m_swapchainsLock);
            std::unique_lock lock2(m_frameLock);

            if (m_frameBegun == m_frameCompleted) {
                return XR_ERROR_CALL_ORDER_INVALID;
            }

            // Serializes the app work between D3D12/Vulkan and D3D11.
            if (isD3D12Session()) {
                serializeD3D12Frame();
            } else if (isVulkanSession()) {
                serializeVulkanFrame();
            } else if (isOpenGLSession()) {
                serializeOpenGLFrame();
            }

            if (m_useFrameTimingOverride || IsTraceEnabled()) {
                m_renderTimerApp.stop();
                m_gpuTimerApp[m_currentTimerIndex]->stop();
            }

            const auto lastPrecompositionTime = m_gpuTimerPrecomposition[m_currentTimerIndex]->query();
            if (IsTraceEnabled()) {
                m_gpuTimerPrecomposition[m_currentTimerIndex]->start();
            }

            std::set<std::pair<pvrTextureSwapChain, uint32_t>> committedSwapchainImages;

            // Construct the list of layers.
            std::vector<pvrLayer_Union> layersAllocator(frameEndInfo->layerCount + 1);
            for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                auto& layer = layersAllocator[i];
                layer.Header.Flags = 0;

                // OpenGL needs to flip the texture vertically, which PVR can conveniently do for us.
                if (isOpenGLSession()) {
                    layer.Header.Flags = pvrLayerFlag_TextureOriginAtBottomLeft;
                }

                // COMPLIANCE: We ignore layerFlags, since there is no equivalent.
                // Log the most common case that might cause issue.
                if (!(frameEndInfo->layers[i]->layerFlags & XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT)) {
                    LOG_TELEMETRY_ONCE(logUnimplemented("LayerFlagsNotSupported"));
                }

                if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
                    const XrCompositionLayerProjection* proj =
                        reinterpret_cast<const XrCompositionLayerProjection*>(frameEndInfo->layers[i]);

                    TraceLoggingWrite(g_traceProvider,
                                      "xrEndFrame_Layer",
                                      TLArg("Proj", "Type"),
                                      TLArg(proj->layerFlags, "Flags"),
                                      TLXArg(proj->space, "Space"));

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

                        if (!m_swapchains.count(proj->views[eye].subImage.swapchain)) {
                            return XR_ERROR_HANDLE_INVALID;
                        }

                        Swapchain& xrSwapchain = *(Swapchain*)proj->views[eye].subImage.swapchain;

                        // Fill out color buffer information.
                        prepareAndCommitSwapchainImage(
                            xrSwapchain, proj->views[eye].subImage.imageArrayIndex, committedSwapchainImages);
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

                        // Other applications (eg: SteamVR) always pass 0, and I am observing strange flickering when
                        // passing any other value. Let's follow what SteamVR does.
                        layer.EyeFov.SensorSampleTime = 0;

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

                                    // Fill out depth buffer information.
                                    prepareAndCommitSwapchainImage(
                                        xrDepthSwapchain, depth->subImage.imageArrayIndex, committedSwapchainImages);
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

                    if (!m_swapchains.count(quad->subImage.swapchain)) {
                        return XR_ERROR_HANDLE_INVALID;
                    }

                    Swapchain& xrSwapchain = *(Swapchain*)quad->subImage.swapchain;

                    // COMPLIANCE: We ignore eyeVisibility, since there is no equivalent.
                    if (quad->eyeVisibility != XR_EYE_VISIBILITY_BOTH) {
                        LOG_TELEMETRY_ONCE(logUnimplemented("QuadEyeVisibilityNotSupported"));
                    }

                    // Fill out color buffer information.
                    prepareAndCommitSwapchainImage(
                        xrSwapchain, quad->subImage.imageArrayIndex, committedSwapchainImages);
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
                        // Workaround: always use head-locked quads, otherwise PVR seems to misplace them in space.
                        CHECK_XRCMD(xrLocateSpace(quad->space, m_viewSpace, frameEndInfo->displayTime, &location));
                        layer.Header.Flags = pvrLayerFlag_HeadLocked;
                        layer.Quad.QuadPoseCenter = xrPoseToPvrPose(Pose::Multiply(quad->pose, location.pose));
                    } else {
                        layer.Quad.QuadPoseCenter = xrPoseToPvrPose(quad->pose);
                        layer.Header.Flags = pvrLayerFlag_HeadLocked;
                    }

                    layer.Quad.QuadSize.x = quad->size.width;
                    layer.Quad.QuadSize.y = quad->size.height;
                } else {
                    return XR_ERROR_LAYER_INVALID;
                }
            }

            {
                // Defer initialization of guardian resources until they are first needed.
                if (m_guardianSpace == XR_NULL_HANDLE) {
                    initializeGuardianResources();
                }

                // Draw the guardian on top of everything.
                auto& layer = layersAllocator[frameEndInfo->layerCount];
                layer.Header.Type = pvrLayerType_Disabled;

                // Measure the floor distance between the center of the guardian and the headset.
                XrSpaceLocation viewToBase{XR_TYPE_SPACE_LOCATION};
                CHECK_XRCMD(xrLocateSpace(m_viewSpace, m_originSpace, frameEndInfo->displayTime, &viewToBase));
                XrSpaceLocation guardianToBase{XR_TYPE_SPACE_LOCATION};
                CHECK_XRCMD(xrLocateSpace(m_guardianSpace, m_originSpace, frameEndInfo->displayTime, &guardianToBase));
                if (Pose::IsPoseValid(viewToBase.locationFlags) && Pose::IsPoseValid(guardianToBase.locationFlags) &&
                    Length(XrVector3f{guardianToBase.pose.position.x, 0.f, guardianToBase.pose.position.z} -
                           XrVector3f{viewToBase.pose.position.x, 0.f, viewToBase.pose.position.z}) >
                        m_guardianThreshold) {
                    layer.Header.Type = pvrLayerType_Quad;
                    layer.Header.Flags = pvrLayerFlag_HeadLocked;
                    layer.Quad.ColorTexture = m_guardianSwapchain;
                    layer.Quad.Viewport.x = layer.Quad.Viewport.y = 0;
                    layer.Quad.Viewport.width = m_guardianExtent.width;
                    layer.Quad.Viewport.height = m_guardianExtent.height;

                    // Place the guardian in 3D space as a 2D overlay.
                    layer.Quad.QuadPoseCenter =
                        xrPoseToPvrPose(Pose::Multiply(guardianToBase.pose, Pose::Invert(viewToBase.pose)));
                    layer.Quad.QuadSize.x = layer.Quad.QuadSize.y = m_guardianRadius * 2;
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
            const auto measuredFps = m_frameTimes.size();
            const auto clientFps = pvr_getFloatConfig(m_pvrSession, "client_fps", 0);

            // Submit frame timing to PVR.
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

            // Submit the layers to PVR.
            const auto submitFrame = [this, layersAllocator, measuredFps, clientFps, lastPrecompositionTime]() {
                std::vector<const pvrLayerHeader*> layers;
                for (auto& layer : layersAllocator) {
                    layers.push_back(&layer.Header);
                }

                TraceLocalActivity(endFrame);
                TraceLoggingWriteStart(endFrame,
                                       "PVR_EndFrame",
                                       TLArg(layers.size(), "NumLayers"),
                                       TLArg(measuredFps, "MeasuredFps"),
                                       TLArg(clientFps, "ClientFps"),
                                       TLArg(lastPrecompositionTime, "LastPrecompositionTimeUs"));
                CHECK_PVRCMD(pvr_endFrame(m_pvrSession, 0, layers.data(), (unsigned int)layers.size()));
                TraceLoggingWriteStop(endFrame, "PVR_EndFrame");
            };

            if (!m_useDeferredFrameSubmit) {
                submitFrame();
                m_asyncEndFrame = {};
            } else {
                // TODO: Not clear why, but in certain circumstances, PVR will make us wait here rather than
                // pvr_waitToBeginFrame(). We run this work asynchronously and we will wait just before invoking
                // pvr_waitToBeginFrame().
                TraceLoggingWrite(g_traceProvider, "EndFrame_DeferFrameSubmit");
                m_asyncEndFrame = std::async(std::launch::async, submitFrame);
            }

            // When using RenderDoc, signal a frame through the dummy swapchain.
            if (m_dxgiSwapchain) {
                m_dxgiSwapchain->Present(0, 0);
                m_d3d11DeviceContext->Flush();
            }

            m_frameCompleted = m_frameBegun;

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
