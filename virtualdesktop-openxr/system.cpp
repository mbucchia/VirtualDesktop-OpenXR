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

// From our OVR_CAPIShim.c fork.
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_InitializeWithPathOverride(const ovrInitParams* inputParams, const wchar_t* overrideLibraryPath);
OVR_PUBLIC_FUNCTION(ovrResult) ovr_ReInitialize(const ovrInitParams* inputParams);

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetSystem
    XrResult OpenXrRuntime::xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) {
        if (getInfo->type != XR_TYPE_SYSTEM_GET_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetSystem",
                          TLXArg(instance, "Instance"),
                          TLArg(xr::ToCString(getInfo->formFactor), "FormFactor"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (getInfo->formFactor != XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) {
            return XR_ERROR_FORM_FACTOR_UNSUPPORTED;
        }

        // This is the latest point where we can defer initialization of LibOVR and the OVR session.
        if (!ensureOVRSession()) {
            m_cachedHmdInfo = {};
            return XR_ERROR_FORM_FACTOR_UNAVAILABLE;
        }

        m_systemCreated = true;

        *systemId = (XrSystemId)1;

        TraceLoggingWrite(g_traceProvider, "xrGetSystem", TLArg((int)*systemId, "SystemId"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetSystemProperties
    XrResult OpenXrRuntime::xrGetSystemProperties(XrInstance instance,
                                                  XrSystemId systemId,
                                                  XrSystemProperties* properties) {
        if (properties->type != XR_TYPE_SYSTEM_PROPERTIES) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(
            g_traceProvider, "xrGetSystemProperties", TLXArg(instance, "Instance"), TLArg((int)systemId, "SystemId"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        XrSystemHandTrackingPropertiesEXT* handTrackingProperties =
            reinterpret_cast<XrSystemHandTrackingPropertiesEXT*>(properties->next);
        while (handTrackingProperties) {
            if (handTrackingProperties->type == XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT) {
                break;
            }
            handTrackingProperties = reinterpret_cast<XrSystemHandTrackingPropertiesEXT*>(handTrackingProperties->next);
        }

        XrSystemEyeGazeInteractionPropertiesEXT* eyeGazeInteractionProperties =
            reinterpret_cast<XrSystemEyeGazeInteractionPropertiesEXT*>(properties->next);
        while (eyeGazeInteractionProperties) {
            if (eyeGazeInteractionProperties->type == XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT) {
                break;
            }
            eyeGazeInteractionProperties =
                reinterpret_cast<XrSystemEyeGazeInteractionPropertiesEXT*>(eyeGazeInteractionProperties->next);
        }

        XrSystemEyeTrackingPropertiesFB* eyeTrackingProperties =
            reinterpret_cast<XrSystemEyeTrackingPropertiesFB*>(properties->next);
        while (eyeTrackingProperties) {
            if (eyeTrackingProperties->type == XR_TYPE_SYSTEM_EYE_TRACKING_PROPERTIES_FB) {
                break;
            }
            eyeTrackingProperties = reinterpret_cast<XrSystemEyeTrackingPropertiesFB*>(eyeTrackingProperties->next);
        }

        XrSystemFaceTrackingPropertiesFB* faceTrackingProperties =
            reinterpret_cast<XrSystemFaceTrackingPropertiesFB*>(properties->next);
        while (faceTrackingProperties) {
            if (faceTrackingProperties->type == XR_TYPE_SYSTEM_FACE_TRACKING_PROPERTIES_FB) {
                break;
            }
            faceTrackingProperties = reinterpret_cast<XrSystemFaceTrackingPropertiesFB*>(faceTrackingProperties->next);
        }

        XrSystemHeadsetIdPropertiesMETA* headsetIdProperties =
            reinterpret_cast<XrSystemHeadsetIdPropertiesMETA*>(properties->next);
        while (headsetIdProperties) {
            if (headsetIdProperties->type == XR_TYPE_SYSTEM_HEADSET_ID_PROPERTIES_META) {
                break;
            }
            headsetIdProperties = reinterpret_cast<XrSystemHeadsetIdPropertiesMETA*>(headsetIdProperties->next);
        }

        properties->vendorId = m_cachedHmdInfo.VendorId;

        sprintf_s(properties->systemName, sizeof(properties->systemName), "%s", m_cachedHmdInfo.ProductName);
        properties->systemId = systemId;

        properties->trackingProperties.positionTracking = XR_TRUE;
        properties->trackingProperties.orientationTracking = XR_TRUE;

        static_assert(ovrMaxLayerCount >= XR_MIN_COMPOSITION_LAYERS_SUPPORTED);
        properties->graphicsProperties.maxLayerCount = ovrMaxLayerCount;
        properties->graphicsProperties.maxSwapchainImageWidth = 16384;
        properties->graphicsProperties.maxSwapchainImageHeight = 16384;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetSystemProperties",
                          TLArg((int)properties->systemId, "SystemId"),
                          TLArg(properties->vendorId, "VendorId"),
                          TLArg(properties->systemName, "SystemName"),
                          TLArg(!!properties->trackingProperties.positionTracking, "PositionTracking"),
                          TLArg(!!properties->trackingProperties.orientationTracking, "OrientationTracking"),
                          TLArg(properties->graphicsProperties.maxLayerCount, "MaxLayerCount"),
                          TLArg(properties->graphicsProperties.maxSwapchainImageWidth, "MaxSwapchainImageWidth"),
                          TLArg(properties->graphicsProperties.maxSwapchainImageHeight, "MaxSwapchainImageHeight"));

        if (has_XR_EXT_hand_tracking && handTrackingProperties) {
            handTrackingProperties->supportsHandTracking =
                (m_handJointsState && m_handJointsState->HandTrackingActive) ? XR_TRUE : XR_FALSE;

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystemProperties",
                              TLArg((int)properties->systemId, "SystemId"),
                              TLArg(!!handTrackingProperties->supportsHandTracking, "SupportsHandTracking"));
        }

        if (has_XR_EXT_eye_gaze_interaction && eyeGazeInteractionProperties) {
            eyeGazeInteractionProperties->supportsEyeGazeInteraction =
                (m_eyeTrackingType != EyeTracking::None) ? XR_TRUE : XR_FALSE;

            TraceLoggingWrite(
                g_traceProvider,
                "xrGetSystemProperties",
                TLArg(!!eyeGazeInteractionProperties->supportsEyeGazeInteraction, "SupportsEyeGazeInteraction"));
        }

        if (has_XR_FB_eye_tracking_social && eyeTrackingProperties) {
            eyeTrackingProperties->supportsEyeTracking = m_faceState ? XR_TRUE : XR_FALSE;

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystemProperties",
                              TLArg(!!eyeTrackingProperties->supportsEyeTracking, "SupportsEyeTracking"));
        }

        if (has_XR_FB_face_tracking && faceTrackingProperties) {
            faceTrackingProperties->supportsFaceTracking = m_faceState ? XR_TRUE : XR_FALSE;

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystemProperties",
                              TLArg(!!faceTrackingProperties->supportsFaceTracking, "SupportsFaceTracking"));
        }

        if (has_XR_META_headset_id && headsetIdProperties) {
            uint8_t uuid[] = {82, 80, 120, 165, 90, 171, 77, 201, 184, 2, 30, 189, 108, 124, 255, 244};
            memcpy(&headsetIdProperties->id, uuid, sizeof(uuid));
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateEnvironmentBlendModes
    XrResult OpenXrRuntime::xrEnumerateEnvironmentBlendModes(XrInstance instance,
                                                             XrSystemId systemId,
                                                             XrViewConfigurationType viewConfigurationType,
                                                             uint32_t environmentBlendModeCapacityInput,
                                                             uint32_t* environmentBlendModeCountOutput,
                                                             XrEnvironmentBlendMode* environmentBlendModes) {
        // We only support immersive VR mode.
        static const XrEnvironmentBlendMode blendModes[] = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateEnvironmentBlendModes",
                          TLXArg(instance, "Instance"),
                          TLArg((int)systemId, "SystemId"),
                          TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"),
                          TLArg(environmentBlendModeCapacityInput, "EnvironmentBlendModeCapacityInput"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_systemCreated || systemId != (XrSystemId)1) {
            return XR_ERROR_SYSTEM_INVALID;
        }

        if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        if (environmentBlendModeCapacityInput && environmentBlendModeCapacityInput < ARRAYSIZE(blendModes)) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *environmentBlendModeCountOutput = ARRAYSIZE(blendModes);
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateEnvironmentBlendModes",
                          TLArg(*environmentBlendModeCountOutput, "EnvironmentBlendModeCountOutput"));

        if (environmentBlendModeCapacityInput && environmentBlendModes) {
            for (uint32_t i = 0; i < *environmentBlendModeCountOutput; i++) {
                environmentBlendModes[i] = blendModes[i];
                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateEnvironmentBlendModes",
                                  TLArg(xr::ToCString(environmentBlendModes[i]), "EnvironmentBlendMode"));
            }
        }

        return XR_SUCCESS;
    }

    bool OpenXrRuntime::initializeOVR() {
#ifndef STANDALONE_RUNTIME
        // The bundled runtime is meant to only work with Virtual Desktop.
        m_useOculusRuntime = false;
#else
        m_useOculusRuntime = !IsServiceRunning(L"VirtualDesktop.Server.exe");
#endif
        if (m_useOculusRuntime && !getSetting("allow_oculus_runtime").value_or(true)) {
            // Indicate that Virtual Desktop is required by the current configuration.
            OnceLog("Virtual Desktop Server is not running\n");
            return false;
        }

        std::wstring overridePath;
        if (!m_useOculusRuntime) {
            // Locate Virtual Desktop's LibOVR.
            std::filesystem::path path(
                RegGetString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Virtual Desktop, Inc.\\Virtual Desktop Streamer", "Path")
                    .value());
            path = path / L"VirtualDesktop.";

            overridePath = path.wstring();
        }

        // Initialize OVR.
        ovrInitParams initParams{};
        initParams.Flags = ovrInit_RequestVersion | (has_XR_MND_headless ? ovrInit_Invisible : ovrInit_FocusAware);
        initParams.RequestedMinorVersion = OVR_MINOR_VERSION;
        const ovrResult result =
            ovr_InitializeWithPathOverride(&initParams, overridePath.empty() ? nullptr : overridePath.c_str());
        TraceLoggingWrite(g_traceProvider,
                          "OVR_Initialize",
                          TLArg(overridePath.c_str(), "OverridePath"),
                          TLArg((int)result, "Result"));
        if (result == ovrError_LibLoad) {
            // This would happen on Pico. Indicate that Virtual Desktop is required.
            OnceLog("Virtual Desktop Server is not running\n");
            return false;
        } else if (result == ovrError_ServiceConnection || result == ovrError_RemoteSession) {
            return false;
        }
        CHECK_OVRCMD(result);

        Log("Using %s runtime\n", !m_useOculusRuntime ? "Virtual Desktop" : "Oculus");

        if (!m_useOculusRuntime) {
            identifyVirtualDesktop();
        }

        std::string_view versionString(ovr_GetVersionString());
        Log("OVR: %s\n", versionString.data());
        TraceLoggingWrite(g_traceProvider, "OVR_SDK", TLArg(versionString.data(), "VersionString"));

        m_isOVRLoaded = true;
        m_ovrSession = nullptr;

        return true;
    }

    void OpenXrRuntime::identifyVirtualDesktop() {
        std::wstring version(
            RegGetString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Virtual Desktop, Inc.\\Virtual Desktop Streamer", "Version")
                .value_or(L"Unknown"));
        Log("Streamer: %ls\n", version.c_str());
        TraceLoggingWrite(g_traceProvider, "VirtualDesktopVersion", TLArg(version.data(), "Version"));

        try {
            std::stringstream ss(xr::wide_to_utf8(version));
            std::string component;
            std::getline(ss, component, '.');
            const int major = std::stoi(component);
            std::getline(ss, component, '.');
            const int minor = std::stoi(component);
            std::getline(ss, component, '.');
            const int release = std::stoi(component);

            // FIXME: Identify version-specific quirks.
            m_alwaysAdvertiseEyeTracking = major == 1 && (minor < 29 || (minor == 29 && release < 2));

        } catch (std::exception&) {
        }
    }

    void OpenXrRuntime::enterVisibleMode() {
        ovrInitParams initParams{};
        initParams.Flags = ovrInit_RequestVersion | ovrInit_FocusAware;
        initParams.RequestedMinorVersion = OVR_MINOR_VERSION;
        CHECK_OVRCMD(ovr_ReInitialize(&initParams));
        TraceLoggingWrite(g_traceProvider, "OVR_ReInitialize");

        ovr_Destroy(m_ovrSession);
        m_ovrSession = nullptr;

        CHECK_MSG(ensureOVRSession(), "Failed to enter invisible mode\n");
    }

    bool OpenXrRuntime::ensureOVRSession() {
        if (m_ovrSession) {
            return true;
        }

        if (!m_isOVRLoaded) {
            if (!initializeOVR()) {
                return false;
            }
        }

        const ovrResult result = ovr_Create(&m_ovrSession, reinterpret_cast<ovrGraphicsLuid*>(&m_adapterLuid));
        TraceLoggingWrite(g_traceProvider, "OVR_Create", TLArg((int)result, "Result"));
        if (result == ovrError_NoHmd) {
            return false;
        }
        CHECK_OVRCMD(result);

        // Force Virtual Desktop to enter visible mode. This will make sure we transition our state machine later.
        ovrSessionStatus status{};
        CHECK_OVRCMD(ovr_GetSessionStatus(m_ovrSession, &status));

        // Tell Virtual Desktop that this is a VirtualDesktopXR session.
        if (!m_useOculusRuntime) {
            ovr_SetBool(m_ovrSession, "IsVDXR", true);
        }

        // Calibrate the timestamp conversion.
        m_ovrTimeFromQpcTimeOffset = INFINITY;
        for (int i = 0; i < 100; i++) {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            const double qpcTime = (double)now.QuadPart / m_qpcFrequency.QuadPart;
            m_ovrTimeFromQpcTimeOffset = std::min(m_ovrTimeFromQpcTimeOffset, ovr_GetTimeInSeconds() - qpcTime);
        }
        TraceLoggingWrite(
            g_traceProvider, "ConvertTime", TLArg(m_ovrTimeFromQpcTimeOffset, "OvrTimeFromQpcTimeOffset"));

        m_isLowVideoMemorySystem = false;
        {
            // Detect low memory systems.
            ComPtr<IDXGIFactory1> dxgiFactory;
            CHECK_HRCMD(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

            ComPtr<IDXGIAdapter1> dxgiAdapter;
            for (UINT adapterIndex = 0;; adapterIndex++) {
                // EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to
                // enumerate.
                CHECK_HRCMD(dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf()));

                DXGI_ADAPTER_DESC1 desc;
                CHECK_HRCMD(dxgiAdapter->GetDesc1(&desc));
                if (!memcmp(&desc.AdapterLuid, &m_adapterLuid, sizeof(LUID))) {
                    ComPtr<IDXGIAdapter3> dxgiAdapter3;
                    if (SUCCEEDED(dxgiAdapter->QueryInterface(dxgiAdapter3.ReleaseAndGetAddressOf()))) {
                        DXGI_QUERY_VIDEO_MEMORY_INFO queryVideoMemory;
                        if (SUCCEEDED(dxgiAdapter3->QueryVideoMemoryInfo(
                                0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &queryVideoMemory))) {
                            m_isLowVideoMemorySystem = queryVideoMemory.Budget < 3'758'096'384;
                        }
                    }
                    break;
                }
            }
        }

        initializeSystem();

        return true;
    }

    void OpenXrRuntime::initializeSystem() {
        // Query HMD properties.
        const ovrHmdDesc hmdInfo = ovr_GetHmdDesc(m_ovrSession);
        TraceLoggingWrite(g_traceProvider,
                          "OVR_HmdDesc",
                          TLArg((int)hmdInfo.Type, "Type"),
                          TLArg(hmdInfo.VendorId, "VendorId"),
                          TLArg(hmdInfo.ProductId, "ProductId"),
                          TLArg(hmdInfo.Manufacturer, "Manufacturer"),
                          TLArg(hmdInfo.ProductName, "ProductName"),
                          TLArg(hmdInfo.SerialNumber, "SerialNumber"),
                          TLArg(hmdInfo.FirmwareMinor, "FirmwareMinor"),
                          TLArg(hmdInfo.FirmwareMajor, "FirmwareMajor"),
                          TLArg(hmdInfo.Resolution.w, "ResolutionWidth"),
                          TLArg(hmdInfo.Resolution.h, "ResolutionHeight"),
                          TLArg(hmdInfo.DisplayRefreshRate, "DisplayRefreshRate"));

        // Detect if the device changed.
        if (std::string_view(m_cachedHmdInfo.SerialNumber) != hmdInfo.SerialNumber) {
            m_cachedHmdInfo = hmdInfo;
            Log("Device is: %s (%d)\n", m_cachedHmdInfo.ProductName, m_cachedHmdInfo.Type);

            // Try initializing the face and eye tracking data through Virtual Desktop, for supported devices only.
            if (!m_useOculusRuntime && (m_cachedHmdInfo.Type == ovrHmd_QuestPro || m_alwaysAdvertiseEyeTracking)) {
                initializeFaceTrackingMmf();
            }

            m_eyeTrackingType = EyeTracking::None;
            if (!getSetting("simulate_eye_tracking").value_or(false)) {
                if (m_faceState) {
                    m_eyeTrackingType = EyeTracking::Mmf;
                }
            } else {
                m_eyeTrackingType = EyeTracking::Simulated;
            }

            // Cache common information.
            m_displayRefreshRate = hmdInfo.DisplayRefreshRate;
            m_idealFrameDuration = m_predictedFrameDuration = 1.0 / hmdInfo.DisplayRefreshRate;
            m_cachedEyeInfo[xr::StereoView::Left] =
                ovr_GetRenderDesc(m_ovrSession, ovrEye_Left, m_cachedHmdInfo.DefaultEyeFov[ovrEye_Left]);
            m_cachedEyeInfo[xr::StereoView::Right] =
                ovr_GetRenderDesc(m_ovrSession, ovrEye_Right, m_cachedHmdInfo.DefaultEyeFov[ovrEye_Right]);

            for (uint32_t i = 0; i < xr::StereoView::Count; i++) {
                m_cachedEyeFov[i].angleDown = -atan(m_cachedEyeInfo[i].Fov.DownTan);
                m_cachedEyeFov[i].angleUp = atan(m_cachedEyeInfo[i].Fov.UpTan);
                m_cachedEyeFov[i].angleLeft = -atan(m_cachedEyeInfo[i].Fov.LeftTan);
                m_cachedEyeFov[i].angleRight = atan(m_cachedEyeInfo[i].Fov.RightTan);

                TraceLoggingWrite(g_traceProvider,
                                  "OVR_EyeRenderInfo",
                                  TLArg(i == xr::StereoView::Left ? "Left" : "Right", "Eye"),
                                  TLArg(xr::ToString(m_cachedEyeInfo[i].HmdToEyePose).c_str(), "EyePose"),
                                  TLArg(xr::ToString(m_cachedEyeFov[i]).c_str(), "Fov"));
            }
        }

        // Setup common parameters.
        // Virtual Desktop has a mode called "Stage Tracking" which requires us to use floor as the origin. For Oculus,
        // we use eye level for convenience.
        CHECK_OVRCMD(ovr_SetTrackingOriginType(
            m_ovrSession, !m_useOculusRuntime ? ovrTrackingOrigin_FloorLevel : ovrTrackingOrigin_EyeLevel));
    }

    void OpenXrRuntime::initializeFaceTrackingMmf() {
        *m_bodyStateFile.put() = OpenFileMapping(FILE_MAP_READ, false, L"VirtualDesktop.FaceState");
        if (!m_bodyStateFile) {
            TraceLoggingWrite(g_traceProvider, "VirtualDesktopBodyTracker_NotAvailable");
            return;
        }

        // To keep compatibility with older versions, we try mapping various versions of the structure.
        m_handJointsState = reinterpret_cast<BodyTracking::BodyStateV2*>(
            MapViewOfFile(m_bodyStateFile.get(), FILE_MAP_READ, 0, 0, sizeof(BodyTracking::BodyStateV2)));
        if (m_handJointsState) {
            m_faceState = reinterpret_cast<BodyTracking::BodyStateV1*>(m_handJointsState);
        } else {
            m_faceState = reinterpret_cast<BodyTracking::BodyStateV1*>(
                MapViewOfFile(m_bodyStateFile.get(), FILE_MAP_READ, 0, 0, sizeof(BodyTracking::BodyStateV1)));
        }
        if (!m_handJointsState) {
            TraceLoggingWrite(g_traceProvider, "VirtualDesktopBodyTracker_MappingError_BodyStateV2");
        }
        if (!m_faceState) {
            TraceLoggingWrite(g_traceProvider, "VirtualDesktopBodyTracker_MappingError_BodyStateV1");
        }
    }

} // namespace virtualdesktop_openxr
