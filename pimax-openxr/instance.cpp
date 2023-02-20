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
#include "store.h"
#include "utils.h"
#include "version.h"

namespace {

    using namespace pimax_openxr::log;

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

    // A mock implementation of GetModuleFileNameA() that fakes being the SteamVR process.
    decltype(GetModuleFileNameA)* g_original_GetModuleFileNameA = nullptr;
    DWORD WINAPI hooked_GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
        // We try to only intercept calls from the PVR client.
        HMODULE callerModule;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)_ReturnAddress(),
                               &callerModule)) {
            HMODULE libpvrModule;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, PVRCLIENT_DLL_NAME, &libpvrModule) &&
                callerModule != libpvrModule) {
                return g_original_GetModuleFileNameA(hModule, lpFilename, nSize);
            }
        }

        // The code in libpvrclient64.dll seems to fail if there is no folder.
        sprintf_s(lpFilename, nSize, "fake\\vrserver.exe");
        return (DWORD)strlen(lpFilename);
    }

    // A mock implementation of VerifyVersionInfoW() that always returns at least Windows 10 compatibility.
    decltype(VerifyVersionInfoW)* g_original_VerifyVersionInfoW = nullptr;
    BOOL WINAPI hooked_VerifyVersionInfoW(LPOSVERSIONINFOEXW lpVersionInformation,
                                          DWORD dwTypeMask,
                                          DWORDLONG dwlConditionMask) {
        // We try to only intercept calls from the PVR client.
        HMODULE callerModule;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)_ReturnAddress(),
                               &callerModule)) {
            HMODULE libpvrModule;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, PVRCLIENT_DLL_NAME, &libpvrModule) &&
                callerModule != libpvrModule) {
                return g_original_VerifyVersionInfoW(lpVersionInformation, dwTypeMask, dwlConditionMask);
            }
        }

        // PVR only seems to call this once and with a check against version 6.3 (Windows 8.1). Pretend the check
        // passes.
        return true;
    }

} // namespace

namespace pimax_openxr {

    using namespace pimax_openxr::utils;
    using namespace pimax_openxr::log;

    // CONFORMANCE: We do not handle multithreading properly. TODO: All functions must be thread-safe.

    const std::string RuntimePrettyName =
        fmt::format("PimaxXR - v{}.{}.{}", RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch);

    OpenXrRuntime::OpenXrRuntime() {
        if (getSetting("enable_telemetry").value_or(0)) {
            m_telemetry.initialize();
        }

        const auto runtimeVersion =
            xr::ToString(XR_MAKE_VERSION(RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch));
        TraceLoggingWrite(g_traceProvider, "PimaxXR", TLArg(runtimeVersion.c_str(), "Version"));
        m_telemetry.logVersion(runtimeVersion);

        // Initialize PVR.

        m_useFrameTimingOverride = getSetting("use_frame_timing_override").value_or(1);
        if (m_useFrameTimingOverride) {
            // Detour hack: during initialization of the PVR client, we pretend to be "vrserver" (the SteamVR core
            // process) in order to remove PVR frame timing constraints.
            DetourDllAttach(
                "kernel32.dll", "GetModuleFileNameA", hooked_GetModuleFileNameA, g_original_GetModuleFileNameA);

            // Detour hack: we always ensure compatibility with Windows 10 in order to make pvr_waitToBeginFrame()
            // behave as expected.
            // This was discovered with the PVR_Sample, which specifies <supportedOS
            // Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/> in its manifest.
            // Without this manifest entry, VerifyVersionInfoW always returns Windows 8 compatibility only.
            // https://social.msdn.microsoft.com/Forums/windows/en-US/298a1817-0af5-4efc-9663-db9a841a233b/verifyversioninfo-and-windows-10?forum=windowssdk
            DetourDllAttach(
                "kernel32.dll", "VerifyVersionInfoW", hooked_VerifyVersionInfoW, g_original_VerifyVersionInfoW);
        }

        CHECK_PVRCMD(pvr_initialise(&m_pvr));

        if (m_useFrameTimingOverride) {
            DetourDllDetach(
                "kernel32.dll", "GetModuleFileNameA", hooked_GetModuleFileNameA, g_original_GetModuleFileNameA);
        }

        std::string_view versionString(pvr_getVersionString(m_pvr));
        Log("PVR: %s\n", versionString.data());
        TraceLoggingWrite(g_traceProvider, "PVR_SDK", TLArg(versionString.data(), "VersionString"));

        // Identify the version of Pitool.
        const auto pitoolVersion = RegGetString(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{0D1DA8F2-89A7-4DAC-A9EF-B55E82CDA462}}_is1",
            "DisplayVersion");
        if (pitoolVersion) {
            Log("Pitool: %s\n", pitoolVersion->c_str());
            TraceLoggingWrite(g_traceProvider, "Pitool", TLArg(pitoolVersion->c_str(), "VersionString"));
        } else {
            Log("Could not detect Pitool version\n");
        }

        // We want to log a warning if HAGS is on.
        const auto hwSchMode =
            RegGetDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers", "HwSchMode");
        if (hwSchMode && hwSchMode.value() == 2) {
            TraceLoggingWrite(g_traceProvider, "HwSchMode", TLArg("On", "Mode"));
            Log("HAGS is on\n");
        }

