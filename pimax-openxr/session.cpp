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

        if (!m_graphicsRequirementQueried) {
            return XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING;
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
                const XrGraphicsBindingD3D11KHR* d3dBindings =
                    reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(entry);

                const auto result = initializeD3D11(*d3dBindings);
                if (XR_FAILED(result)) {
                    return result;
                }

                hasGraphicsBindings = true;
                break;
            } else if (has_XR_KHR_D3D12_enable && entry->type == XR_TYPE_GRAPHICS_BINDING_D3D12_KHR) {
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
                const XrGraphicsBindingVulkanKHR* vkBindings =
                    reinterpret_cast<const XrGraphicsBindingVulkanKHR*>(entry);

                const auto result = initializeVulkan(*vkBindings);
                if (XR_FAILED(result)) {
                    return result;
                }

                hasGraphicsBindings = true;
                break;
            } else if (has_XR_KHR_opengl_enable && entry->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR) {
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

        // Create guardian resources.
        {
            HRESULT hr;

            // Load the guardian texture.
            auto image = std::make_unique<DirectX::ScratchImage>();
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            hr =
                DirectX::LoadFromWICFile((dllHome / L"guardian.png").c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image);

            if (SUCCEEDED(hr)) {
                ComPtr<ID3D11Resource> texture;
                hr = DirectX::CreateTexture(
                    m_d3d11Device.Get(), image->GetImages(), 1, image->GetMetadata(), texture.ReleaseAndGetAddressOf());

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

                    CHECK_PVRCMD(
                        pvr_createTextureSwapChainDX(m_pvrSession, m_d3d11Device.Get(), &desc, &m_guardianSwapchain));

                    // Copy and commit the guardian texture to the swapchain.
                    int imageIndex = -1;
                    CHECK_PVRCMD(pvr_getTextureSwapChainCurrentIndex(m_pvrSession, m_guardianSwapchain, &imageIndex));
                    ID3D11Texture2D* swapchainTexture;
                    CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
                        m_pvrSession, m_guardianSwapchain, imageIndex, IID_PPV_ARGS(&swapchainTexture)));
                    m_d3d11DeviceContext->CopyResource(swapchainTexture, texture.Get());
                    m_d3d11DeviceContext->Flush();
                    CHECK_PVRCMD(pvr_commitTextureSwapChain(m_pvrSession, m_guardianSwapchain));
                } else {
                    ErrorLog("Failed to create texture from guardian.png: %X\n");
                }
            } else {
                ErrorLog("Failed to load guardian.png: %X\n");
            }

            // Create the guardian reference space, 1m below eyesight, flat on the floor.
            Space& xrSpace = *new Space;
            xrSpace.referenceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            xrSpace.poseInSpace = Pose::MakePose(Quaternion::RotationRollPitchYaw({PVR::DegreeToRad(-90.f), 0.f, 0.f}),
                                                 XrVector3f{0, -1, 0});

            m_guardianSpace = (XrSpace)&xrSpace;

            // Maintain a list of known spaces for validation and cleanup.
            m_spaces.insert(m_guardianSpace);
        }

        // Read configuration and set up the session accordingly.
        if (getSetting("recenter_on_startup").value_or(1)) {
            CHECK_PVRCMD(pvr_recenterTrackingOrigin(m_pvrSession));
        }
        refreshSettings();

        {
            const bool enableLighthouse = !!pvr_getIntConfig(m_pvrSession, "enable_lighthouse_tracking", 0);
            const int fovLevel = pvr_getIntConfig(m_pvrSession, "fov_level", 1);

            TraceLoggingWrite(
                g_traceProvider,
                "PVR_Config",
                TLArg(enableLighthouse, "EnableLighthouse"),
                TLArg(fovLevel, "FovLevel"),
                TLArg(m_useParallelProjection, "UseParallelProjection"),
                TLArg(!!pvr_getIntConfig(m_pvrSession, "dbg_asw_enable", 0), "EnableSmartSmoothing"),
                TLArg(pvr_getIntConfig(m_pvrSession, "dbg_force_framerate_divide_by", 1), "CompulsiveSmoothingRate"));

            m_telemetry.logScenario(isD3D12Session()    ? "D3D12"
                                    : isVulkanSession() ? "Vulkan"
                                    : isOpenGLSession() ? "OpenGL"
                                                        : "D3D11",
                                    enableLighthouse,
                                    fovLevel,
                                    m_useParallelProjection);
        }

        m_sessionCreated = true;

        // FIXME: Reset the session and frame state here.
        m_sessionState = XR_SESSION_STATE_IDLE;
        m_sessionStateDirty = true;
        m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

        m_frameWaited = m_frameBegun = m_frameCompleted = 0;

        m_frameTimes.clear();

        m_isControllerActive[0] = m_isControllerActive[1] = false;
        rebindControllerActions(0);
        rebindControllerActions(1);
        m_activeActionSets.clear();
        m_validActionSets.clear();

        m_sessionStartTime = m_sessionStateEventTime;
        m_sessionTotalFrameCount = 0;

        try {
            // Create a reference space with the origin and the HMD pose.
            {
                XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                spaceInfo.poseInReferenceSpace = Pose::Identity();
                spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
                CHECK_XRCMD(xrCreateReferenceSpace((XrSession)1, &spaceInfo, &m_originSpace));
            }
            {
                XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                spaceInfo.poseInReferenceSpace = Pose::Identity();
                spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                CHECK_XRCMD(xrCreateReferenceSpace((XrSession)1, &spaceInfo, &m_viewSpace));
            }
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

        m_telemetry.logUsage(pvr_getTimeSeconds(m_pvr) - m_sessionStartTime, m_sessionTotalFrameCount);

        // Destroy all swapchains.
        while (m_swapchains.size()) {
            CHECK_XRCMD(xrDestroySwapchain(*m_swapchains.begin()));
        }
        if (m_guardianSwapchain) {
            pvr_destroyTextureSwapChain(m_pvrSession, m_guardianSwapchain);
            m_guardianSwapchain = nullptr;
        }

        // Destroy reference spaces.
        CHECK_XRCMD(xrDestroySpace(m_originSpace));
        m_originSpace = XR_NULL_HANDLE;
        CHECK_XRCMD(xrDestroySpace(m_viewSpace));
        m_viewSpace = XR_NULL_HANDLE;

        // FIXME: Add session and frame resource cleanup here.
        cleanupOpenGL();
        cleanupVulkan();
        cleanupD3D12();
        cleanupD3D11();
        m_handTrackers.clear();
        m_sessionState = XR_SESSION_STATE_UNKNOWN;
        m_sessionStateDirty = false;
        m_sessionCreated = false;
        m_sessionExiting = false;

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

        if (beginInfo->primaryViewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        if (m_sessionState != XR_SESSION_STATE_IDLE && m_sessionState != XR_SESSION_STATE_READY) {
            return XR_ERROR_SESSION_NOT_READY;
        }

        m_sessionState = XR_SESSION_STATE_SYNCHRONIZED;
        m_sessionStateDirty = true;
        m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEndSession
    XrResult OpenXrRuntime::xrEndSession(XrSession session) {
        TraceLoggingWrite(g_traceProvider, "xrEndSession", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (m_sessionState != XR_SESSION_STATE_STOPPING) {
            return XR_ERROR_SESSION_NOT_STOPPING;
        }

        m_sessionExiting = true;

        m_sessionState = XR_SESSION_STATE_IDLE;
        m_sessionStateDirty = true;
        m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrRequestExitSession
    XrResult OpenXrRuntime::xrRequestExitSession(XrSession session) {
        TraceLoggingWrite(g_traceProvider, "xrRequestExitSession", TLXArg(session, "Session"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (m_sessionState != XR_SESSION_STATE_SYNCHRONIZED && m_sessionState != XR_SESSION_STATE_VISIBLE &&
            m_sessionState != XR_SESSION_STATE_FOCUSED) {
            return XR_ERROR_SESSION_NOT_RUNNING;
        }

        m_sessionState = XR_SESSION_STATE_STOPPING;
        m_sessionStateDirty = true;
        m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

        return XR_SUCCESS;
    }

    // Read dynamic settings from the registry.
    void OpenXrRuntime::refreshSettings() {
        // Value is in unit of hundredth.
        m_joystickDeadzone = getSetting("joystick_deadzone").value_or(2) / 100.f;

        m_swapGripAimPoses = getSetting("swap_grip_aim_poses").value_or(0);
        const auto forcedInteractionProfile = getSetting("force_interaction_profile").value_or(0);
        if (forcedInteractionProfile == 1) {
            m_forcedInteractionProfile = ForcedInteractionProfile::OculusTouchController;
        } else if (forcedInteractionProfile == 2) {
            m_forcedInteractionProfile = ForcedInteractionProfile::MicrosoftMotionController;
        } else {
            m_forcedInteractionProfile.reset();
        }

        if (getSetting("guardian").value_or(1)) {
            m_guardianThreshold = getSetting("guardian_threshold").value_or(1100) / 1e3f;
            m_guardianRadius = getSetting("guardian_radius").value_or(1600) / 1e3f;
        } else {
            m_guardianThreshold = INFINITY;
        }

        m_controllerAimOffset = Pose::MakePose(
            Quaternion::RotationRollPitchYaw({PVR::DegreeToRad((float)getSetting("aim_pose_rot_x").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("aim_pose_rot_y").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("aim_pose_rot_z").value_or(0.f))}),
            XrVector3f{getSetting("aim_pose_offset_x").value_or(0.f) / 1000.f,
                       getSetting("aim_pose_offset_y").value_or(0.f) / 1000.f,
                       getSetting("aim_pose_offset_z").value_or(0.f) / 1000.f});
        m_controllerGripOffset = Pose::MakePose(
            Quaternion::RotationRollPitchYaw({PVR::DegreeToRad((float)getSetting("grip_pose_rot_x").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("grip_pose_rot_y").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("grip_pose_rot_z").value_or(0.f))}),
            XrVector3f{getSetting("grip_pose_offset_x").value_or(0) / 1000.f,
                       getSetting("grip_pose_offset_y").value_or(0) / 1000.f,
                       getSetting("grip_pose_offset_z").value_or(0) / 1000.f});

        m_controllerPalmOffset = Pose::MakePose(
            Quaternion::RotationRollPitchYaw({PVR::DegreeToRad((float)getSetting("palm_pose_rot_x").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("palm_pose_rot_y").value_or(0.f)),
                                              PVR::DegreeToRad((float)getSetting("palm_pose_rot_z").value_or(0.f))}),
            XrVector3f{getSetting("palm_pose_offset_x").value_or(0) / 1000.f,
                       getSetting("palm_pose_offset_y").value_or(0) / 1000.f,
                       getSetting("palm_pose_offset_z").value_or(0) / 1000.f});

        // Force re-evaluating poses.
        m_cachedControllerType[0].clear();
        m_cachedControllerType[1].clear();

        // Value is already in microseconds.
        m_frameTimeOverrideOffsetUs = getSetting("frame_time_override_offset").value_or(0);

        // Multiplier is a percentage. Convert to milliseconds (*10) then convert the whole expression (including frame
        // duration) from milliseconds to microseconds.
        m_frameTimeOverrideUs =
            (uint64_t)(getSetting("frame_time_override_multiplier").value_or(0) * 10.f * m_frameDuration * 1000.f);

        m_frameTimeFilterLength = getSetting("frame_time_filter_length").value_or(5);

        TraceLoggingWrite(
            g_traceProvider,
            "PXR_Config",
            TLArg(m_joystickDeadzone, "JoystickDeadzone"),
            TLArg(m_swapGripAimPoses, "SwapGripAimPoses"),
            TLArg((int)m_forcedInteractionProfile.value_or((ForcedInteractionProfile)-1), "ForcedInteractionProfile"),
            TLArg(m_guardianThreshold, "GuardianThreshold"),
            TLArg(m_guardianRadius, "GuardianRadius"),
            TLArg(m_frameTimeOverrideOffsetUs, "FrameTimeOverrideOffset"),
            TLArg(m_frameTimeOverrideUs, "FrameTimeOverride"),
            TLArg(m_frameTimeFilterLength, "FrameTimeFilterLength"));
    }

} // namespace pimax_openxr
