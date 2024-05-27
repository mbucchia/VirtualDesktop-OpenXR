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
#include "trackers.h"
#include "utils.h"
#include "version.h"
#include "commit.h"

namespace {
    wil::unique_handle g_fakeHmdConnectedEvent;
} // namespace

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::utils;
    using namespace virtualdesktop_openxr::log;

    const std::string RuntimePrettyName = fmt::format(RUNTIME_PRETTY_NAME " - v{}.{}.{} ({})",
                                                      RuntimeVersionMajor,
                                                      RuntimeVersionMinor,
                                                      RuntimeVersionPatch,
                                                      RuntimeCommitHash);

    XrResult XRAPI_CALL xrRequestBodyTrackingFidelityMETA(XrBodyTrackerFB bodyTracker,
                                                          const XrBodyTrackingFidelityMETA fidelity);

    OpenXrRuntime::OpenXrRuntime() {
        const auto runtimeVersion =
            xr::ToString(XR_MAKE_VERSION(RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch));
        TraceLoggingWrite(g_traceProvider, "VirtualDesktopOpenXR", TLArg(runtimeVersion.c_str(), "Version"));

        m_useApplicationDeviceForSubmission = getSetting("quirk_use_application_device_for_submission").value_or(false);

        // Latch the disabled trackers now.
        for (uint32_t i = 0; i < std::size(TrackerRoles); i++) {
            m_isTrackerDisabled[i] = getSetting(TrackerRoles[i].role + "_tracker_disabled").value_or(false);
        }

        // Watch for changes in the registry.
        try {
            wil::unique_hkey keyToWatch;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              xr::utf8_to_wide(RegPrefix).c_str(),
                              0,
                              KEY_WOW64_64KEY | KEY_READ,
                              keyToWatch.put()) == ERROR_SUCCESS) {
                m_registryWatcher = wil::make_registry_watcher(
                    std::move(keyToWatch), true, [&](wil::RegistryChangeKind changeType) { refreshSettings(); });
            }
        } catch (std::exception&) {
            // Ignore errors that can happen with UWP applications not able to write to the registry.
        }

        QueryPerformanceFrequency(&m_qpcFrequency);

        initializeExtensionsTable();
    }

    OpenXrRuntime::~OpenXrRuntime() {
        // Destroy actionset and actions (tied to the instance).
        for (auto action : m_actionsForCleanup) {
            Action* xrAction = (Action*)action;
            delete xrAction;
        }
        for (auto actionSet : m_actionSets) {
            ActionSet* xrActionSet = (ActionSet*)actionSet;
            delete xrActionSet;
        }

        if (m_sessionCreated) {
            // TODO: Ideally we do not invoke OpenXR public APIs to avoid confusing event tracing and possible
            // deadlocks.
            xrDestroySession((XrSession)1);
        }

        if (m_bodyState) {
            UnmapViewOfFile(m_bodyState);
        }

        if (m_ovrSession) {
            ovr_Destroy(m_ovrSession);
        }
        ovr_Shutdown();
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProcAddr
    XrResult OpenXrRuntime::xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
        TraceLoggingWrite(g_traceProvider,
                          "xrGetInstanceProcAddr",
                          TLXArg(instance, "Instance"),
                          TLArg(name, "Name"),
                          TLPArg(function, "Function"));

        XrResult result = XR_ERROR_FUNCTION_UNSUPPORTED;

        // XR_META_body_tracking_fidelity is not in the SDK yet and requires special handling.
        const std::string_view apiName(name);
        if (has_XR_META_body_tracking_fidelity && apiName == "xrRequestBodyTrackingFidelityMETA") {
            *function = reinterpret_cast<PFN_xrVoidFunction>(virtualdesktop_openxr::xrRequestBodyTrackingFidelityMETA);
            result = XR_SUCCESS;
        } else {
            result = OpenXrApi::xrGetInstanceProcAddr(instance, name, function);
        }

        TraceLoggingWrite(
            g_traceProvider, "xrGetInstanceProcAddr", TLPArg(function ? *function : nullptr, "RuntimeFunction"));

        return result;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateInstanceExtensionProperties
    XrResult OpenXrRuntime::xrEnumerateInstanceExtensionProperties(const char* layerName,
                                                                   uint32_t propertyCapacityInput,
                                                                   uint32_t* propertyCountOutput,
                                                                   XrExtensionProperties* properties) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateInstanceExtensionProperties",
                          TLArg(layerName, "LayerName"),
                          TLArg(propertyCapacityInput, "PropertyCapacityInput"));

        if (propertyCapacityInput && propertyCapacityInput < m_extensionsTable.size()) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *propertyCountOutput = (uint32_t)m_extensionsTable.size();
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateInstanceExtensionProperties",
                          TLArg(*propertyCountOutput, "PropertyCountOutput"));

        if (propertyCapacityInput && properties) {
            for (uint32_t i = 0; i < *propertyCountOutput; i++) {
                if (properties[i].type != XR_TYPE_EXTENSION_PROPERTIES) {
                    return XR_ERROR_VALIDATION_FAILURE;
                }

                sprintf_s(properties[i].extensionName,
                          sizeof(properties[0].extensionName),
                          "%s",
                          m_extensionsTable[i].extensionName);
                properties[i].extensionVersion = m_extensionsTable[i].extensionVersion;
                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateInstanceExtensionProperties",
                                  TLArg(properties[i].extensionName, "ExtensionName"),
                                  TLArg(properties[i].extensionVersion, "ExtensionVersion"));
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateInstance
    XrResult OpenXrRuntime::xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) {
        if (createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateInstance",
                          TLArg(xr::ToString(createInfo->applicationInfo.apiVersion).c_str(), "ApiVersion"),
                          TLArg(createInfo->applicationInfo.applicationName, "ApplicationName"),
                          TLArg(createInfo->applicationInfo.applicationVersion, "ApplicationVersion"),
                          TLArg(createInfo->applicationInfo.engineName, "EngineName"),
                          TLArg(createInfo->applicationInfo.engineVersion, "EngineVersion"),
                          TLArg(createInfo->createFlags, "CreateFlags"));

        // We only support one concurrent instance.
        if (m_instanceCreated) {
            return XR_ERROR_LIMIT_REACHED;
        }

        if (XR_VERSION_MAJOR(createInfo->applicationInfo.apiVersion) != XR_VERSION_1_0) {
            return XR_ERROR_API_VERSION_UNSUPPORTED;
        }

        {
            char path[_MAX_PATH];
            GetModuleFileNameA(nullptr, path, sizeof(path));
            std::filesystem::path fullPath(path);
            m_exeName = fullPath.filename().string();
        }
        m_applicationName = createInfo->applicationInfo.applicationName;

        Log("Application: %s (%s); Engine: %s\n",
            m_applicationName.c_str(),
            m_exeName.c_str(),
            createInfo->applicationInfo.engineName);

        for (uint32_t i = 0; i < createInfo->enabledApiLayerCount; i++) {
            TraceLoggingWrite(
                g_traceProvider, "xrCreateInstance", TLArg(createInfo->enabledApiLayerNames[i], "ApiLayerName"));
            Log("Requested API layer: %s\n", createInfo->enabledApiLayerNames[i]);
        }

        for (uint32_t i = 0; i < createInfo->enabledExtensionCount; i++) {
            const std::string_view extensionName(createInfo->enabledExtensionNames[i]);

            TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLArg(extensionName.data(), "ExtensionName"));
            Log("Requested extension: %s\n", extensionName.data());

            if (std::find_if(
                    m_extensionsTable.cbegin(), m_extensionsTable.cend(), [&extensionName](const Extension& extension) {
                        return extension.extensionName == extensionName;
                    }) == m_extensionsTable.cend()) {
                return XR_ERROR_EXTENSION_NOT_PRESENT;
            }

            registerInstanceExtension(std::string(extensionName));
        }

        // FIXME: Put application quirks below.

        HMODULE ovrPlugin;
        m_isOculusXrPlugin =
            m_applicationName.find("Oculus VR Plugin") == 0 ||
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "OVRPlugin.dll", &ovrPlugin);
        if (m_isOculusXrPlugin) {
            // For some reason, certain applications built with the Oculus plugin will attempt to initialize LibOVR.
            // However certain applications like Ghosts of Tabor are "delicate" and do no like our DLL inject, so
            // instead we perform our own (simpler) style.
            const bool useOwnInjectionMethod =
#ifndef STANDALONE_RUNTIME
                startsWith(m_exeName, "GhostsOfTabor") || startsWith(m_applicationName, "GhostsOfTabor") ||
                getSetting("quirk_force_own_injection").value_or(false);
#else
                true;
#endif
            if (!useOwnInjectionMethod) {
                Log("Using Virtual Desktop ovr_Detect() injection path\n");

                // We make sure that ovr_Detect() succeeds by injecting Virtual Desktop.
                std::filesystem::path path(RegGetString(HKEY_LOCAL_MACHINE,
                                                        "SOFTWARE\\Virtual Desktop, Inc.\\Virtual Desktop Streamer",
                                                        "Path")
                                               .value_or(L""));
                if (!path.empty()) {
                    path = path /
#ifdef _WIN64
                           L"VirtualDesktop.Injector64.dll";
#else
                           L"VirtualDesktop.Injector32.dll";
#endif

                    LoadLibraryW(path.c_str());
                }
            } else {
                Log("Using VDXR ovr_Detect() injection path\n");

                // We create a fake event to make ovr_Detect() succeed.
                if (!g_fakeHmdConnectedEvent) {
                    *g_fakeHmdConnectedEvent.put() = CreateEventW(nullptr, true, true, nullptr);
                }
            }
        }

        m_isConformanceTest = m_applicationName == "conformance test";
        m_isOpenComposite = startsWith(m_applicationName, "OpenComposite_");
        if (m_isOpenComposite) {
            Log("Detected OpenComposite - OpenComposite is not supported, use at your own risks!\n");
        }

        if ((startsWith(m_exeName, "Contractors_") && endsWith(m_exeName, "-Win64-Shipping.exe"))) {
            m_controllerGripOffset.position.z = -0.1f;
            m_quirkedControllerPoses = true;
        }

        m_forceSlowpathSwapchains = getSetting("quirk_force_slowpath_swapchains").value_or(false);
        m_allowVrs = getSetting("allow_vrs").value_or(false);

        m_supersamplingFactor = getSetting("supersampling").value_or(100) / 100.f;
        m_upscalingMultiplier = getSetting("upscaling").value_or(100) / 100.f;

        TraceLoggingWrite(g_traceProvider,
                          "VDXR_Config",
                          TLArg(m_supersamplingFactor, "SupersamplingFactor"),
                          TLArg(m_upscalingMultiplier, "UpscalingMultiplier"));

        // Do this late, since it might rely on extensions being registered.
        initializeRemappingTables();

        m_instanceCreated = true;
        *instance = (XrInstance)1;

        TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLXArg(*instance, "Instance"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyInstance
    XrResult OpenXrRuntime::xrDestroyInstance(XrInstance instance) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyInstance", TLXArg(instance, "Instance"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // The caller will destroy this class next, which will take care of all the cleanup.

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProperties
    XrResult OpenXrRuntime::xrGetInstanceProperties(XrInstance instance,
                                                    XrInstanceProperties* instanceProperties,
                                                    void* returnAddress) {
        if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties", TLXArg(instance, "Instance"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // The OculusXR Plugin only loads successfully when the returned OpenXR runtime name is "Oculus". We fake that
        // if the caller is the OculusXR Plugin, but we return the real runtime name otherwise.
        // Some games (like 7th Guest VR) do not play well when forcing the runtime name, so we exclude them.
        const bool needOculusXrPluginWorkaround = m_isOculusXrPlugin && m_exeName != "The7thGuestVR-Win64-Shipping.exe";
        if (!needOculusXrPluginWorkaround) {
#ifndef STANDALONE_RUNTIME
            sprintf_s(instanceProperties->runtimeName, sizeof(instanceProperties->runtimeName), "VirtualDesktopXR");
#else
            sprintf_s(instanceProperties->runtimeName, sizeof(instanceProperties->runtimeName), RUNTIME_PRETTY_NAME);
#endif
        } else {
            sprintf_s(instanceProperties->runtimeName, sizeof(instanceProperties->runtimeName), "Oculus");
        }

        // This cannot be all 0.
        instanceProperties->runtimeVersion = XR_MAKE_VERSION(
            RuntimeVersionMajor,
            RuntimeVersionMinor,
            (RuntimeVersionMajor == 0 && RuntimeVersionMinor == 0 && RuntimeVersionPatch == 0) ? 1
                                                                                               : RuntimeVersionPatch);

        TraceLoggingWrite(g_traceProvider,
                          "xrGetInstanceProperties",
                          TLArg(instanceProperties->runtimeName, "RuntimeName"),
                          TLArg(xr::ToString(instanceProperties->runtimeVersion).c_str(), "RuntimeVersion"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrPollEvent
    XrResult OpenXrRuntime::xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
        TraceLoggingWrite(g_traceProvider, "xrPollEvent", TLXArg(instance, "Instance"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // Generate session events.
        updateSessionState();
        if (!m_sessionEventQueue.empty()) {
            XrEventDataSessionStateChanged* const buffer = reinterpret_cast<XrEventDataSessionStateChanged*>(eventData);
            buffer->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
            buffer->next = nullptr;
            buffer->session = (XrSession)1;
            buffer->state = m_sessionEventQueue.front().first;
            buffer->time = ovrTimeToXrTime(m_sessionEventQueue.front().second);
            m_sessionEventQueue.pop_front();

            TraceLoggingWrite(g_traceProvider,
                              "xrPollEvent",
                              TLArg("SessionStateChanged", "Type"),
                              TLXArg(buffer->session, "Session"),
                              TLArg(xr::ToCString(buffer->state), "State"),
                              TLArg(buffer->time, "Time"));

            return XR_SUCCESS;
        }

        if (m_currentInteractionProfileDirty) {
            XrEventDataInteractionProfileChanged* const buffer =
                reinterpret_cast<XrEventDataInteractionProfileChanged*>(eventData);
            buffer->type = XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED;
            buffer->next = nullptr;
            buffer->session = (XrSession)1;

            TraceLoggingWrite(g_traceProvider,
                              "xrPollEvent",
                              TLArg("InteractionProfileChanged", "Type"),
                              TLXArg(buffer->session, "Session"));

            m_currentInteractionProfileDirty = false;

            return XR_SUCCESS;
        }

        if (m_visibilityMaskDirty) {
            XrEventDataVisibilityMaskChangedKHR* const buffer =
                reinterpret_cast<XrEventDataVisibilityMaskChangedKHR*>(eventData);
            buffer->type = XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR;
            buffer->next = nullptr;
            buffer->session = (XrSession)1;
            buffer->viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            buffer->viewIndex =
                m_visibilityMaskDirty == xr::StereoView::Count ? xr::StereoView::Left : xr::StereoView::Right;

            TraceLoggingWrite(g_traceProvider,
                              "VisibilityMaskChanged",
                              TLPArg(buffer->session, "Session"),
                              TLArg(xr::ToCString(buffer->viewConfigurationType), "ViewConfigurationType"),
                              TLArg(buffer->viewIndex, "ViewIndex"));

            m_visibilityMaskDirty--;

            return XR_SUCCESS;
        }

        return XR_EVENT_UNAVAILABLE;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrResultToString
    XrResult OpenXrRuntime::xrResultToString(XrInstance instance,
                                             XrResult value,
                                             char buffer[XR_MAX_RESULT_STRING_SIZE]) {
#define EMIT_RESULT_STRING(name, value)                                                                                \
    case name:                                                                                                         \
        sprintf_s(buffer, XR_MAX_RESULT_STRING_SIZE, "%s", #name);                                                     \
        break;

        switch (value) {
            XR_LIST_ENUM_XrResult(EMIT_RESULT_STRING);

        default:
            if (XR_FAILED(value)) {
                sprintf_s(buffer, XR_MAX_RESULT_STRING_SIZE, "XR_UNKNOWN_FAILURE_%d", (int)value);
            } else {
                sprintf_s(buffer, XR_MAX_RESULT_STRING_SIZE, "XR_UNKNOWN_SUCCESS_%d", (int)value);
            }
        }

#undef EMIT_RESULT_STRING

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrStructureTypeToString
    XrResult OpenXrRuntime::xrStructureTypeToString(XrInstance instance,
                                                    XrStructureType value,
                                                    char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
#define EMIT_STRUCTURE_TYPE_STRING(name, value)                                                                        \
    case name:                                                                                                         \
        sprintf_s(buffer, XR_MAX_STRUCTURE_NAME_SIZE, "%s", #name);                                                    \
        break;

        switch ((int)value) {
            XR_LIST_ENUM_XrStructureType(EMIT_STRUCTURE_TYPE_STRING);

        default:
            sprintf_s(buffer, XR_MAX_STRUCTURE_NAME_SIZE, "XR_UNKNOWN_STRUCTURE_TYPE_%d", (int)value);
        }

#undef EMIT_STRUCTURE_TYPE_STRING

        return XR_SUCCESS;
    }

    void OpenXrRuntime::initializeExtensionsTable() {
        m_extensionsTable.push_back( // Direct3D 11 support.
            {XR_KHR_D3D11_ENABLE_EXTENSION_NAME, XR_KHR_D3D11_enable_SPEC_VERSION});
        m_extensionsTable.push_back( // Direct3D 12 support.
            {XR_KHR_D3D12_ENABLE_EXTENSION_NAME, XR_KHR_D3D12_enable_SPEC_VERSION});
        m_extensionsTable.push_back( // Vulkan support.
            {XR_KHR_VULKAN_ENABLE_EXTENSION_NAME, XR_KHR_vulkan_enable_SPEC_VERSION});
        m_extensionsTable.push_back( // Vulkan support.
            {XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME, XR_KHR_vulkan_enable2_SPEC_VERSION});
        m_extensionsTable.push_back( // OpenGL support.
            {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, XR_KHR_opengl_enable_SPEC_VERSION});

        m_extensionsTable.push_back( // Depth buffer submission.
            {XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, XR_KHR_composition_layer_depth_SPEC_VERSION});

        m_extensionsTable.push_back( // Qpc timestamp conversion.
            {XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME,
             XR_KHR_win32_convert_performance_counter_time_SPEC_VERSION});

        m_extensionsTable.push_back( // For UWP apps.
            {XR_EXT_WIN32_APPCONTAINER_COMPATIBLE_EXTENSION_NAME, XR_EXT_win32_appcontainer_compatible_SPEC_VERSION});

        m_extensionsTable.push_back( // Hidden area mesh.
            {XR_KHR_VISIBILITY_MASK_EXTENSION_NAME, XR_KHR_visibility_mask_SPEC_VERSION});

        m_extensionsTable.push_back( // Mock display refresh rate.
            {XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME, XR_FB_display_refresh_rate_SPEC_VERSION});

        m_extensionsTable.push_back( // Palm pose.
            {XR_EXT_PALM_POSE_EXTENSION_NAME, XR_EXT_palm_pose_SPEC_VERSION});

        m_extensionsTable.push_back( // Audio GUID.
            {XR_OCULUS_AUDIO_DEVICE_GUID_EXTENSION_NAME, XR_OCULUS_audio_device_guid_SPEC_VERSION});

#ifdef HAS_CYLINDER_LAYERS
        m_extensionsTable.push_back( // Cylinder layers.
            {XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME, XR_KHR_composition_layer_cylinder_SPEC_VERSION});
#endif

#ifdef HAS_CUBE_LAYERS
        m_extensionsTable.push_back( // Cube layers.
            {XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME, XR_KHR_composition_layer_cube_SPEC_VERSION});
#endif

#ifdef HAS_HAND_JOINTS_TRACKING
        m_extensionsTable.push_back( // Hand tracking.
            {XR_EXT_HAND_TRACKING_EXTENSION_NAME, XR_EXT_hand_tracking_SPEC_VERSION});
        m_extensionsTable.push_back( // Hand tracking.
            {XR_EXT_HAND_TRACKING_DATA_SOURCE_EXTENSION_NAME, XR_EXT_hand_tracking_data_source_SPEC_VERSION});
#endif
#ifdef HAS_HAND_TRACKING_AIM
        m_extensionsTable.push_back( // Hand tracking.
            {XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME, XR_FB_hand_tracking_aim_SPEC_VERSION});
#endif

#ifdef HAS_EYE_TRACKING
        m_extensionsTable.push_back( // Eye tracking.
            {XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, XR_EXT_eye_gaze_interaction_SPEC_VERSION});
#endif

#ifdef HAS_FACIAL_EYE_TRACKING
        m_extensionsTable.push_back( // Face, body & social eye tracking.
            {XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME, XR_FB_eye_tracking_social_SPEC_VERSION});
#endif
#ifdef HAS_FACIAL_EXPRESSION_TRACKING
        m_extensionsTable.push_back( // Face, body & & social eye tracking.
            {XR_FB_FACE_TRACKING_EXTENSION_NAME, XR_FB_face_tracking_SPEC_VERSION});
        m_extensionsTable.push_back( // Face, body & & social eye tracking.
            {XR_FB_FACE_TRACKING2_EXTENSION_NAME, XR_FB_face_tracking2_SPEC_VERSION});
#endif
#ifdef HAS_BODY_TRACKING
        m_extensionsTable.push_back( // Face, body & social eye tracking.
            {XR_FB_BODY_TRACKING_EXTENSION_NAME, XR_FB_body_tracking_SPEC_VERSION});
        m_extensionsTable.push_back( // Face, body & social eye tracking.
            {XR_META_BODY_TRACKING_FULL_BODY_EXTENSION_NAME, XR_META_body_tracking_full_body_SPEC_VERSION});
        m_extensionsTable.push_back( // Face, body & social eye tracking.
            {XR_META_BODY_TRACKING_FIDELITY_EXTENSION_NAME, XR_META_body_tracking_fidelity_SPEC_VERSION});

        m_extensionsTable.push_back( // Vive Tracker emulation.
            {XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME, XR_HTCX_vive_tracker_interaction_SPEC_VERSION});
#endif

#ifdef HAS_INVISIBLE_MODE
        m_extensionsTable.push_back( // Headless sessions.
            {XR_MND_HEADLESS_EXTENSION_NAME, XR_MND_headless_SPEC_VERSION});
#endif

        if (getSetting("enable_quad_views").value_or(false)) {
            m_extensionsTable.push_back( // Quad views.
                {XR_VARJO_QUAD_VIEWS_EXTENSION_NAME, XR_VARJO_quad_views_SPEC_VERSION});
            m_extensionsTable.push_back( // Foveated rendering with quad views.
                {XR_VARJO_FOVEATED_RENDERING_EXTENSION_NAME, XR_VARJO_foveated_rendering_SPEC_VERSION});
        }

        // To keep Oculus OpenXR plugin happy.
        m_extensionsTable.push_back({XR_EXT_UUID_EXTENSION_NAME, XR_EXT_uuid_SPEC_VERSION});
        m_extensionsTable.push_back({XR_META_HEADSET_ID_EXTENSION_NAME, XR_META_headset_id_SPEC_VERSION});

        // FIXME: Add new extensions here.
    }

    XrTime OpenXrRuntime::ovrTimeToXrTime(double ovrTime) const {
        return (XrTime)(ovrTime * 1e9);
    }

    double OpenXrRuntime::xrTimeToOvrTime(XrTime xrTime) const {
        return xrTime / 1e9;
    }

    std::optional<int> OpenXrRuntime::getSetting(const std::string& value) const {
        return RegGetDword(HKEY_LOCAL_MACHINE, RegPrefix, value);
    }

    XrResult XRAPI_CALL xrRequestBodyTrackingFidelityMETA(XrBodyTrackerFB bodyTracker,
                                                          const XrBodyTrackingFidelityMETA fidelity) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "xrRequestBodyTrackingFidelityMETA");

        XrResult result;
        try {
            result =
                dynamic_cast<OpenXrRuntime*>(GetInstance())->xrRequestBodyTrackingFidelityMETA(bodyTracker, fidelity);
        } catch (std::exception& exc) {
            TraceLoggingWriteTagged(local, "xrRequestBodyTrackingFidelityMETA_Error", TLArg(exc.what(), "Error"));
            ErrorLog("xrRequestBodyTrackingFidelityMETA: %s\n", exc.what());
            result = XR_ERROR_RUNTIME_FAILURE;
        }

        TraceLoggingWriteStop(local, "xrRequestBodyTrackingFidelityMETA", TLArg(xr::ToCString(result), "Result"));
        if (XR_FAILED(result)) {
            ErrorLog("xrRequestBodyTrackingFidelityMETA failed with %s\n", xr::ToCString(result));
        }

        return result;
    }

    // Singleton class instance.
    std::unique_ptr<OpenXrRuntime> g_instance = nullptr;

    OpenXrApi* GetInstance() {
        if (!g_instance) {
            g_instance = std::make_unique<OpenXrRuntime>();
        }
        return g_instance.get();
    }

    void ResetInstance() {
        g_instance.reset();
    }

} // namespace virtualdesktop_openxr

namespace {

    // This hook will cause the LibOVR loader's ovr_Detect() to succeed.
    DEFINE_DETOUR_FUNCTION(HANDLE, OpenEventW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName) {
        if (g_fakeHmdConnectedEvent && std::wstring(lpName) == L"OculusHMDConnected") {
            HANDLE handle = nullptr;
            DuplicateHandle(GetCurrentProcess(),
                            g_fakeHmdConnectedEvent.get(),
                            GetCurrentProcess(),
                            &handle,
                            dwDesiredAccess,
                            bInheritHandle,
                            0);
            return handle;
        }
        return original_OpenEventW(dwDesiredAccess, bInheritHandle, lpName);
    }

} // namespace

extern "C" __declspec(dllexport) const char* WINAPI getVersionString() {
    return virtualdesktop_openxr::RuntimePrettyName.c_str();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    using namespace virtualdesktop_openxr::utils;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();
        TraceLoggingRegister(virtualdesktop_openxr::log::g_traceProvider);
        virtualdesktop_openxr::utils::InitializeHighPrecisionTimer();
        DetourDllAttach("Kernel32", "OpenEventW", hooked_OpenEventW, original_OpenEventW);
        break;

    case DLL_PROCESS_DETACH:
        DetourDllDetach("Kernel32", "OpenEventW", hooked_OpenEventW, original_OpenEventW);
        TraceLoggingUnregister(virtualdesktop_openxr::log::g_traceProvider);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