        // Create the PVR session. Failing here is not considered fatal. We will try to initialize again during
        // xrGetSystem(). This is to allow the application to create the instance and query its properties even if
        // pi_server is not available.
        if (pvr_createSession(m_pvr, &m_pvrSession) == pvr_success) {
            if (pvr_getEyeHiddenAreaMesh(m_pvrSession, pvrEye_Left, nullptr, 0) == 0) {
                Log("Hidden area mesh is not enabled\n");
            }
        }

        QueryPerformanceFrequency(&m_qpcFrequency);

        // Calibrate the timestamp conversion.
        m_pvrTimeFromQpcTimeOffset = INFINITY;
        for (int i = 0; i < 100; i++) {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            const double qpcTime = (double)now.QuadPart / m_qpcFrequency.QuadPart;
            m_pvrTimeFromQpcTimeOffset = std::min(m_pvrTimeFromQpcTimeOffset, pvr_getTimeSeconds(m_pvr) - qpcTime);
        }
        TraceLoggingWrite(
            g_traceProvider, "ConvertTime", TLArg(m_pvrTimeFromQpcTimeOffset, "PvrTimeFromQpcTimeOffset"));

        // Watch for changes in the registry.
        try {
            m_registryWatcher = wil::make_registry_watcher(
                HKEY_LOCAL_MACHINE, xr::utf8_to_wide(RegPrefix).c_str(), true, [&](wil::RegistryChangeKind changeType) {
                    refreshSettings();
                });
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
        while (m_actionSets.size()) {
            CHECK_XRCMD(xrDestroyActionSet(*m_actionSets.begin()));
        }

        if (m_sessionCreated) {
            xrDestroySession((XrSession)1);
        }

        if (m_pvrSession) {
            // Workaround: the environment doesn't appear to be cleared when re-initializing PVR. Clear the one pointer
            // we care about.
            m_pvrSession->envh->pvr_dxgl_interface = nullptr;

            pvr_destroySession(m_pvrSession);
        }
        pvr_shutdown(m_pvr);

        if (m_useFrameTimingOverride) {
            DetourDllDetach(
                "kernel32.dll", "VerifyVersionInfoW", hooked_VerifyVersionInfoW, g_original_VerifyVersionInfoW);
        }
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
        m_telemetry.logApplicationInfo(createInfo->applicationInfo.applicationName,
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

        // Latch the state of parallel projection now. This is needed for computing the recommended swapchain sizes as
        // part of xrGetSystem(). Note: we may reset this later in case the system does not use canted displays.
        m_useParallelProjection = !pvr_getIntConfig(m_pvrSession, "steamvr_use_native_fov", 0);

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

        sprintf_s(instanceProperties->runtimeName, sizeof(instanceProperties->runtimeName), "PimaxXR (Unofficial)");
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
            buffer->time = pvrTimeToXrTime(m_sessionEventQueue.front().second);
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

        // This was originally protected with m_isVisibilityMaskSupported, however certain apps like FS 2020 have bugs
        // that rely on the extension being present.
        m_extensionsTable.push_back( // Hidden area mesh.
            {XR_KHR_VISIBILITY_MASK_EXTENSION_NAME, XR_KHR_visibility_mask_SPEC_VERSION});

        m_extensionsTable.push_back( // Mock display refresh rate.
            {XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME, XR_FB_display_refresh_rate_SPEC_VERSION});

        m_extensionsTable.push_back( // Hand tracking.
            {XR_EXT_HAND_TRACKING_EXTENSION_NAME, XR_EXT_hand_tracking_SPEC_VERSION});
        m_extensionsTable.push_back( // Hand tracking.
            {XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME, XR_EXT_hand_joints_motion_range_SPEC_VERSION});

        // FIXME: Add new extensions here.
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

    AppInsights* GetTelemetry() {
        if (g_instance) {
            return &g_instance->m_telemetry;
        }
        return nullptr;
    }

} // namespace pimax_openxr

extern "C" __declspec(dllexport) const char* WINAPI getVersionString() {
    return pimax_openxr::RuntimePrettyName.c_str();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        TraceLoggingRegister(pimax_openxr::log::g_traceProvider);
        DetourRestoreAfterWith();
        InitializeHighPrecisionTimer();
        DisableThreadLibraryCalls(hModule);

        if (!pimax_openxr::utils::RegGetDword(HKEY_LOCAL_MACHINE, pimax_openxr::RegPrefix, "disable_platform_sdk")
                 .value_or(0)) {
            // Initialize the platform SDK (requirement for the store).
            // Do this in a background thread to avoid interfering with app initialization/shutdown.
            CreateThread(
                NULL,
                0,
                [](void* param) -> DWORD {
                    // Increment our own DLL refcount to prevent unloading until finished.
                    // https://devblogs.microsoft.com/oldnewthing/20131105-00/?p=2733
                    HMODULE self;
                    GetModuleHandleExA(
                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(&DllMain), &self);

                    pimax_openxr::store::storeAsyncInit();

                    // Allow the DLL to be unloaded now.
                    FreeLibraryAndExitThread(self, 0);
                    return 0;
                },
                nullptr,
                0,
                0);
        }

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
