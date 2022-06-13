// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
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

        // Check for user presence and exit conditions. Emit events accordingly.
        pvrHmdStatus status{};
        CHECK_PVRCMD(pvr_getHmdStatus(m_pvrSession, &status));
        TraceLoggingWrite(g_traceProvider,
                          "PVR_HmdStatus",
                          TLArg(status.ServiceReady, "ServiceReady"),
                          TLArg(status.HmdPresent, "HmdPresent"),
                          TLArg(status.HmdMounted, "HmdMounted"),
                          TLArg(status.IsVisible, "IsVisible"),
                          TLArg(status.DisplayLost, "DisplayLost"),
                          TLArg(status.ShouldQuit, "ShouldQuit"));
        if (!(status.ServiceReady && status.HmdPresent) || status.DisplayLost || status.ShouldQuit) {
            m_sessionState = XR_SESSION_STATE_LOSS_PENDING;
            m_sessionStateDirty = true;
            m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

            return XR_SESSION_LOSS_PENDING;
        }

        // Important: for state transitions, we must wait for the application to poll the session state to make sure
        // that it sees every single state.

        bool wasSessionStateDirty = m_sessionStateDirty;
        if (!wasSessionStateDirty && status.IsVisible) {
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
            if (m_sessionState != XR_SESSION_STATE_SYNCHRONIZED) {
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
            std::unique_lock lock(m_frameLock);

            // Wait for a call to xrBeginFrame() to match the previous call to xrWaitFrame().
            if (m_frameWaited) {
                TraceLoggingWrite(g_traceProvider, "WaitFrame1_Begin");
                const bool timedOut = !m_frameCondVar.wait_for(lock, 10000ms, [&] { return m_frameBegun; });
                TraceLoggingWrite(g_traceProvider, "WaitFrame1_End", TLArg(timedOut, "TimedOut"));
                // TODO: What to do if timed out? This would mean an app deadlock should have happened.
            }

            // Calculate the time to the next frame.
            auto timeout = 100ms;
            double amount = 0.0;
            if (m_lastFrameWaitedTime) {
                const double now = pvr_getTimeSeconds(m_pvr);
                const double nextFrameTime = m_lastFrameWaitedTime.value() + m_frameDuration;
                if (nextFrameTime > now) {
                    amount = nextFrameTime - now;
                    timeout = std::chrono::milliseconds((uint64_t)(amount * 1e3));
                } else {
                    timeout = 0ms;
                }
            }

            // Wait for xrEndFrame() completion or for the next frame time.
            TraceLoggingWrite(g_traceProvider, "WaitFrame2_Begin", TLArg(amount, "Amount"));
            const bool timedOut = !m_frameCondVar.wait_for(lock, timeout, [&] { return !m_frameBegun; });
            TraceLoggingWrite(g_traceProvider, "WaitFrame2_End", TLArg(timedOut, "TimedOut"));

            const double now = pvr_getTimeSeconds(m_pvr);
            double predictedDisplayTime = pvr_getPredictedDisplayTime(m_pvrSession, m_nextFrameIndex);
            TraceLoggingWrite(g_traceProvider,
                              "WaitFrame",
                              TLArg(m_nextFrameIndex, "ThisFrameIndex"),
                              TLArg(now, "Now"),
                              TLArg(predictedDisplayTime, "PredictedDisplayTime"),
                              TLArg(predictedDisplayTime - now, "PhotonTime"));

            // When behind too much (200ms is arbitrary), we skip rendering and provide an ideal frame time.
            if (predictedDisplayTime < now - 0.2) {
                // We always render the first frame to kick off PVR.
                frameState->shouldRender = m_nextFrameIndex == 0;
                predictedDisplayTime = now + m_frameDuration;
            }

            // Setup the app frame for use and the next frame for this call.
            frameState->predictedDisplayTime = pvrTimeToXrTime(predictedDisplayTime);
            frameState->predictedDisplayPeriod = pvrTimeToXrTime(m_frameDuration);

            m_frameWaited = true;
            m_nextFrameIndex++;
            TraceLoggingWrite(g_traceProvider, "WaitFrame", TLArg(m_nextFrameIndex, "NextFrameIndex"));
        }

        m_lastFrameWaitedTime = pvr_getTimeSeconds(m_pvr);

        TraceLoggingWrite(g_traceProvider,
                          "xrWaitFrame",
                          TLArg(frameState->shouldRender, "ShouldRender"),
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
            std::unique_lock lock(m_frameLock);

            if (!m_frameWaited) {
                return XR_ERROR_CALL_ORDER_INVALID;
            }

            if (m_frameBegun) {
                frameDiscarded = true;
            }

            m_currentFrameIndex = m_nextFrameIndex;

            // TODO: Not sure why we need this workaround. The very first call to pvr_beginFrame() crashes inside
            // PVR unless there is a call to pvr_endFrame() first... Also unclear why the call occasionally fails
            // with error code -1 (undocumented).
            if (m_canBeginFrame) {
                TraceLoggingWrite(
                    g_traceProvider, "PVR_BeginFrame_Begin", TLArg(m_currentFrameIndex, "CurrentFrameIndex"));
                const auto result = pvr_beginFrame(m_pvrSession, m_currentFrameIndex);
                TraceLoggingWrite(g_traceProvider, "PVR_BeginFrame_End", TLArg((int)result, "Result"));
            }

            m_frameWaited = false;
            m_frameBegun = true;

            // Signal xrWaitFrame().
            TraceLoggingWrite(g_traceProvider, "BeginFrame_Signal");
            m_frameCondVar.notify_one();
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

        if (frameEndInfo->layerCount > 16) {
            return XR_ERROR_LAYER_LIMIT_EXCEEDED;
        }

        // Critical section.
        {
            std::unique_lock lock(m_frameLock);

            if (!m_frameBegun) {
                return XR_ERROR_CALL_ORDER_INVALID;
            }

            // Serializes the app work between D3D12/Vulkan and D3D11.
            if (isD3D12Session()) {
                serializeD3D12Frame();
            } else if (isVulkanSession()) {
                serializeVulkanFrame();
            }

            std::set<std::pair<pvrTextureSwapChain, uint32_t>> committedSwapchainImages;

            // Construct the list of layers.
            std::vector<pvrLayer_Union> layersAllocator(frameEndInfo->layerCount);
            std::vector<pvrLayerHeader*> layers;
            for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                auto& layer = layersAllocator[i];

                // TODO: What do we do with layerFlags?

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
                        XrPosef transformed;
                        StoreXrPose(&transformed,
                                    XMMatrixMultiply(LoadXrPose(proj->views[eye].pose), LoadXrPose(location.pose)));
                        layer.EyeFov.RenderPose[eye] = xrPoseToPvrPose(transformed);

                        layer.EyeFov.Fov[eye].DownTan = -tan(proj->views[eye].fov.angleDown);
                        layer.EyeFov.Fov[eye].UpTan = tan(proj->views[eye].fov.angleUp);
                        layer.EyeFov.Fov[eye].LeftTan = -tan(proj->views[eye].fov.angleLeft);
                        layer.EyeFov.Fov[eye].RightTan = tan(proj->views[eye].fov.angleRight);

                        // This looks incorrect (because "sensor time" should be different from "display time"), but
                        // this is what the PVR sample code does.
                        layer.EyeFov.SensorSampleTime = xrTimeToPvrTime(frameEndInfo->displayTime);

                        // Submit depth.
                        if (m_isDepthSupported) {
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

                    // TODO: We ignore eyeVisibility as there is no equivalent.

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

                    // Fill out pose and quad information.
                    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
                    CHECK_XRCMD(xrLocateSpace(quad->space, m_originSpace, frameEndInfo->displayTime, &location));
                    XrPosef transformed;
                    StoreXrPose(&transformed, XMMatrixMultiply(LoadXrPose(quad->pose), LoadXrPose(location.pose)));
                    layer.Quad.QuadPoseCenter = xrPoseToPvrPose(transformed);

                    layer.Quad.QuadSize.x = quad->size.width;
                    layer.Quad.QuadSize.y = quad->size.height;
                } else {
                    return XR_ERROR_LAYER_INVALID;
                }

                layers.push_back(&layer.Header);
            }

            // Submit the layers to PVR.
            if (!layers.empty()) {
                TraceLoggingWrite(g_traceProvider,
                                  "PVR_EndFrame_Begin",
                                  TLArg(m_nextFrameIndex, "CurrentFrameIndex"),
                                  TLArg(layers.size(), "NumLayers"));
                CHECK_PVRCMD(
                    pvr_endFrame(m_pvrSession, m_currentFrameIndex, layers.data(), (unsigned int)layers.size()));
                TraceLoggingWrite(g_traceProvider, "PVR_EndFrame_End");

                m_canBeginFrame = true;
            }

            // When using RenderDoc, signal a frame through the dummy swapchain.
            if (m_dxgiSwapchain) {
                m_dxgiSwapchain->Present(0, 0);
                m_d3d11DeviceContext->Flush();
            }

            m_frameBegun = false;

            // Signal xrWaitFrame().
            TraceLoggingWrite(g_traceProvider, "EndFrame_Signal");
            m_frameCondVar.notify_one();
        }

        return XR_SUCCESS;
    }

} // namespace pimax_openxr
