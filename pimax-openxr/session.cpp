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
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateSession
    XrResult OpenXrRuntime::xrCreateSession(XrInstance instance,
                                            const XrSessionCreateInfo* createInfo,
                                            XrSession* session) {
        if (createInfo->type != XR_TYPE_SESSION_CREATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateSession",
                          TLXArg(instance, "Instance"),
                          TLArg((int)createInfo->systemId, "SystemId"),
                          TLArg(createInfo->createFlags, "CreateFlags"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || createInfo->systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        // We only support one concurrent session.
        if (m_sessionCreated) {
            return XR_ERROR_LIMIT_REACHED;
        }

        // Get the graphics device and initialize the necessary resources.
        bool hasGraphicsBindings = false;
        const XrBaseInStructure* entry = reinterpret_cast<const XrBaseInStructure*>(createInfo->next);
        while (entry) {
            if (has_XR_KHR_D3D11_enable && entry->type == XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) {
                if (!m_graphicsRequirementQueried) {
                    return XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING;
                }

                const XrGraphicsBindingD3D11KHR* d3dBindings =
                    reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(entry);

                const auto result = initializeD3D11(*d3dBindings);
                if (XR_FAILED(result)) {
                    return result;
                }

                hasGraphicsBindings = true;
                break;
            } else if (has_XR_KHR_D3D12_enable && entry->type == XR_TYPE_GRAPHICS_BINDING_D3D12_KHR) {
                if (!m_graphicsRequirementQueried) {
                    return XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING;
                }

                const XrGraphicsBindingD3D12KHR* d3dBindings =
                    reinterpret_cast<const XrGraphicsBindingD3D12KHR*>(entry);

                const auto result = initializeD3D12(*d3dBindings);
                if (XR_FAILED(result)) {
                    return result;
                }

                hasGraphicsBindings = true;
                break;
            } else if ((has_XR_KHR_vulkan_enable || has_XR_KHR_vulkan_enable2) &&
                       entry->type == XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR) {
                if (!m_graphicsRequirementQueried) {
                    return XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING;
                }

                const XrGraphicsBindingVulkanKHR* vkBindings =
                    reinterpret_cast<const XrGraphicsBindingVulkanKHR*>(entry);

                const auto result = initializeVulkan(*vkBindings);
                if (XR_FAILED(result)) {
                    return result;
                }

                hasGraphicsBindings = true;
                break;
            } else if (has_XR_KHR_opengl_enable && entry->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR) {
                if (!m_graphicsRequirementQueried) {
                    return XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING;
                }

                const XrGraphicsBindingOpenGLWin32KHR* glBindings =
                    reinterpret_cast<const XrGraphicsBindingOpenGLWin32KHR*>(entry);

                const auto result = initializeOpenGL(*glBindings);
                if (XR_FAILED(result)) {
                    return result;
                }

                hasGraphicsBindings = true;
                break;
            }

            entry = entry->next;
        }

        if (!hasGraphicsBindings) {
            return XR_ERROR_GRAPHICS_DEVICE_INVALID;
        }

        // Read configuration and set up the session accordingly.
        if (getSetting("recenter_on_startup").value_or(true)) {
            CHECK_PVRCMD(pvr_recenterTrackingOrigin(m_pvrSession));
        }
        refreshSettings();

        {
            const bool enableLighthouse = !!pvr_getIntConfig(m_pvrSession, "enable_lighthouse_tracking", 0);

            TraceLoggingWrite(
                g_traceProvider,
                "PVR_Config",
                TLArg(enableLighthouse, "EnableLighthouse"),
                TLArg(m_fovLevel, "FovLevel"),
                TLArg(m_useParallelProjection, "UseParallelProjection"),
                TLArg(!!pvr_getIntConfig(m_pvrSession, "dbg_asw_enable", 0), "EnableSmartSmoothing"),
                TLArg(pvr_getIntConfig(m_pvrSession, "dbg_force_framerate_divide_by", 1), "CompulsiveSmoothingRate"));

            m_telemetry.logScenario(isD3D12Session()    ? "D3D12"
                                    : isVulkanSession() ? "Vulkan"
                                    : isOpenGLSession() ? "OpenGL"
                                                        : "D3D11",
                                    enableLighthouse,
                                    m_fovLevel,
                                    m_useParallelProjection,
                                    m_useMirrorWindow);
        }

        m_sessionCreated = true;

        // FIXME: Reset the session and frame state here.
        m_sessionState = XR_SESSION_STATE_IDLE;
        updateSessionState(true);

        m_frameWaited = m_frameBegun = m_frameCompleted = 0;

        m_frameTimes.clear();

        m_isControllerActive[0] = m_isControllerActive[1] = false;
        m_controllerAimPose[0] = m_controllerGripPose[0] = m_controllerAimPose[1] = m_controllerGripPose[1] =
            Pose::Identity();
        rebindControllerActions(0);
        rebindControllerActions(1);
        m_activeActionSets.clear();

        m_sessionStartTime = pvr_getTimeSeconds(m_pvr);
        m_sessionTotalFrameCount = 0;

        try {
            // Create a reference space with the origin and the HMD pose.
            m_originSpace = new Space;
            m_originSpace->referenceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            m_originSpace->poseInSpace = Pose::Identity();
            m_viewSpace = new Space;
            m_viewSpace->referenceType = XR_REFERENCE_SPACE_TYPE_VIEW;
            m_viewSpace->poseInSpace = Pose::Identity();
        } catch (std::exception& exc) {
            m_sessionCreated = false;
            throw exc;
        }

        *session = (XrSession)1;

        TraceLoggingWrite(g_traceProvider, "xrCreateSession", TLXArg(*session, "Session"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroySession
    XrResult OpenXrRuntime::xrDestroySession(XrSession session) {
        TraceLoggingWrite(g_traceProvider, "xrDestroySession", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (m_useAsyncSubmission && !m_needStartAsyncSubmissionThread) {
            {
                std::unique_lock lock(m_asyncSubmissionMutex);

                m_terminateAsyncThread = true;
                m_asyncSubmissionCondVar.notify_all();
            }
            m_asyncSubmissionThread.join();
            m_asyncSubmissionThread = {};
            m_needStartAsyncSubmissionThread = true;
        }

        // Shutdown the mirror window.
        if (m_mirrorWindowThread.joinable()) {
            // Avoid race conditions where the window will not receive the message.
            while (!m_mirrorWindowReady) {
                std::this_thread::sleep_for(100ms);
            }
            while (m_mirrorWindowHwnd) {
                PostMessage(m_mirrorWindowHwnd, WM_CLOSE, 0, 0);
            }
            m_mirrorWindowThread.join();
            m_mirrorWindowThread = {};
        }

        m_telemetry.logUsage(pvr_getTimeSeconds(m_pvr) - m_sessionStartTime, m_sessionTotalFrameCount);

#ifndef NOASEEVRCLIENT
        // Stop the eye tracker.
        if (m_eyeTrackingType == EyeTracking::aSeeVR) {
            stopDroolonTracking();
        }
#endif

        // Destroy hand trackers (tied to session).
        for (auto handTracker : m_handTrackers) {
            HandTracker* xrHandTracker = (HandTracker*)handTracker;
            delete xrHandTracker;
        }
        m_handTrackers.clear();

        // Destroy action spaces (tied to session).
        for (auto space : m_spaces) {
            Space* xrSpace = (Space*)space;
            delete xrSpace;
        }
        m_spaces.clear();
        if (m_guardianSpace) {
            delete m_guardianSpace;
        }
        delete m_originSpace;
        delete m_viewSpace;
        m_guardianSpace = m_originSpace = m_viewSpace = nullptr;

        // Destroy all swapchains (tied to session).
        while (m_swapchains.size()) {
            // TODO: Ideally we do not invoke OpenXR public APIs to avoid confusing event tracing and possible
            // deadlocks.
            CHECK_XRCMD(xrDestroySwapchain(*m_swapchains.begin()));
        }
        if (m_guardianSwapchain) {
            pvr_destroyTextureSwapChain(m_pvrSession, m_guardianSwapchain);
            m_guardianSwapchain = nullptr;
        }
        if (m_overlaySwapchain) {
            pvr_destroyTextureSwapChain(m_pvrSession, m_overlaySwapchain);
            m_overlaySwapchain = nullptr;
        }
        m_overlayBackground.Reset();

        // We do not destroy actionsets and actions, since they are tied to the instance.

        // FIXME: Add session and frame resource cleanup here.
        cleanupOpenGL();
        cleanupVulkan();
        cleanupD3D12();
        cleanupD3D11();
        cleanupSubmissionDevice();
        m_handTrackers.clear();
        m_sessionState = XR_SESSION_STATE_UNKNOWN;
        m_sessionCreated = false;
        m_sessionBegun = false;
        m_sessionLossPending = false;
        m_sessionStopping = false;
        m_sessionExiting = false;

        // Workaround: PVR ties the last use D3D device to the PVR session, and therefore we must teardown the previous
        // PVR session to clear that state. Because OpenComposite tends to call many API in unconventional order, we
        // reset the session here.
        {
            // Workaround: the environment doesn't appear to be cleared when re-initializing PVR. Clear the one pointer
            // we care about.
            m_pvrSession->envh->pvr_dxgl_interface = nullptr;

            pvr_destroySession(m_pvrSession);
            m_pvrSession = nullptr;

            ensurePvrSession();
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrBeginSession
    XrResult OpenXrRuntime::xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
        if (beginInfo->type != XR_TYPE_SESSION_BEGIN_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(
            g_traceProvider,
            "xrBeginSession",
            TLXArg(session, "Session"),
            TLArg(xr::ToCString(beginInfo->primaryViewConfigurationType), "PrimaryViewConfigurationType"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (beginInfo->primaryViewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO &&
            (!has_XR_VARJO_quad_views ||
             beginInfo->primaryViewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO)) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        if (m_sessionBegun) {
            return XR_ERROR_SESSION_RUNNING;
        }

        if (m_sessionState != XR_SESSION_STATE_READY) {
            return XR_ERROR_SESSION_NOT_READY;
        }

#ifndef NOASEEVRCLIENT
        if (m_eyeTrackingType == EyeTracking::aSeeVR) {
            startDroolonTracking();
        }
#endif

        m_primaryViewConfigurationType = beginInfo->primaryViewConfigurationType;
        if (m_primaryViewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO) {
            Log("Beginning session with quad views\n");
            LOG_TELEMETRY_ONCE(logFeature("QuadViews"));
        }

        m_useAsyncSubmission = getSetting("async_submission").value_or(true);
        m_needStartAsyncSubmissionThread = m_useAsyncSubmission;
        // Creation of the submission threads is deferred to the first xrWaitFrame() to accomodate OpenComposite quirks.

        // Re-assert our compulsive smoothing setting.
        pvr_setIntConfig(m_pvrSession, "dbg_force_framerate_divide_by", m_lockFramerate ? 2 : 1);

        m_sessionBegun = true;
        updateSessionState();

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEndSession
    XrResult OpenXrRuntime::xrEndSession(XrSession session) {
        TraceLoggingWrite(g_traceProvider, "xrEndSession", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_sessionBegun) {
            return XR_ERROR_SESSION_NOT_RUNNING;
        }

        if (m_sessionState != XR_SESSION_STATE_STOPPING) {
            return XR_ERROR_SESSION_NOT_STOPPING;
        }

        m_sessionExiting = true;
        updateSessionState();

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrRequestExitSession
    XrResult OpenXrRuntime::xrRequestExitSession(XrSession session) {
        TraceLoggingWrite(g_traceProvider, "xrRequestExitSession", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_sessionBegun || m_sessionState == XR_SESSION_STATE_IDLE || m_sessionState == XR_SESSION_STATE_EXITING) {
            return XR_ERROR_SESSION_NOT_RUNNING;
        }

        m_sessionStopping = true;
        updateSessionState();

        return XR_SUCCESS;
    }

    // Update the session state machine.
    void OpenXrRuntime::updateSessionState(bool forceSendEvent) {
        if (forceSendEvent) {
            m_sessionEventQueue.push_back(std::make_pair(m_sessionState, pvr_getTimeSeconds(m_pvr)));
        }

        while (true) {
            const auto oldSessionState = m_sessionState;
            switch (m_sessionState) {
            case XR_SESSION_STATE_IDLE:
                if (m_sessionExiting) {
                    m_sessionState = XR_SESSION_STATE_EXITING;
                } else {
                    m_sessionState = XR_SESSION_STATE_READY;
                }
                break;
            case XR_SESSION_STATE_READY:
                if (m_frameCompleted > 0) {
                    m_sessionState = XR_SESSION_STATE_SYNCHRONIZED;
                }
                break;
            case XR_SESSION_STATE_SYNCHRONIZED:
                if (m_sessionStopping) {
                    m_sessionState = XR_SESSION_STATE_STOPPING;
                } else if (m_hmdStatus.IsVisible) {
                    m_sessionState = XR_SESSION_STATE_VISIBLE;
                }
                break;
            case XR_SESSION_STATE_VISIBLE:
                if (m_sessionStopping) {
                    m_sessionState = XR_SESSION_STATE_SYNCHRONIZED;
                } else if (m_hmdStatus.HmdMounted) {
                    m_sessionState = XR_SESSION_STATE_FOCUSED;
                }
                break;
            case XR_SESSION_STATE_FOCUSED:
                if (m_sessionStopping || !m_hmdStatus.HmdMounted) {
                    m_sessionState = XR_SESSION_STATE_VISIBLE;
                }
                break;
            case XR_SESSION_STATE_STOPPING:
                if (m_sessionExiting) {
                    m_sessionState = XR_SESSION_STATE_IDLE;
                }
                break;
            }

            if (m_sessionState != oldSessionState) {
                TraceLoggingWrite(g_traceProvider,
                                  "PXR_State",
                                  TLArg(xr::ToCString(oldSessionState), "From"),
                                  TLArg(xr::ToCString(m_sessionState), "To"));

                m_sessionEventQueue.push_back(std::make_pair(m_sessionState, pvr_getTimeSeconds(m_pvr)));
            } else {
                break;
            }
        }

        TraceLoggingWrite(g_traceProvider, "PXR_State", TLArg(xr::ToCString(m_sessionState), "Current"));
    }

    // Read dynamic settings from the registry.
    void OpenXrRuntime::refreshSettings() {
        // Value is in unit of hundredth.
        m_joystickDeadzone = getSetting("joystick_deadzone").value_or(2) / 100.f;

        const auto forcedInteractionProfile = getSetting("force_interaction_profile").value_or(0);
        if (forcedInteractionProfile == 1) {
            m_forcedInteractionProfile = ForcedInteractionProfile::OculusTouchController;
        } else if (forcedInteractionProfile == 2) {
            m_forcedInteractionProfile = ForcedInteractionProfile::MicrosoftMotionController;
        } else {
            m_forcedInteractionProfile.reset();
        }

        if (getSetting("guardian").value_or(true)) {
            m_guardianThreshold = getSetting("guardian_threshold").value_or(1100) / 1e3f;
            m_guardianRadius = getSetting("guardian_radius").value_or(1600) / 1e3f;
        } else {
            m_guardianThreshold = INFINITY;
        }

        const auto oldControllerAimOffset = m_controllerAimOffset;
        m_controllerAimOffset = Pose::MakePose(
            Quaternion::RotationRollPitchYaw({PVR::DegreeToRad((float)getSetting("aim_pose_rot_x").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("aim_pose_rot_y").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("aim_pose_rot_z").value_or(0.f))}),
            XrVector3f{getSetting("aim_pose_offset_x").value_or(0.f) / 1000.f,
                       getSetting("aim_pose_offset_y").value_or(0.f) / 1000.f,
                       getSetting("aim_pose_offset_z").value_or(0.f) / 1000.f});

        const auto oldControllerGripOffset = m_controllerGripOffset;
        m_controllerGripOffset = Pose::MakePose(
            Quaternion::RotationRollPitchYaw({PVR::DegreeToRad((float)getSetting("grip_pose_rot_x").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("grip_pose_rot_y").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("grip_pose_rot_z").value_or(0.f))}),
            XrVector3f{getSetting("grip_pose_offset_x").value_or(0) / 1000.f,
                       getSetting("grip_pose_offset_y").value_or(0) / 1000.f,
                       getSetting("grip_pose_offset_z").value_or(0) / 1000.f});

        // Force re-evaluating poses.
        if (!Pose::Equals(oldControllerAimOffset, m_controllerAimOffset) ||
            !Pose::Equals(oldControllerGripOffset, m_controllerGripOffset)) {
            m_cachedControllerType[0].clear();
            m_cachedControllerType[1].clear();
        }

        // Value is already in microseconds.
        m_frameTimeOverrideOffsetUs = getSetting("frame_time_override_offset").value_or(0);

        // Multiplier is a percentage. Convert to milliseconds (*10) then convert the whole expression (including frame
        // duration) from milliseconds to microseconds.
        m_frameTimeOverrideUs =
            (uint64_t)(getSetting("frame_time_override_multiplier").value_or(0) * 10.f * m_idealFrameDuration * 1000.f);

        m_frameTimeFilterLength = getSetting("frame_time_filter_length").value_or(5);

        m_useMirrorWindow = getSetting("mirror_window").value_or(false);

        m_droolonProjectionDistance = getSetting("droolon_projection_distance").value_or(35) / 100.f;

        m_useDeferredFrameWait = getSetting("defer_frame_wait").value_or(false);
        m_lockFramerate = getSetting("lock_framerate").value_or(false);

        m_postProcessFocusView = getSetting("postprocess_focus_view").value_or(true);

        m_honorPremultiplyFlagOnProj0 = getSetting("honor_premultiply_flag_on_proj0").value_or(false);

        m_swapGripAimPoses = getSetting("quirk_swap_grip_aim_poses").value_or(false);

        m_useRunningStart = !getSetting("quirk_disable_running_start").value_or(false);

        m_syncGpuWorkInEndFrame = getSetting("quirk_sync_gpu_work_in_end_frame").value_or(false);

        TraceLoggingWrite(
            g_traceProvider,
            "PXR_Config",
            TLArg(m_joystickDeadzone, "JoystickDeadzone"),
            TLArg((int)m_forcedInteractionProfile.value_or((ForcedInteractionProfile)-1), "ForcedInteractionProfile"),
            TLArg(m_guardianThreshold, "GuardianThreshold"),
            TLArg(m_guardianRadius, "GuardianRadius"),
            TLArg(m_frameTimeOverrideOffsetUs, "FrameTimeOverrideOffset"),
            TLArg(m_frameTimeOverrideUs, "FrameTimeOverride"),
            TLArg(m_frameTimeFilterLength, "FrameTimeFilterLength"),
            TLArg(m_useMirrorWindow, "MirrorWindow"),
            TLArg(m_droolonProjectionDistance, "DroolonProjectionDistance"),
            TLArg(m_useDeferredFrameWait, "UseDeferredFrameWait"),
            TLArg(m_lockFramerate, "LockFramerate"),
            TLArg(m_postProcessFocusView, "PostProcessFocusView"),
            TLArg(m_honorPremultiplyFlagOnProj0, "HonorPremultiplyFlagOnProj0"),
            TLArg(m_swapGripAimPoses, "SwapGripAimPoses"),
            TLArg(m_useRunningStart, "UseRunningStart"),
            TLArg(m_syncGpuWorkInEndFrame, "SyncGpuWorkInEndFrame"));

        if (m_pvrSession) {
            pvr_setIntConfig(m_pvrSession, "dbg_force_framerate_divide_by", m_lockFramerate ? 2 : 1);
        }

        const auto debugControllerType = getSetting("debug_controller_type").value_or(0);
        if (debugControllerType == 1) {
            m_debugControllerType = "vive_controller";
        } else if (debugControllerType == 2) {
            m_debugControllerType = "knuckles";
        } else if (debugControllerType == 3) {
            m_debugControllerType = "pimax_crystal";
        } else {
            m_debugControllerType.clear();
        }

        m_debugFocusViews = getSetting("debug_focus_view").value_or(false);
    }

    // Create guardian resources.
    void OpenXrRuntime::initializeGuardianResources() {
        HRESULT hr;

        // Load the guardian texture.
        auto image = std::make_unique<DirectX::ScratchImage>();
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        hr = DirectX::LoadFromWICFile((dllHome / L"guardian.png").c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image);

        if (SUCCEEDED(hr)) {
            ComPtr<ID3D11Resource> texture;
            hr = DirectX::CreateTexture(m_pvrSubmissionDevice.Get(),
                                        image->GetImages(),
                                        1,
                                        image->GetMetadata(),
                                        texture.ReleaseAndGetAddressOf());

            if (SUCCEEDED(hr)) {
                // Create a PVR swapchain for the texture.
                pvrTextureSwapChainDesc desc{};
                desc.Type = pvrTexture_2D;
                desc.StaticImage = true;
                desc.ArraySize = 1;
                desc.Width = m_guardianExtent.width = (int)image->GetMetadata().width;
                desc.Height = m_guardianExtent.height = (int)image->GetMetadata().height;
                desc.MipLevels = (int)image->GetMetadata().mipLevels;
                desc.SampleCount = 1;
                desc.Format = dxgiToPvrTextureFormat(image->GetMetadata().format);

                CHECK_PVRCMD(pvr_createTextureSwapChainDX(
                    m_pvrSession, m_pvrSubmissionDevice.Get(), &desc, &m_guardianSwapchain));

                // Copy and commit the guardian texture to the swapchain.
                int imageIndex = -1;
                CHECK_PVRCMD(pvr_getTextureSwapChainCurrentIndex(m_pvrSession, m_guardianSwapchain, &imageIndex));
                ComPtr<ID3D11Texture2D> swapchainTexture;
                CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(m_pvrSession,
                                                             m_guardianSwapchain,
                                                             imageIndex,
                                                             IID_PPV_ARGS(swapchainTexture.ReleaseAndGetAddressOf())));

                m_pvrSubmissionContext->CopyResource(swapchainTexture.Get(), texture.Get());
                m_pvrSubmissionContext->Flush();
                CHECK_PVRCMD(pvr_commitTextureSwapChain(m_pvrSession, m_guardianSwapchain));
            } else {
                ErrorLog("Failed to create texture from guardian.png: %X\n");
            }
        } else {
            ErrorLog("Failed to load guardian.png: %X\n");
        }

        // Create the guardian reference space, 1m below eyesight, flat on the floor.
        m_guardianSpace = new Space;
        m_guardianSpace->referenceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        m_guardianSpace->poseInSpace =
            Pose::MakePose(Quaternion::RotationRollPitchYaw({PVR::DegreeToRad(-90.f), 0.f, 0.f}), XrVector3f{0, -1, 0});
    }

} // namespace pimax_openxr
