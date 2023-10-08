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
#include "version.h"

// From our OVR_CAPIShim.c fork.
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_InitializeWithPathOverride(const ovrInitParams* inputParams, const wchar_t* overrideLibraryPath);

namespace {

    using namespace virtualdesktop_openxr::log;

    extern "C" NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution,
                                                            BOOLEAN SetResolution,
                                                            PULONG CurrentResolution);
    extern "C" NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution,
                                                              PULONG MaximumResolution,
                                                              PULONG CurrentResolution);

    void InitializeHighPrecisionTimer() {
        // https://stackoverflow.com/questions/3141556/how-to-setup-timer-resolution-to-0-5-ms
        ULONG min, max, current;
        NtQueryTimerResolution(&min, &max, &current);
        TraceLoggingWrite(
            g_traceProvider, "NtQueryTimerResolution", TLArg(min, "Min"), TLArg(max, "Max"), TLArg(current, "Current"));

        ULONG currentRes;
        NtSetTimerResolution(max, TRUE, &currentRes);

        // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation
        // Enable HighQoS to achieve maximum performance, and turn off power saving.
        {
            PROCESS_POWER_THROTTLING_STATE PowerThrottling{};
            PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
            PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
            PowerThrottling.StateMask = 0;

            SetProcessInformation(
                GetCurrentProcess(), ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling));
        }

        // https://forums.oculusvr.com/t5/General/SteamVR-has-fixed-the-problems-with-Windows-11/td-p/956413
        // Always honor Timer Resolution Requests. This is to ensure that the timer resolution set-up above sticks
        // through transitions of the main window (eg: minimization).
        {
            // This setting was introduced in Windows 11 and the definition is not available in older headers.
#ifndef PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
            const auto PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION = 0x4U;
#endif

            PROCESS_POWER_THROTTLING_STATE PowerThrottling{};
            PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
            PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
            PowerThrottling.StateMask = 0;

            SetProcessInformation(
                GetCurrentProcess(), ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling));
        }
    }

    // https://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c
    bool IsServiceRunning(const std::wstring_view& name) {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        bool found = false;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (Process32First(snapshot, &entry) == TRUE) {
            while (Process32Next(snapshot, &entry) == TRUE) {
                if (std::wstring_view(entry.szExeFile) == name) {
                    found = true;
                    break;
                }
            }
        }
        CloseHandle(snapshot);

        return found;
    }

} // namespace

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::utils;
    using namespace virtualdesktop_openxr::log;

    const std::string RuntimePrettyName =
        fmt::format("VirtualDesktopXR - v{}.{}.{}", RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch);

    OpenXrRuntime::OpenXrRuntime() {
        const auto runtimeVersion =
            xr::ToString(XR_MAKE_VERSION(RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch));
        TraceLoggingWrite(g_traceProvider, "VirtualDesktopOpenXR", TLArg(runtimeVersion.c_str(), "Version"));

        // Note: this is not compatible with async_submission=1!
        m_useApplicationDeviceForSubmission = getSetting("quirk_use_application_device_for_submission").value_or(false);

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

        initializeExtensionsTable();
        initializeRemappingTables();
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

        if (m_faceState) {
            UnmapViewOfFile(m_faceState);
        }

        if (m_ovrSession) {
            ovr_Destroy(m_ovrSession);
        }
        ovr_Shutdown();
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProcAddr
    XrResult OpenXrRuntime::xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
        TraceLoggingWrite(g_traceProvider, "xrGetInstanceProcAddr", TLXArg(instance, "Instance"), TLArg(name, "Name"));

        const auto result = OpenXrApi::xrGetInstanceProcAddr(instance, name, function);

        TraceLoggingWrite(g_traceProvider, "xrGetInstanceProcAddr", TLPArg(function, "Function"));

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

        Log("Application: %s; Engine: %s\n",
            createInfo->applicationInfo.applicationName,
            createInfo->applicationInfo.engineName);

        if (XR_VERSION_MAJOR(createInfo->applicationInfo.apiVersion) != XR_VERSION_1_0) {
            return XR_ERROR_API_VERSION_UNSUPPORTED;
        }

        m_applicationName = createInfo->applicationInfo.applicationName;

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
    XrResult OpenXrRuntime::xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) {
        if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties", TLXArg(instance, "Instance"));

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        sprintf_s(instanceProperties->runtimeName, sizeof(instanceProperties->runtimeName), "VirtualDesktopXR");
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

        m_extensionsTable.push_back( // Hidden area mesh.
            {XR_KHR_VISIBILITY_MASK_EXTENSION_NAME, XR_KHR_visibility_mask_SPEC_VERSION});

        m_extensionsTable.push_back( // Mock display refresh rate.
            {XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME, XR_FB_display_refresh_rate_SPEC_VERSION});

        m_extensionsTable.push_back( // Eye tracking.
            {XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, XR_EXT_eye_gaze_interaction_SPEC_VERSION});

        // To keep Oculus OpenXR plugin happy.
        m_extensionsTable.push_back({XR_EXT_UUID_EXTENSION_NAME, XR_EXT_uuid_SPEC_VERSION});
        m_extensionsTable.push_back({XR_META_HEADSET_ID_EXTENSION_NAME, XR_META_headset_id_SPEC_VERSION});

        // FIXME: Add new extensions here.
    }

    bool OpenXrRuntime::InitializeOVR() {
        std::wstring overridePath;
        if (!getSetting("use_oculus_runtime").value_or(false)) {
            if (!IsServiceRunning(L"VirtualDesktop.Server.exe")) {
                return false;
            }

            // Locate Virtual Desktop's LibOVR.
            std::filesystem::path path(
                RegGetString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Virtual Desktop, Inc.\\Virtual Desktop Streamer", "Path")
                    .value());
            path = path / L"VirtualDesktop.";

            overridePath = path.wstring();
        }

        // Initialize OVR.
        ovrResult result;
        ovrInitParams initParams{};
        initParams.Flags = ovrInit_RequestVersion | ovrInit_FocusAware;
        initParams.RequestedMinorVersion = OVR_MINOR_VERSION;
        result = ovr_InitializeWithPathOverride(&initParams, overridePath.empty() ? nullptr : overridePath.c_str());
        if (result == ovrError_ServiceConnection) {
            return false;
        }
        CHECK_OVRCMD(result);

        std::string_view versionString(ovr_GetVersionString());
        Log("OVR: %s\n", versionString.data());
        TraceLoggingWrite(g_traceProvider, "OVR_SDK", TLArg(versionString.data(), "VersionString"));

        result = ovr_Create(&m_ovrSession, reinterpret_cast<ovrGraphicsLuid*>(&m_adapterLuid));
        if (result == ovrError_NoHmd) {
            return false;
        }
        CHECK_OVRCMD(result);

        QueryPerformanceFrequency(&m_qpcFrequency);

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

        return true;
    }

    std::optional<int> OpenXrRuntime::getSetting(const std::string& value) const {
        return RegGetDword(HKEY_LOCAL_MACHINE, RegPrefix, value);
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

extern "C" __declspec(dllexport) const char* WINAPI getVersionString() {
    return virtualdesktop_openxr::RuntimePrettyName.c_str();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        TraceLoggingRegister(virtualdesktop_openxr::log::g_traceProvider);
        InitializeHighPrecisionTimer();
        break;

    case DLL_PROCESS_DETACH:
        TraceLoggingUnregister(virtualdesktop_openxr::log::g_traceProvider);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
