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

#pragma once

#include "pch.h"

#include "runtime.h"
#include "log.h"

#define CHECK_PVRCMD(cmd) xr::detail::_CheckPVRResult(cmd, #cmd, FILE_AND_LINE)

namespace xr {
    inline std::string ToString(XrVersion version) {
        return fmt::format("{}.{}.{}", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));
    }

    inline std::string ToString(pvrPosef pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.Position.x,
                           pose.Position.y,
                           pose.Position.z,
                           pose.Orientation.x,
                           pose.Orientation.y,
                           pose.Orientation.z,
                           pose.Orientation.w);
    }

    inline std::string ToString(XrPosef pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.position.x,
                           pose.position.y,
                           pose.position.z,
                           pose.orientation.x,
                           pose.orientation.y,
                           pose.orientation.z,
                           pose.orientation.w);
    }

    inline std::string ToString(XrFovf fov) {
        return fmt::format(
            "(l:{:.3f}, r:{:.3f}, u:{:.3f}, d:{:.3f})", fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown);
    }

    inline std::string ToString(XrRect2Di rect) {
        return fmt::format("x:{}, y:{} w:{} h:{}", rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
    }

    namespace detail {

        [[noreturn]] inline void _ThrowPVRResult(pvrResult pvr,
                                                 const char* originator = nullptr,
                                                 const char* sourceLocation = nullptr) {
            xr::detail::_Throw(xr::detail::_Fmt("pvrResult failure [%x]", pvr), originator, sourceLocation);
        }

        inline HRESULT _CheckPVRResult(pvrResult pvr,
                                       const char* originator = nullptr,
                                       const char* sourceLocation = nullptr) {
            if (pvr != pvr_success) {
                xr::detail::_ThrowPVRResult(pvr, originator, sourceLocation);
            }

            return pvr;
        }
    } // namespace detail

} // namespace xr

namespace {

    using namespace pimax_openxr;
    using namespace pimax_openxr::log;
    using namespace DirectX;
    using namespace xr::math;

    XrTime pvrTimeToXrTime(double pvrTime) {
        return (XrTime)(pvrTime * 1e9);
    }

    double xrTimeToPvrTime(XrTime xrTime) {
        return xrTime / 1e9;
    }

    XrPosef pvrPoseToXrPose(const pvrPosef& pvrPose) {
        XrPosef xrPose;
        xrPose.position.x = pvrPose.Position.x;
        xrPose.position.y = pvrPose.Position.y;
        xrPose.position.z = pvrPose.Position.z;
        xrPose.orientation.x = pvrPose.Orientation.x;
        xrPose.orientation.y = pvrPose.Orientation.y;
        xrPose.orientation.z = pvrPose.Orientation.z;
        xrPose.orientation.w = pvrPose.Orientation.w;

        return xrPose;
    }

    pvrPosef xrPoseToPvrPose(const XrPosef& xrPose) {
        pvrPosef pvrPose;
        pvrPose.Position.x = xrPose.position.x;
        pvrPose.Position.y = xrPose.position.y;
        pvrPose.Position.z = xrPose.position.z;
        pvrPose.Orientation.x = xrPose.orientation.x;
        pvrPose.Orientation.y = xrPose.orientation.y;
        pvrPose.Orientation.z = xrPose.orientation.z;
        pvrPose.Orientation.w = xrPose.orientation.w;

        return pvrPose;
    }

    pvrTextureFormat dxgiToPvrTextureFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return PVR_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return PVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return PVR_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return PVR_FORMAT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return PVR_FORMAT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return PVR_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return PVR_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_D16_UNORM:
            return PVR_FORMAT_D16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return PVR_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_D32_FLOAT:
            return PVR_FORMAT_D32_FLOAT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return PVR_FORMAT_D32_FLOAT_S8X24_UINT;
        case DXGI_FORMAT_BC1_UNORM:
            return PVR_FORMAT_BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return PVR_FORMAT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:
            return PVR_FORMAT_BC2_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return PVR_FORMAT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:
            return PVR_FORMAT_BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return PVR_FORMAT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC6H_UF16:
            return PVR_FORMAT_BC6H_UF16;
        case DXGI_FORMAT_BC6H_SF16:
            return PVR_FORMAT_BC6H_SF16;
        case DXGI_FORMAT_BC7_UNORM:
            return PVR_FORMAT_BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return PVR_FORMAT_BC7_UNORM_SRGB;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return PVR_FORMAT_R11G11B10_FLOAT;
        default:
            return PVR_FORMAT_UNKNOWN;
        }
    }

    bool isValidSwapchainRect(pvrTextureSwapChainDesc desc, XrRect2Di rect) {
        if (rect.offset.x < 0 || rect.offset.y < 0 || rect.extent.width <= 0 || rect.extent.height <= 0) {
            return false;
        }

        if (rect.offset.x + rect.extent.width > desc.Width || rect.offset.y + rect.extent.height > desc.Height) {
            return false;
        }

        return true;
    }

    class OpenXrRuntime : public OpenXrApi {
      public:
        OpenXrRuntime() {
            CHECK_PVRCMD(pvr_initialise(&m_pvr));

            std::string_view versionString(pvr_getVersionString(m_pvr));
            Log("PVR: %s\n", versionString.data());
            TraceLoggingWrite(g_traceProvider, "PVR_SDK", TLArg(versionString.data(), "VersionString"));

            QueryPerformanceFrequency(&m_qpcFrequency);

            // Calibrate the timestamp conversion.
            m_pvrTimeFromQpcTimeOffset = INFINITY;
            for (int i = 0; i < 100; i++) {
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                const double qpcTime = (double)now.QuadPart / m_qpcFrequency.QuadPart;
                m_pvrTimeFromQpcTimeOffset = min(m_pvrTimeFromQpcTimeOffset, pvr_getTimeSeconds(m_pvr) - qpcTime);
            }
            TraceLoggingWrite(
                g_traceProvider, "ConvertTime", TLArg(m_pvrTimeFromQpcTimeOffset, "PvrTimeFromQpcTimeOffset"));
        }

        virtual ~OpenXrRuntime() {
            if (m_sessionCreated) {
                xrDestroySession((XrSession)1);
            }

            if (m_pvrSession) {
                pvr_destroySession(m_pvrSession);
            }
            pvr_shutdown(m_pvr);
        }

        XrResult xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) override {
            TraceLoggingWrite(
                g_traceProvider, "xrGetInstanceProcAddr", TLPArg(instance, "Instance"), TLArg(name, "Name"));

            const std::string apiName(name);
            XrResult result = XR_SUCCESS;

            // TODO: This should be auto-generated by the dispatch layer, but today our generator only looks at core
            // spec.
            if (apiName == "xrGetD3D11GraphicsRequirementsKHR") {
                *function = reinterpret_cast<PFN_xrVoidFunction>(_xrGetD3D11GraphicsRequirementsKHR);
            } else if (apiName == "xrConvertWin32PerformanceCounterToTimeKHR") {
                *function = reinterpret_cast<PFN_xrVoidFunction>(_xrConvertWin32PerformanceCounterToTimeKHR);
            } else if (apiName == "xrConvertTimeToWin32PerformanceCounterKHR") {
                *function = reinterpret_cast<PFN_xrVoidFunction>(_xrConvertTimeToWin32PerformanceCounterKHR);
            } else if (apiName == "xrGetVisibilityMaskKHR") {
                *function = reinterpret_cast<PFN_xrVoidFunction>(_xrGetVisibilityMaskKHR);
            } else {
                result = OpenXrApi::xrGetInstanceProcAddr(instance, name, function);
            }

            TraceLoggingWrite(g_traceProvider, "xrGetInstanceProcAddr", TLPArg(function, "Function"));

            return result;
        }

        //
        // Instance management.
        //

        XrResult xrEnumerateInstanceExtensionProperties(const char* layerName,
                                                        uint32_t propertyCapacityInput,
                                                        uint32_t* propertyCountOutput,
                                                        XrExtensionProperties* properties) override {
            static const struct {
                const char* extensionName;
                uint32_t extensionVersion;
            } extensions[] = {
                // Direct3D 11 support.
                {XR_KHR_D3D11_ENABLE_EXTENSION_NAME, XR_KHR_D3D11_enable_SPEC_VERSION},

                // Qpc timestamp conversion.
                {XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME,
                 XR_KHR_win32_convert_performance_counter_time_SPEC_VERSION},

                // Hidden area mesh.
                {XR_KHR_VISIBILITY_MASK_EXTENSION_NAME, XR_KHR_visibility_mask_SPEC_VERSION},

                // FIXME: Add new extensions here.
            };

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateInstanceExtensionProperties",
                              TLArg(layerName, "LayerName"),
                              TLArg(propertyCapacityInput, "PropertyCapacityInput"));

            if (propertyCapacityInput && propertyCapacityInput < ARRAYSIZE(extensions)) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *propertyCountOutput = ARRAYSIZE(extensions);
            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateInstanceExtensionProperties",
                              TLArg(*propertyCountOutput, "PropertyCountOutput"));

            if (properties) {
                for (uint32_t i = 0; i < *propertyCountOutput; i++) {
                    if (properties[i].type != XR_TYPE_EXTENSION_PROPERTIES) {
                        return XR_ERROR_VALIDATION_FAILURE;
                    }

                    sprintf_s(properties[i].extensionName,
                              sizeof(properties[0].extensionName),
                              "%s",
                              extensions[i].extensionName);
                    properties[i].extensionVersion = extensions[i].extensionVersion;
                    TraceLoggingWrite(g_traceProvider,
                                      "xrEnumerateInstanceExtensionProperties",
                                      TLArg(properties[i].extensionName, "ExtensionName"),
                                      TLArg(properties[i].extensionVersion, "ExtensionVersion"));
                }
            }

            return XR_SUCCESS;
        }

        XrResult xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) override {
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

            for (uint32_t i = 0; i < createInfo->enabledApiLayerCount; i++) {
                TraceLoggingWrite(
                    g_traceProvider, "xrCreateInstance", TLArg(createInfo->enabledApiLayerNames[i], "ApiLayerName"));
                Log("Requested API layer: %s\n", createInfo->enabledApiLayerNames[i]);
            }

            for (uint32_t i = 0; i < createInfo->enabledExtensionCount; i++) {
                const std::string_view extensionName(createInfo->enabledExtensionNames[i]);

                TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLArg(extensionName.data(), "ExtensionName"));
                Log("Requested extension: %s\n", extensionName.data());

                // FIXME: Add new extension validation here.
                if (extensionName == XR_KHR_D3D11_ENABLE_EXTENSION_NAME) {
                    m_isD3D11Supported = true;
                } else if (extensionName == XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME ||
                           extensionName == XR_KHR_VISIBILITY_MASK_EXTENSION_NAME) {
                    // Do nothing.
                } else {
                    return XR_ERROR_EXTENSION_NOT_PRESENT;
                }
            }

            m_instanceCreated = true;
            *instance = (XrInstance)1;

            TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLPArg(*instance, "Instance"));

            return XR_SUCCESS;
        }

        XrResult xrDestroyInstance(XrInstance instance) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroyInstance", TLPArg(instance, "Instance"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // The caller will destroy this class next, which will take care of all the cleanup.

            return XR_SUCCESS;
        }

        XrResult xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) override {
            if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties", TLPArg(instance, "Instance"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            sprintf_s(instanceProperties->runtimeName, sizeof(instanceProperties->runtimeName), "Pimax (Unofficial)");
            // This cannot be all 0.
            instanceProperties->runtimeVersion =
                XR_MAKE_VERSION(RuntimeVersionMajor,
                                RuntimeVersionMinor,
                                (RuntimeVersionMajor == 0 && RuntimeVersionMinor == 0 && RuntimeVersionPatch == 0)
                                    ? 1
                                    : RuntimeVersionPatch);

            TraceLoggingWrite(g_traceProvider,
                              "xrGetInstanceProperties",
                              TLArg(instanceProperties->runtimeName, "RuntimeName"),
                              TLArg(xr::ToString(instanceProperties->runtimeVersion).c_str(), "RuntimeVersion"));

            return XR_SUCCESS;
        }

        XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) override {
            TraceLoggingWrite(g_traceProvider, "xrPollEvent", TLPArg(instance, "Instance"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // Generate session events.
            if (m_sessionStateDirty) {
                XrEventDataSessionStateChanged* const buffer =
                    reinterpret_cast<XrEventDataSessionStateChanged*>(eventData);
                buffer->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
                buffer->next = nullptr;
                buffer->session = (XrSession)1;
                buffer->state = m_sessionState;
                buffer->time = pvrTimeToXrTime(m_sessionStateEventTime);

                TraceLoggingWrite(g_traceProvider,
                                  "xrPollEvent",
                                  TLPArg(buffer->session, "Session"),
                                  TLArg(xr::ToCString(buffer->state), "State"),
                                  TLArg(buffer->time, "Time"));

                m_sessionStateDirty = false;

                if (m_sessionState == XR_SESSION_STATE_IDLE) {
                    m_sessionState = XR_SESSION_STATE_READY;
                    m_sessionStateDirty = true;
                    m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);
                }

                return XR_SUCCESS;
            }

            return XR_EVENT_UNAVAILABLE;
        }

        XrResult xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) override {
            if (getInfo->type != XR_TYPE_SYSTEM_GET_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystem",
                              TLPArg(instance, "Instance"),
                              TLArg(xr::ToCString(getInfo->formFactor), "FormFactor"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (getInfo->formFactor != XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) {
                return XR_ERROR_FORM_FACTOR_UNSUPPORTED;
            }

            // Create the PVR session.
            CHECK_PVRCMD(pvr_createSession(m_pvr, &m_pvrSession));

            // Check for HMD presence.
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
            if (!(status.ServiceReady && status.HmdPresent)) {
                return XR_ERROR_FORM_FACTOR_UNAVAILABLE;
            }

            // Cache common information.
            CHECK_PVRCMD(pvr_getEyeRenderInfo(m_pvrSession, pvrEye_Left, &m_cachedEyeInfo[0]));
            CHECK_PVRCMD(pvr_getEyeRenderInfo(m_pvrSession, pvrEye_Right, &m_cachedEyeInfo[1]));

            // Setup common parameters.
            CHECK_PVRCMD(pvr_setTrackingOriginType(m_pvrSession, pvrTrackingOrigin_EyeLevel));

            m_systemCreated = true;
            *systemId = (XrSystemId)1;

            TraceLoggingWrite(g_traceProvider, "xrGetSystem", TLArg((int)*systemId, "SystemId"));

            return XR_SUCCESS;
        }

        XrResult xrGetSystemProperties(XrInstance instance,
                                       XrSystemId systemId,
                                       XrSystemProperties* properties) override {
            if (properties->type != XR_TYPE_SYSTEM_PROPERTIES) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystemProperties",
                              TLPArg(instance, "Instance"),
                              TLArg((int)systemId, "SystemId"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (!m_systemCreated || systemId != (XrSystemId)1) {
                return XR_ERROR_SYSTEM_INVALID;
            }

            // Query HMD properties.
            pvrHmdInfo info{};
            CHECK_PVRCMD(pvr_getHmdInfo(m_pvrSession, &info));
            TraceLoggingWrite(g_traceProvider,
                              "PVR_HmdInfo",
                              TLArg(info.VendorId, "VendorId"),
                              TLArg(info.ProductId, "ProductId"),
                              TLArg(info.Manufacturer, "Manufacturer"),
                              TLArg(info.ProductName, "ProductName"),
                              TLArg(info.SerialNumber, "SerialNumber"),
                              TLArg(info.FirmwareMinor, "FirmwareMinor"),
                              TLArg(info.FirmwareMajor, "FirmwareMajor"),
                              TLArg(info.Resolution.w, "ResolutionWidth"),
                              TLArg(info.Resolution.h, "ResolutionHeight"));

            properties->vendorId = info.VendorId;
            sprintf_s(properties->systemName, sizeof(properties->systemName), "%s", info.ProductName);
            properties->systemId = systemId;

            properties->trackingProperties.positionTracking = XR_TRUE;
            properties->trackingProperties.orientationTracking = XR_TRUE;

            properties->graphicsProperties.maxLayerCount = pvrMaxLayerCount;
            properties->graphicsProperties.maxSwapchainImageWidth = 16384;
            properties->graphicsProperties.maxSwapchainImageHeight = 16384;

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystemProperties",
                              TLArg((int)properties->systemId, "SystemId"),
                              TLArg(properties->vendorId, "VendorId"),
                              TLArg(properties->systemName, "SystemName"),
                              TLArg(properties->trackingProperties.positionTracking, "PositionTracking"),
                              TLArg(properties->trackingProperties.orientationTracking, "OrientationTracking"),
                              TLArg(properties->graphicsProperties.maxLayerCount, "MaxLayerCount"),
                              TLArg(properties->graphicsProperties.maxSwapchainImageWidth, "MaxSwapchainImageWidth"),
                              TLArg(properties->graphicsProperties.maxSwapchainImageHeight, "MaxSwapchainImageHeight"));

            return XR_SUCCESS;
        }

        XrResult xrEnumerateEnvironmentBlendModes(XrInstance instance,
                                                  XrSystemId systemId,
                                                  XrViewConfigurationType viewConfigurationType,
                                                  uint32_t environmentBlendModeCapacityInput,
                                                  uint32_t* environmentBlendModeCountOutput,
                                                  XrEnvironmentBlendMode* environmentBlendModes) override {
            // We only support immersive VR mode.
            static const XrEnvironmentBlendMode blendModes[] = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateEnvironmentBlendModes",
                              TLPArg(instance, "Instance"),
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

            if (environmentBlendModes) {
                for (uint32_t i = 0; i < *environmentBlendModeCountOutput; i++) {
                    environmentBlendModes[i] = blendModes[i];
                    TraceLoggingWrite(g_traceProvider,
                                      "xrEnumerateEnvironmentBlendModes",
                                      TLArg(xr::ToCString(environmentBlendModes[i]), "EnvironmentBlendMode"));
                }
            }

            return XR_SUCCESS;
        }

        //
        // Graphics API management.
        //

        XrResult xrGetD3D11GraphicsRequirementsKHR(XrInstance instance,
                                                   XrSystemId systemId,
                                                   XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
            if (graphicsRequirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetD3D11GraphicsRequirementsKHR",
                              TLPArg(instance, "Instance"),
                              TLArg((int)systemId, "SystemId"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (!m_systemCreated || systemId != (XrSystemId)1) {
                return XR_ERROR_SYSTEM_INVALID;
            }

            if (!m_isD3D11Supported) {
                return XR_ERROR_FUNCTION_UNSUPPORTED;
            }

            // Get the display device LUID.
            pvrDisplayInfo info{};
            CHECK_PVRCMD(pvr_getEyeDisplayInfo(m_pvrSession, pvrEye_Left, &info));
            TraceLoggingWrite(g_traceProvider,
                              "PVR_EyeDisplayInfo",
                              TraceLoggingCharArray((char*)&info.luid, sizeof(LUID), "Luid"),
                              TLArg(info.edid_vid, "EdidVid"),
                              TLArg(info.edid_pid, "EdidPid"),
                              TLArg(info.pos_x, "PosX"),
                              TLArg(info.pos_y, "PosY"),
                              TLArg(info.width, "Width"),
                              TLArg(info.height, "Height"),
                              TLArg(info.refresh_rate, "RefreshRate"),
                              TLArg((int)info.disp_state, "DispState"),
                              TLArg((int)info.eye_display, "EyeDisplay"),
                              TLArg((int)info.eye_rotate, "EyeRotate"));

            memcpy(&m_adapterLuid, &info.luid, sizeof(LUID));

            memcpy(&graphicsRequirements->adapterLuid, &m_adapterLuid, sizeof(LUID));
            graphicsRequirements->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

            TraceLoggingWrite(
                g_traceProvider,
                "xrGetD3D11GraphicsRequirementsKHR",
                TraceLoggingCharArray((char*)&graphicsRequirements->adapterLuid, sizeof(LUID), "AdapterLuid"),
                TLArg((int)graphicsRequirements->minFeatureLevel, "MinFeatureLevel"));

            // We also store the expected frame duration.
            m_frameDuration = 1.0 / info.refresh_rate;

            m_graphicsRequirementQueried = true;

            return XR_SUCCESS;
        }

        //
        // Session management.
        //

        XrResult xrCreateSession(XrInstance instance,
                                 const XrSessionCreateInfo* createInfo,
                                 XrSession* session) override {
            if (createInfo->type != XR_TYPE_SESSION_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateSession",
                              TLPArg(instance, "Instance"),
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

            // Get the graphics device.
            bool hasGraphicsBindings = false;
            const XrBaseInStructure* entry = reinterpret_cast<const XrBaseInStructure*>(createInfo->next);
            while (entry) {
                if (m_isD3D11Supported && entry->type == XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) {
                    const XrGraphicsBindingD3D11KHR* d3dBindings =
                        reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(entry);

                    // Check that this is the correct adapter for the HMD.
                    ComPtr<IDXGIDevice> dxgiDevice;
                    CHECK_HRCMD(d3dBindings->device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

                    ComPtr<IDXGIAdapter> adapter;
                    CHECK_HRCMD(dxgiDevice->GetAdapter(adapter.ReleaseAndGetAddressOf()));

                    DXGI_ADAPTER_DESC desc;
                    CHECK_HRCMD(adapter->GetDesc(&desc));

                    std::string deviceName;
                    const std::wstring wadapterDescription(desc.Description);
                    std::transform(wadapterDescription.begin(),
                                   wadapterDescription.end(),
                                   std::back_inserter(deviceName),
                                   [](wchar_t c) { return (char)c; });

                    TraceLoggingWrite(g_traceProvider, "xrCreateSession", TLArg(deviceName.c_str(), "AdapterName"));
                    Log("Using adapter: %s\n", deviceName.c_str());

                    if (memcmp(&desc.AdapterLuid, &m_adapterLuid, sizeof(LUID))) {
                        return XR_ERROR_GRAPHICS_DEVICE_INVALID;
                    }
                    m_d3d11Device = d3dBindings->device;
                    m_d3d11Device->GetImmediateContext(m_d3d11DeviceContext.ReleaseAndGetAddressOf());

                    hasGraphicsBindings = true;
                }

                entry = entry->next;
            }

            if (!hasGraphicsBindings) {
                return XR_ERROR_GRAPHICS_DEVICE_INVALID;
            }

            m_sessionCreated = true;
            *session = (XrSession)1;

            // FIXME: Reset the session and frame state here.
            m_sessionState = XR_SESSION_STATE_IDLE;
            m_sessionStateDirty = true;
            m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

            m_frameWaited = m_frameBegun = false;
            m_lastFrameWaitedTime.reset();

            // Create a reference space with the origin and the HMD pose.
            {
                XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                spaceInfo.poseInReferenceSpace = Pose::Identity();
                spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
                CHECK_XRCMD(xrCreateReferenceSpace(*session, &spaceInfo, &m_originSpace));
            }
            {
                XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                spaceInfo.poseInReferenceSpace = Pose::Identity();
                spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                CHECK_XRCMD(xrCreateReferenceSpace(*session, &spaceInfo, &m_viewSpace));
            }

            TraceLoggingWrite(g_traceProvider, "xrCreateSession", TLPArg(*session, "Session"));

            return XR_SUCCESS;
        }

        XrResult xrDestroySession(XrSession session) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroySession", TLPArg(session, "Session"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // Destroy all swapchains.
            while (m_swapchains.size()) {
                CHECK_XRCMD(xrDestroySwapchain(*m_swapchains.begin()));
            }

            // Destroy reference spaces.
            CHECK_XRCMD(xrDestroySpace(m_originSpace));
            m_originSpace = XR_NULL_HANDLE;
            CHECK_XRCMD(xrDestroySpace(m_viewSpace));
            m_viewSpace = XR_NULL_HANDLE;

            // FIXME: Add session and frame resource cleanup here.
            m_d3d11DeviceContext.Reset();
            m_d3d11Device.Reset();
            m_sessionState = XR_SESSION_STATE_UNKNOWN;
            m_sessionStateDirty = false;
            m_sessionCreated = false;

            return XR_SUCCESS;
        }

        XrResult xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) override {
            if (beginInfo->type != XR_TYPE_SESSION_BEGIN_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(
                g_traceProvider,
                "xrBeginSession",
                TLPArg(session, "Session"),
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

        XrResult xrEndSession(XrSession session) override {
            TraceLoggingWrite(g_traceProvider, "xrEndSession", TLPArg(session, "Session"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (m_sessionState != XR_SESSION_STATE_STOPPING) {
                return XR_ERROR_SESSION_NOT_STOPPING;
            }

            m_sessionState = XR_SESSION_STATE_IDLE;
            m_sessionStateDirty = true;
            m_sessionStateEventTime = pvr_getTimeSeconds(m_pvr);

            return XR_SUCCESS;
        }

        XrResult xrRequestExitSession(XrSession session) override {
            TraceLoggingWrite(g_traceProvider, "xrRequestExitSession", TLPArg(session, "Session"));

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

        //
        // Spaces management.
        //

        XrResult xrEnumerateReferenceSpaces(XrSession session,
                                            uint32_t spaceCapacityInput,
                                            uint32_t* spaceCountOutput,
                                            XrReferenceSpaceType* spaces) override {
            static const XrReferenceSpaceType referenceSpaces[] = {XR_REFERENCE_SPACE_TYPE_VIEW,
                                                                   XR_REFERENCE_SPACE_TYPE_LOCAL};

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateReferenceSpaces",
                              TLPArg(session, "Session"),
                              TLArg(spaceCapacityInput, "SpaceCapacityInput"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (spaceCapacityInput && spaceCapacityInput < ARRAYSIZE(referenceSpaces)) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *spaceCountOutput = ARRAYSIZE(referenceSpaces);
            TraceLoggingWrite(
                g_traceProvider, "xrEnumerateReferenceSpaces", TLArg(*spaceCountOutput, "SpaceCountOutput"));

            if (spaces) {
                for (uint32_t i = 0; i < *spaceCountOutput; i++) {
                    spaces[i] = referenceSpaces[i];
                    TraceLoggingWrite(
                        g_traceProvider, "xrEnumerateReferenceSpaces", TLArg(xr::ToCString(spaces[i]), "Space"));
                }
            }

            return XR_SUCCESS;
        }

        XrResult xrCreateReferenceSpace(XrSession session,
                                        const XrReferenceSpaceCreateInfo* createInfo,
                                        XrSpace* space) override {
            if (createInfo->type != XR_TYPE_REFERENCE_SPACE_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateReferenceSpace",
                              TLPArg(session, "Session"),
                              TLArg(xr::ToCString(createInfo->referenceSpaceType), "ReferenceSpaceType"),
                              TLArg(xr::ToString(createInfo->poseInReferenceSpace).c_str(), "PoseInReferenceSpace"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We still accept STAGE even though we don't advertise it... Certain applications like OpenComposite assume
            // that it is available. It will implicitly revert to LOCAL.
            if (createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_VIEW &&
                createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_LOCAL &&
                createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_STAGE) {
                return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED;
            }

            // Create the internal struct.
            Space* xrSpace = new Space;
            xrSpace->referenceType = createInfo->referenceSpaceType;
            xrSpace->poseInSpace = createInfo->poseInReferenceSpace;

            *space = (XrSpace)xrSpace;

            // Maintain a list of known spaces for validation and cleanup.
            m_spaces.insert(*space);

            TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace", TLPArg(*space, "Space"));

            return XR_SUCCESS;
        }

        XrResult xrGetReferenceSpaceBoundsRect(XrSession session,
                                               XrReferenceSpaceType referenceSpaceType,
                                               XrExtent2Df* bounds) override {
            TraceLoggingWrite(g_traceProvider,
                              "xrGetReferenceSpaceBoundsRect",
                              TLPArg(session, "Session"),
                              TLArg(xr::ToCString(referenceSpaceType), "ReferenceSpaceType"));

            bounds->width = bounds->height = 0.f;

            return XR_SPACE_BOUNDS_UNAVAILABLE;
        }

        XrResult xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) override {
            if (location->type != XR_TYPE_SPACE_LOCATION) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrLocateSpace",
                              TLPArg(space, "Space"),
                              TLPArg(baseSpace, "BaseSpace"),
                              TLArg(time, "Time"));

            location->locationFlags = 0;

            // TODO: Do nothing for action spaces.
            if (space == (XrSpace)1 || baseSpace == (XrSpace)1) {
                return XR_SUCCESS;
            }

            if (!m_spaces.count(space) || !m_spaces.count(baseSpace)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            Space* xrSpace = (Space*)space;
            Space* xrBaseSpace = (Space*)baseSpace;

            // Locate the HMD for view poses, otherwise use the origin.
            XrPosef pose = Pose::Identity();
            if ((xrSpace->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW ||
                 xrBaseSpace->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW) &&
                xrSpace->referenceType != xrBaseSpace->referenceType) {
                pvrPoseStatef state{};
                CHECK_PVRCMD(
                    pvr_getTrackedDevicePoseState(m_pvrSession, pvrTrackedDevice_HMD, xrTimeToPvrTime(time), &state));
                TraceLoggingWrite(g_traceProvider,
                                  "PVR_HmdPoseState",
                                  TLArg(state.StatusFlags, "StatusFlags"),
                                  TLArg(xr::ToString(state.ThePose).c_str(), "Pose"));

                pose = pvrPoseToXrPose(state.ThePose);
                if (state.StatusFlags & pvrStatus_OrientationTracked) {
                    location->locationFlags |=
                        (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT);
                }
                // For 9-axis setups, we propagate the Orientation bit to Position.
                if (state.StatusFlags & pvrStatus_PositionTracked || state.StatusFlags & pvrStatus_OrientationTracked) {
                    location->locationFlags |=
                        (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT);
                }

                // If the view is the reference, then we need the inverted pose.
                if (xrBaseSpace->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW) {
                    StoreXrPose(&location->pose, LoadInvertedXrPose(location->pose));
                }
            } else {
                location->locationFlags =
                    (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT |
                     XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT);
            }

            // Apply the offset transforms.
            StoreXrPose(
                &location->pose,
                XMMatrixMultiply(LoadXrPose(xrSpace->poseInSpace),
                                 XMMatrixMultiply(LoadXrPose(pose), LoadInvertedXrPose(xrBaseSpace->poseInSpace))));

            TraceLoggingWrite(g_traceProvider,
                              "xrLocateSpace",
                              TLArg(location->locationFlags, "LocationFlags"),
                              TLArg(xr::ToString(location->pose).c_str(), "Pose"));

            return XR_SUCCESS;
        }

        XrResult xrDestroySpace(XrSpace space) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroySpace", TLPArg(space, "Space"));

            // TODO: Do nothing for action spaces.
            if (space == (XrSpace)1) {
                return XR_SUCCESS;
            }

            if (!m_spaces.count(space)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            Space* xrSpace = (Space*)space;
            delete xrSpace;

            m_spaces.erase(space);

            return XR_SUCCESS;
        }

        //
        // Views and swapchains management.
        //

        XrResult xrEnumerateViewConfigurations(XrInstance instance,
                                               XrSystemId systemId,
                                               uint32_t viewConfigurationTypeCapacityInput,
                                               uint32_t* viewConfigurationTypeCountOutput,
                                               XrViewConfigurationType* viewConfigurationTypes) override {
            // We only support Stereo 3D.
            static const XrViewConfigurationType types[] = {XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateViewConfigurations",
                              TLPArg(instance, "Instance"),
                              TLArg((int)systemId, "SystemId"),
                              TLArg(viewConfigurationTypeCapacityInput, "ViewConfigurationTypeCapacityInput"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (!m_systemCreated || systemId != (XrSystemId)1) {
                return XR_ERROR_SYSTEM_INVALID;
            }

            if (viewConfigurationTypeCapacityInput && viewConfigurationTypeCapacityInput < ARRAYSIZE(types)) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *viewConfigurationTypeCountOutput = ARRAYSIZE(types);
            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateViewConfigurations",
                              TLArg(*viewConfigurationTypeCountOutput, "ViewConfigurationTypeCountOutput"));

            if (viewConfigurationTypes) {
                for (uint32_t i = 0; i < *viewConfigurationTypeCountOutput; i++) {
                    viewConfigurationTypes[i] = types[i];
                    TraceLoggingWrite(g_traceProvider,
                                      "xrEnumerateViewConfigurations",
                                      TLArg(xr::ToCString(viewConfigurationTypes[i]), "ViewConfigurationType"));
                }
            }

            return XR_SUCCESS;
        }

        XrResult xrGetViewConfigurationProperties(XrInstance instance,
                                                  XrSystemId systemId,
                                                  XrViewConfigurationType viewConfigurationType,
                                                  XrViewConfigurationProperties* configurationProperties) override {
            if (configurationProperties->type != XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetViewConfigurationProperties",
                              TLPArg(instance, "Instance"),
                              TLArg((int)systemId, "SystemId"),
                              TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (!m_systemCreated || systemId != (XrSystemId)1) {
                return XR_ERROR_SYSTEM_INVALID;
            }

            if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
            }

            configurationProperties->viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            configurationProperties->fovMutable = XR_TRUE;

            TraceLoggingWrite(
                g_traceProvider,
                "xrGetViewConfigurationProperties",
                TLArg(xr::ToCString(configurationProperties->viewConfigurationType), "ViewConfigurationType"),
                TLArg(configurationProperties->fovMutable, "FovMutable"));

            return XR_SUCCESS;
        }

        XrResult xrEnumerateViewConfigurationViews(XrInstance instance,
                                                   XrSystemId systemId,
                                                   XrViewConfigurationType viewConfigurationType,
                                                   uint32_t viewCapacityInput,
                                                   uint32_t* viewCountOutput,
                                                   XrViewConfigurationView* views) override {
            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateViewConfigurationViews",
                              TLPArg(instance, "Instance"),
                              TLArg((int)systemId, "SystemId"),
                              TLArg(viewCapacityInput, "ViewCapacityInput"),
                              TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (!m_systemCreated || systemId != (XrSystemId)1) {
                return XR_ERROR_SYSTEM_INVALID;
            }

            if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
            }

            if (viewCapacityInput && viewCapacityInput < xr::StereoView::Count) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *viewCountOutput = xr::StereoView::Count;
            TraceLoggingWrite(
                g_traceProvider, "xrEnumerateViewConfigurationViews", TLArg(*viewCountOutput, "ViewCountOutput"));

            if (views) {
                for (uint32_t i = 0; i < *viewCountOutput; i++) {
                    if (views[i].type != XR_TYPE_VIEW_CONFIGURATION_VIEW) {
                        return XR_ERROR_VALIDATION_FAILURE;
                    }

                    views[i].maxImageRectWidth = 16384;
                    views[i].maxImageRectHeight = 16384;

                    // TODO: Do we support multisampling?
                    views[i].recommendedSwapchainSampleCount = views[i].maxSwapchainSampleCount = 1;

                    // Recommend the resolution with distortion accounted for.
                    views[i].recommendedImageRectWidth = m_cachedEyeInfo[i].DistortedViewport.Size.w;
                    views[i].recommendedImageRectHeight = m_cachedEyeInfo[i].DistortedViewport.Size.h;

                    TraceLoggingWrite(
                        g_traceProvider,
                        "xrEnumerateViewConfigurationViews",
                        TLArg(views[i].maxImageRectWidth, "MaxImageRectWidth"),
                        TLArg(views[i].maxImageRectHeight, "MaxImageRectHeight"),
                        TLArg(views[i].maxSwapchainSampleCount, "MaxSwapchainSampleCount"),
                        TLArg(views[i].recommendedImageRectWidth, "RecommendedImageRectWidth"),
                        TLArg(views[i].recommendedImageRectHeight, "RecommendedImageRectHeight"),
                        TLArg(views[i].recommendedSwapchainSampleCount, "RecommendedSwapchainSampleCount"));
                }
            }

            return XR_SUCCESS;
        }

        XrResult xrEnumerateSwapchainFormats(XrSession session,
                                             uint32_t formatCapacityInput,
                                             uint32_t* formatCountOutput,
                                             int64_t* formats) override {
            // We match exactly what pvrTextureFormat lists for use.
            static const DXGI_FORMAT d3dFormats[] = {
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // Prefer SRGB formats.
                DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_B8G8R8A8_UNORM,
                DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
                DXGI_FORMAT_B8G8R8X8_UNORM,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                DXGI_FORMAT_D32_FLOAT, // Prefer 32-bit depth.
                DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
                DXGI_FORMAT_D24_UNORM_S8_UINT,
                DXGI_FORMAT_D16_UNORM,

                DXGI_FORMAT_BC1_UNORM,
                DXGI_FORMAT_BC1_UNORM_SRGB,
                DXGI_FORMAT_BC2_UNORM,
                DXGI_FORMAT_BC2_UNORM_SRGB,
                DXGI_FORMAT_BC3_UNORM,
                DXGI_FORMAT_BC3_UNORM_SRGB,
                DXGI_FORMAT_BC6H_UF16,
                DXGI_FORMAT_BC6H_SF16,
                DXGI_FORMAT_BC7_UNORM,
                DXGI_FORMAT_BC7_UNORM_SRGB,
                DXGI_FORMAT_R11G11B10_FLOAT,
            };

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainFormats",
                              TLPArg(session, "Session"),
                              TLArg(formatCapacityInput, "FormatCapacityInput"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // FIXME: Here we assume D3D11 since it's the only API supported for now.
            if (formatCapacityInput && formatCapacityInput < ARRAYSIZE(d3dFormats)) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *formatCountOutput = ARRAYSIZE(d3dFormats);
            TraceLoggingWrite(
                g_traceProvider, "xrEnumerateSwapchainFormats", TLArg(*formatCountOutput, "FormatCountOutput"));

            if (formats) {
                for (uint32_t i = 0; i < *formatCountOutput; i++) {
                    formats[i] = (int64_t)d3dFormats[i];
                    TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats", TLArg(formats[i], "Format"));
                }
            }

            return XR_SUCCESS;
        }

        XrResult xrCreateSwapchain(XrSession session,
                                   const XrSwapchainCreateInfo* createInfo,
                                   XrSwapchain* swapchain) override {
            if (createInfo->type != XR_TYPE_SWAPCHAIN_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateSwapchain",
                              TLPArg(session, "Session"),
                              TLArg(createInfo->arraySize, "ArraySize"),
                              TLArg(createInfo->width, "Width"),
                              TLArg(createInfo->height, "Height"),
                              TLArg(createInfo->createFlags, "CreateFlags"),
                              TLArg(createInfo->format, "Format"),
                              TLArg(createInfo->faceCount, "FaceCount"),
                              TLArg(createInfo->mipCount, "MipCount"),
                              TLArg(createInfo->sampleCount, "SampleCount"),
                              TLArg(createInfo->usageFlags, "UsageFlags"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We don't support cubemaps.
            if (createInfo->faceCount != 1) {
                return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
            }

            if (createInfo->createFlags & XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT) {
                return XR_ERROR_FEATURE_UNSUPPORTED;
            }

            pvrTextureSwapChainDesc desc{};

            // FIXME: Here we assume D3D11 since it's the only API supported for now.
            desc.Format = dxgiToPvrTextureFormat((DXGI_FORMAT)createInfo->format);
            if (desc.Format == PVR_FORMAT_UNKNOWN) {
                return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
            }
            desc.MiscFlags = pvrTextureMisc_DX_Typeless; // OpenXR requires to return typeless texures.

            // Request a swapchain from PVR.
            desc.Type = pvrTexture_2D;
            desc.StaticImage = !!(createInfo->createFlags & XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT);
            desc.ArraySize = createInfo->arraySize;
            desc.Width = createInfo->width;
            desc.Height = createInfo->height;
            desc.MipLevels = createInfo->mipCount;
            if (desc.MipLevels > 1) {
                desc.MiscFlags |= pvrTextureMisc_AllowGenerateMips;
            }
            desc.SampleCount = createInfo->sampleCount;

            // FIXME: Here we assume D3D11 since it's the only API supported for now.
            if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) {
                desc.BindFlags |= pvrTextureBind_DX_RenderTarget;
            }
            if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                desc.BindFlags |= pvrTextureBind_DX_DepthStencil;
            }
            if (createInfo->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
                desc.BindFlags |= pvrTextureBind_DX_UnorderedAccess;
            }

            pvrTextureSwapChain pvrSwapchain{};
            CHECK_PVRCMD(pvr_createTextureSwapChainDX(m_pvrSession, m_d3d11Device.Get(), &desc, &pvrSwapchain));

            // Create the internal struct.
            Swapchain* xrSwapchain = new Swapchain;
            xrSwapchain->pvrSwapchain.push_back(pvrSwapchain);
            xrSwapchain->pvrDesc = desc;

            // Lazily-filled state.
            for (int i = 0; i < desc.ArraySize; i++) {
                if (i) {
                    xrSwapchain->pvrSwapchain.push_back(nullptr);
                }
                xrSwapchain->images.push_back({});
            }

            *swapchain = (XrSwapchain)xrSwapchain;

            // Maintain a list of known swapchains for validation and cleanup.
            m_swapchains.insert(*swapchain);

            TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain", TLPArg(*swapchain, "Swapchain"));

            return XR_SUCCESS;
        }

        XrResult xrDestroySwapchain(XrSwapchain swapchain) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain", TLPArg(swapchain, "Swapchain"));

            if (!m_swapchains.count(swapchain)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            Swapchain* xrSwapchain = (Swapchain*)swapchain;

            while (!xrSwapchain->pvrSwapchain.empty()) {
                auto pvrSwapchain = xrSwapchain->pvrSwapchain.back();
                if (pvrSwapchain) {
                    pvr_destroyTextureSwapChain(m_pvrSession, pvrSwapchain);
                }
                xrSwapchain->pvrSwapchain.pop_back();
            }

            m_swapchains.erase(swapchain);

            return XR_SUCCESS;
        }

        XrResult xrEnumerateSwapchainImages(XrSwapchain swapchain,
                                            uint32_t imageCapacityInput,
                                            uint32_t* imageCountOutput,
                                            XrSwapchainImageBaseHeader* images) override {
            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateSwapchainImages",
                              TLPArg(swapchain, "Swapchain"),
                              TLArg(imageCapacityInput, "ImageCapacityInput"));

            if (!m_swapchains.count(swapchain)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            Swapchain* xrSwapchain = (Swapchain*)swapchain;

            int count = -1;
            CHECK_PVRCMD(pvr_getTextureSwapChainLength(m_pvrSession, xrSwapchain->pvrSwapchain[0], &count));

            if (imageCapacityInput && imageCapacityInput < (uint32_t)count) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *imageCountOutput = count;
            TraceLoggingWrite(
                g_traceProvider, "xrEnumerateSwapchainImages", TLArg(*imageCountOutput, "ImageCountOutput"));

            if (images) {
                const bool alreadyCached = !xrSwapchain->images[0].empty();

                // FIXME: Here we assume D3D11 since it's the only API supported for now.

                // Query the textures for the swapchain.
                XrSwapchainImageD3D11KHR* d3d11Images = reinterpret_cast<XrSwapchainImageD3D11KHR*>(images);
                for (uint32_t i = 0; i < *imageCountOutput; i++) {
                    if (d3d11Images->type != XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) {
                        return XR_ERROR_VALIDATION_FAILURE;
                    }

                    CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
                        m_pvrSession, xrSwapchain->pvrSwapchain[0], i, IID_PPV_ARGS(&d3d11Images[i].texture)));

                    if (!alreadyCached) {
                        xrSwapchain->images[0].push_back(d3d11Images[i].texture);
                    }

                    TraceLoggingWrite(
                        g_traceProvider, "xrEnumerateSwapchainImages", TLPArg(d3d11Images[i].texture, "Texture"));
                }
            }

            return XR_SUCCESS;
        }

        XrResult xrAcquireSwapchainImage(XrSwapchain swapchain,
                                         const XrSwapchainImageAcquireInfo* acquireInfo,
                                         uint32_t* index) override {
            if (acquireInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage", TLPArg(swapchain, "Swapchain"));

            if (!m_swapchains.count(swapchain)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            Swapchain* xrSwapchain = (Swapchain*)swapchain;

            // Query the image index from PVR.
            int pvrIndex = -1;
            CHECK_PVRCMD(pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain->pvrSwapchain[0], &pvrIndex));

            *index = pvrIndex;

            TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage", TLArg(*index, "Index"));

            return XR_SUCCESS;
        }

        XrResult xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) override {
            if (waitInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrWaitSwapchainImage",
                              TLPArg(swapchain, "Swapchain"),
                              TLArg(waitInfo->timeout, "Timeout"));

            if (!m_swapchains.count(swapchain)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We assume that our frame timing in xrWaitFrame() guaranteed availability of the next image. No wait.

            return XR_SUCCESS;
        }

        XrResult xrReleaseSwapchainImage(XrSwapchain swapchain,
                                         const XrSwapchainImageReleaseInfo* releaseInfo) override {
            if (releaseInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage", TLPArg(swapchain, "Swapchain"));

            if (!m_swapchains.count(swapchain)) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We will commit the texture to PVR during xrEndFrame() in order to handle texture arrays properly. Nothing
            // to do here.

            return XR_SUCCESS;
        }

        //
        // Frame management.
        //

        XrResult xrWaitFrame(XrSession session,
                             const XrFrameWaitInfo* frameWaitInfo,
                             XrFrameState* frameState) override {
            if ((frameWaitInfo && frameWaitInfo->type != XR_TYPE_FRAME_WAIT_INFO) ||
                frameState->type != XR_TYPE_FRAME_STATE) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrWaitFrame", TLPArg(session, "Session"));

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

        XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) override {
            if (frameBeginInfo && frameBeginInfo->type != XR_TYPE_FRAME_BEGIN_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrBeginFrame", TLPArg(session, "Session"));

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
                TraceLoggingWrite(g_traceProvider, "BeginFrame", TLArg(m_nextFrameIndex, "CurrentFrameIndex"));

                m_frameWaited = false;
                m_frameBegun = true;

                // Signal xrWaitFrame().
                TraceLoggingWrite(g_traceProvider, "BeginFrame_Signal");
                m_frameCondVar.notify_one();
            }

            return !frameDiscarded ? XR_SUCCESS : XR_FRAME_DISCARDED;
        }

        XrResult xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) override {
            if (frameEndInfo->type != XR_TYPE_FRAME_END_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrEndFrame",
                              TLPArg(session, "Session"),
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

                std::set<std::pair<pvrTextureSwapChain, uint32_t>> committedSwapchainImages;

                // Construct the list of layers.
                std::vector<pvrLayer_Union> layersAllocator;
                std::vector<pvrLayerHeader*> layers;
                for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                    layersAllocator.push_back({});
                    auto& layer = layersAllocator.back();

                    // TODO: What do we do with layerFlags?

                    if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
                        const XrCompositionLayerProjection* proj =
                            reinterpret_cast<const XrCompositionLayerProjection*>(frameEndInfo->layers[i]);

                        TraceLoggingWrite(g_traceProvider,
                                          "xrEndFrame_Layer",
                                          TLArg("Proj", "Type"),
                                          TLArg(proj->layerFlags, "Flags"),
                                          TLPArg(proj->space, "Space"));

                        layer.Header.Type = pvrLayerType_EyeFov;

                        // TODO: layer.EyeFov.SensorSampleTime = XXX? Latch value from latest xrLocateViews()?

                        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                            TraceLoggingWrite(
                                g_traceProvider,
                                "xrEndFrame_View",
                                TLArg("Proj", "Type"),
                                TLArg(eye, "Index"),
                                TLPArg(proj->views[eye].subImage.swapchain, "Swapchain"),
                                TLArg(proj->views[eye].subImage.imageArrayIndex, "ImageArrayIndex"),
                                TLArg(xr::ToString(proj->views[eye].subImage.imageRect).c_str(), "ImageRect"),
                                TLArg(xr::ToString(proj->views[eye].pose).c_str(), "Pose"),
                                TLArg(xr::ToString(proj->views[eye].fov).c_str(), "Fov"));

                            if (!m_swapchains.count(proj->views[eye].subImage.swapchain)) {
                                return XR_ERROR_HANDLE_INVALID;
                            }

                            Swapchain* xrSwapchain = (Swapchain*)proj->views[eye].subImage.swapchain;

                            // Fill out color buffer information.
                            prepareAndCommitSwapchainImage(
                                *xrSwapchain, proj->views[eye].subImage.imageArrayIndex, committedSwapchainImages);
                            layer.EyeFov.ColorTexture[eye] =
                                xrSwapchain->pvrSwapchain[proj->views[eye].subImage.imageArrayIndex];

                            if (!isValidSwapchainRect(xrSwapchain->pvrDesc, proj->views[eye].subImage.imageRect)) {
                                return XR_ERROR_SWAPCHAIN_RECT_INVALID;
                            }
                            layer.EyeFov.Viewport[eye].x = proj->views[eye].subImage.imageRect.offset.x;
                            layer.EyeFov.Viewport[eye].y = proj->views[eye].subImage.imageRect.offset.y;
                            layer.EyeFov.Viewport[eye].width = proj->views[eye].subImage.imageRect.extent.width;
                            layer.EyeFov.Viewport[eye].height = proj->views[eye].subImage.imageRect.extent.height;

                            // Fill out pose and FOV information.
                            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
                            CHECK_XRCMD(
                                xrLocateSpace(proj->space, m_originSpace, frameEndInfo->displayTime, &location));
                            XrPosef transformed;
                            StoreXrPose(&transformed,
                                        XMMatrixMultiply(LoadXrPose(proj->views[eye].pose), LoadXrPose(location.pose)));
                            layer.EyeFov.RenderPose[eye] = xrPoseToPvrPose(transformed);

                            layer.EyeFov.Fov[eye].DownTan = -tan(proj->views[eye].fov.angleDown);
                            layer.EyeFov.Fov[eye].UpTan = tan(proj->views[eye].fov.angleUp);
                            layer.EyeFov.Fov[eye].LeftTan = -tan(proj->views[eye].fov.angleLeft);
                            layer.EyeFov.Fov[eye].RightTan = tan(proj->views[eye].fov.angleRight);
                        }
                    } else if (frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_QUAD) {
                        const XrCompositionLayerQuad* quad =
                            reinterpret_cast<const XrCompositionLayerQuad*>(frameEndInfo->layers[i]);

                        TraceLoggingWrite(g_traceProvider,
                                          "xrEndFrame_Layer",
                                          TLArg("Quad", "Type"),
                                          TLArg(quad->layerFlags, "Flags"),
                                          TLPArg(quad->space, "Space"));
                        TraceLoggingWrite(g_traceProvider,
                                          "xrEndFrame_View",
                                          TLArg("Quad", "Type"),
                                          TLPArg(quad->subImage.swapchain, "Swapchain"),
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

                        Swapchain* xrSwapchain = (Swapchain*)quad->subImage.swapchain;

                        // TODO: We ignore eyeVisibility as there is no equivalent.

                        // Fill out color buffer information.
                        prepareAndCommitSwapchainImage(
                            *xrSwapchain, quad->subImage.imageArrayIndex, committedSwapchainImages);
                        layer.Quad.ColorTexture = xrSwapchain->pvrSwapchain[quad->subImage.imageArrayIndex];

                        if (!isValidSwapchainRect(xrSwapchain->pvrDesc, quad->subImage.imageRect)) {
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
                                      "PVR_SubmitFrame_Begin",
                                      TLArg(m_nextFrameIndex, "CurrentFrameIndex"),
                                      TLArg(layers.size(), "NumLayers"));
                    CHECK_PVRCMD(
                        pvr_submitFrame(m_pvrSession, m_currentFrameIndex, layers.data(), (unsigned int)layers.size()));
                    TraceLoggingWrite(g_traceProvider, "PVR_SubmitFrame_End");
                }

                m_frameBegun = false;

                // Signal xrWaitFrame().
                TraceLoggingWrite(g_traceProvider, "EndFrame_Signal");
                m_frameCondVar.notify_one();
            }

            return XR_SUCCESS;
        }

        XrResult xrLocateViews(XrSession session,
                               const XrViewLocateInfo* viewLocateInfo,
                               XrViewState* viewState,
                               uint32_t viewCapacityInput,
                               uint32_t* viewCountOutput,
                               XrView* views) override {
            if (viewLocateInfo->type != XR_TYPE_VIEW_LOCATE_INFO || viewState->type != XR_TYPE_VIEW_STATE) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrLocateViews",
                              TLPArg(session, "Session"),
                              TLArg(xr::ToCString(viewLocateInfo->viewConfigurationType), "ViewConfigurationType"),
                              TLArg(viewLocateInfo->displayTime, "DisplayTime"),
                              TLPArg(viewLocateInfo->space, "Space"),
                              TLArg(viewCapacityInput, "ViewCapacityInput"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (viewLocateInfo->viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
            }

            if (viewCapacityInput && viewCapacityInput < xr::StereoView::Count) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *viewCountOutput = xr::StereoView::Count;
            TraceLoggingWrite(g_traceProvider, "xrLocateViews", TLArg(*viewCountOutput, "ViewCountOutput"));

            if (views) {
                // Get the HMD pose in the base space.
                XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
                CHECK_XRCMD(xrLocateSpace(m_viewSpace, viewLocateInfo->space, viewLocateInfo->displayTime, &location));
                viewState->viewStateFlags = location.locationFlags;

                if (viewState->viewStateFlags &
                    (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT)) {
                    // Calculate poses for each eye.
                    pvrPosef hmdToEyePose[xr::StereoView::Count];
                    hmdToEyePose[0] = m_cachedEyeInfo[0].HmdToEyePose;
                    hmdToEyePose[1] = m_cachedEyeInfo[1].HmdToEyePose;

                    pvrPosef eyePoses[xr::StereoView::Count]{{}, {}};
                    pvr_calcEyePoses(m_pvr, xrPoseToPvrPose(location.pose), hmdToEyePose, eyePoses);

                    for (uint32_t i = 0; i < *viewCountOutput; i++) {
                        if (views[i].type != XR_TYPE_VIEW) {
                            return XR_ERROR_VALIDATION_FAILURE;
                        }

                        views[i].pose = pvrPoseToXrPose(eyePoses[i]);
                        views[i].fov.angleDown = -atan(m_cachedEyeInfo[i].Fov.DownTan);
                        views[i].fov.angleUp = atan(m_cachedEyeInfo[i].Fov.UpTan);
                        views[i].fov.angleLeft = -atan(m_cachedEyeInfo[i].Fov.LeftTan);
                        views[i].fov.angleRight = atan(m_cachedEyeInfo[i].Fov.RightTan);

                        TraceLoggingWrite(
                            g_traceProvider, "xrLocateViews", TLArg(viewState->viewStateFlags, "ViewStateFlags"));
                        TraceLoggingWrite(g_traceProvider,
                                          "xrLocateViews",
                                          TLArg(xr::ToString(views[i].pose).c_str(), "Pose"),
                                          TLArg(xr::ToString(views[i].fov).c_str(), "Fov"));
                    }
                } else {
                    // All or nothing.
                    viewState->viewStateFlags = 0;
                    TraceLoggingWrite(
                        g_traceProvider, "xrLocateViews", TLArg(viewState->viewStateFlags, "ViewStateFlags"));
                }
            }

            return XR_SUCCESS;
        }

        //
        // Utilities.
        //

        XrResult xrConvertWin32PerformanceCounterToTimeKHR(XrInstance instance,
                                                           const LARGE_INTEGER* performanceCounter,
                                                           XrTime* time) {
            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrConvertWin32PerformanceCounterToTimeKHR",
                              TLPArg(instance, "Instance"),
                              TLArg(performanceCounter->QuadPart, "PerformanceCounter"));

            double pvrTime = (double)performanceCounter->QuadPart / m_qpcFrequency.QuadPart;
            pvrTime += m_pvrTimeFromQpcTimeOffset;

            *time = pvrTimeToXrTime(pvrTime);

            TraceLoggingWrite(g_traceProvider, "xrConvertWin32PerformanceCounterToTimeKHR", TLArg(*time, "Time"));

            return XR_SUCCESS;
        }

        XrResult xrConvertTimeToWin32PerformanceCounterKHR(XrInstance instance,
                                                           XrTime time,
                                                           LARGE_INTEGER* performanceCounter) {
            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrConvertTimeToWin32PerformanceCounterKHR",
                              TLPArg(instance, "Instance"),
                              TLArg(time, "Time"));

            double pvrTime = xrTimeToPvrTime(time);
            pvrTime -= m_pvrTimeFromQpcTimeOffset;

            performanceCounter->QuadPart = (LONGLONG)(pvrTime * m_qpcFrequency.QuadPart);

            TraceLoggingWrite(g_traceProvider,
                              "xrConvertTimeToWin32PerformanceCounterKHR",
                              TLArg(performanceCounter->QuadPart, "PerformanceCounter"));

            return XR_SUCCESS;
        }

        XrResult xrGetVisibilityMaskKHR(XrSession session,
                                        XrViewConfigurationType viewConfigurationType,
                                        uint32_t viewIndex,
                                        XrVisibilityMaskTypeKHR visibilityMaskType,
                                        XrVisibilityMaskKHR* visibilityMask) {
            if (visibilityMask->type != XR_TYPE_VISIBILITY_MASK_KHR) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetVisibilityMaskKHR",
                              TLPArg(session, "Session"),
                              TLArg(xr::ToCString(viewConfigurationType), "ViewConfigurationType"),
                              TLArg(viewIndex, "ViewIndex"),
                              TLArg(xr::ToCString(visibilityMaskType), "VisibilityMaskType"),
                              TLArg(visibilityMask->vertexCapacityInput, "VertexCapacityInput"),
                              TLArg(visibilityMask->indexCapacityInput, "IndexCapacityInput"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            if (viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
            }

            if (viewIndex >= xr::StereoView::Count) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            if (visibilityMaskType != XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR) {
                // TODO: This is a non-standard return for this function.
                return XR_ERROR_FEATURE_UNSUPPORTED;
            }

            const auto verticesCount =
                pvr_getEyeHiddenAreaMesh(m_pvrSession, !viewIndex ? pvrEye_Left : pvrEye_Right, nullptr, 0);
            TraceLoggingWrite(g_traceProvider, "PVR_EyeHiddenAreaMesh", TLArg(verticesCount, "VerticesCount"));

            if (visibilityMask->vertexCapacityInput == 0) {
                visibilityMask->vertexCountOutput = verticesCount;
                visibilityMask->indexCountOutput = verticesCount;
            } else if (visibilityMask->vertices && visibilityMask->indices) {
                if (visibilityMask->vertexCapacityInput < verticesCount ||
                    visibilityMask->indexCapacityInput < verticesCount) {
                    return XR_ERROR_SIZE_INSUFFICIENT;
                }

                static_assert(sizeof(XrVector2f) == sizeof(pvrVector2f));
                pvr_getEyeHiddenAreaMesh(m_pvrSession,
                                         !viewIndex ? pvrEye_Left : pvrEye_Right,
                                         (pvrVector2f*)visibilityMask->vertices,
                                         verticesCount);

                convertSteamVRToOpenXRHiddenMesh(
                    m_cachedEyeInfo[viewIndex].Fov, visibilityMask->vertices, visibilityMask->indices, verticesCount);
            }

            return XR_SUCCESS;
        }

        //
        // Actions management.
        // TODO: Not supported. We do the bare minimum so that the app will not crash but also detect common errors.
        //

        XrResult xrStringToPath(XrInstance instance, const char* pathString, XrPath* path) override {
            TraceLoggingWrite(
                g_traceProvider, "xrStringToPath", TLPArg(instance, "Instance"), TLArg(pathString, "String"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            std::string_view str(pathString);

            bool found = false;
            for (auto entry : m_strings) {
                if (entry.second == str) {
                    *path = entry.first;
                    found = true;
                    break;
                }
            }

            if (!found) {
                *path = (XrPath)++m_stringIndex;
                m_strings.insert_or_assign(*path, str);
            }

            TraceLoggingWrite(g_traceProvider, "xrStringToPath", TLArg(*path, "Path"));

            return XR_SUCCESS;
        }

        XrResult xrPathToString(XrInstance instance,
                                XrPath path,
                                uint32_t bufferCapacityInput,
                                uint32_t* bufferCountOutput,
                                char* buffer) override {
            TraceLoggingWrite(g_traceProvider,
                              "xrPathToString",
                              TLPArg(instance, "Instance"),
                              TLArg(path, "Path"),
                              TLArg(bufferCapacityInput, "BufferCapacityInput"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            const auto it = m_strings.find(path);
            if (it == m_strings.cend()) {
                return XR_ERROR_PATH_INVALID;
            }

            const auto& str = it->second;
            if (bufferCapacityInput && bufferCapacityInput < str.length()) {
                return XR_ERROR_SIZE_INSUFFICIENT;
            }

            *bufferCountOutput = (uint32_t)str.length();
            TraceLoggingWrite(g_traceProvider, "xrPathToString", TLArg(*bufferCountOutput, "BufferCountOutput"));

            if (buffer) {
                sprintf_s(buffer, bufferCapacityInput, "%s", str.c_str());
                TraceLoggingWrite(g_traceProvider, "xrPathToString", TLArg(buffer, "String"));
            }

            return XR_SUCCESS;
        }

        XrResult xrCreateActionSet(XrInstance instance,
                                   const XrActionSetCreateInfo* createInfo,
                                   XrActionSet* actionSet) override {
            if (createInfo->type != XR_TYPE_ACTION_SET_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateActionSet",
                              TLPArg(instance, "Instance"),
                              TLArg(createInfo->actionSetName, "Name"),
                              TLArg(createInfo->localizedActionSetName, "LocalizedName"),
                              TLArg(xr::ToCString(createInfo->type), "Type"),
                              TLArg(createInfo->priority, "Priority"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We don't support action sets. Return a non-null handle to make the application happy.
            *actionSet = (XrActionSet)1;

            TraceLoggingWrite(g_traceProvider, "xrCreateActionSet", TLPArg(*actionSet, "ActionSet"));

            return XR_SUCCESS;
        }

        XrResult xrDestroyActionSet(XrActionSet actionSet) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet", TLPArg(actionSet, "ActionSet"));

            if (actionSet != (XrActionSet)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            return XR_SUCCESS;
        }

        XrResult xrCreateAction(XrActionSet actionSet,
                                const XrActionCreateInfo* createInfo,
                                XrAction* action) override {
            if (createInfo->type != XR_TYPE_ACTION_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateAction",
                              TLPArg(actionSet, "ActionSet"),
                              TLArg(createInfo->actionName, "Name"),
                              TLArg(createInfo->localizedActionName, "LocalizedName"),
                              TLArg(xr::ToCString(createInfo->actionType), "Type"));
            for (uint32_t i = 0; i < createInfo->countSubactionPaths; i++) {
                TraceLoggingWrite(g_traceProvider,
                                  "xrCreateAction",
                                  TLArg(getXrPath(createInfo->subactionPaths[i]).c_str(), "SubactionPath"));
            }

            if (actionSet != (XrActionSet)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We don't support actions. Return a non-null handle to make the application happy.
            *action = (XrAction)1;

            TraceLoggingWrite(g_traceProvider, "xrCreateAction", TLPArg(*action, "Action"));

            return XR_SUCCESS;
        }

        XrResult xrDestroyAction(XrAction action) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroyAction", TLPArg(action, "Action"));

            if (action != (XrAction)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            return XR_SUCCESS;
        }

        XrResult xrCreateActionSpace(XrSession session,
                                     const XrActionSpaceCreateInfo* createInfo,
                                     XrSpace* space) override {
            if (createInfo->type != XR_TYPE_ACTION_SPACE_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateActionSpace",
                              TLPArg(session, "Session"),
                              TLPArg(createInfo->action, "Action"),
                              TLArg(getXrPath(createInfo->subactionPath).c_str(), "SubactionPath"),
                              TLArg(xr::ToString(createInfo->poseInActionSpace).c_str(), "PoseInActionSpace"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            // We don't support action spaces. Return a non-null handle to make the application happy.
            *space = (XrSpace)1;

            TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace", TLPArg(*space, "Space"));

            return XR_SUCCESS;
        }

        XrResult xrSuggestInteractionProfileBindings(
            XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) override {
            if (suggestedBindings->type != XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrSuggestInteractionProfileBindings",
                              TLPArg(instance, "Instance"),
                              TLArg(getXrPath(suggestedBindings->interactionProfile).c_str(), "interactionProfile"));

            if (!m_instanceCreated || instance != (XrInstance)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
                TraceLoggingWrite(g_traceProvider,
                                  "xrSuggestInteractionProfileBindings",
                                  TLPArg(suggestedBindings->suggestedBindings[i].action, "Action"),
                                  TLArg(getXrPath(suggestedBindings->suggestedBindings[i].binding).c_str(), "Path"));
            }

            return XR_SUCCESS;
        }

        XrResult xrAttachSessionActionSets(XrSession session,
                                           const XrSessionActionSetsAttachInfo* attachInfo) override {
            if (attachInfo->type != XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets", TLPArg(session, "Session"));
            for (uint32_t i = 0; i < attachInfo->countActionSets; i++) {
                TraceLoggingWrite(
                    g_traceProvider, "xrAttachSessionActionSets", TLPArg(attachInfo->actionSets[i], "ActionSet"));
            }

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            return XR_SUCCESS;
        }

        XrResult xrGetCurrentInteractionProfile(XrSession session,
                                                XrPath topLevelUserPath,
                                                XrInteractionProfileState* interactionProfile) override {
            if (interactionProfile->type != XR_TYPE_INTERACTION_PROFILE_STATE) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetCurrentInteractionProfile",
                              TLPArg(session, "Session"),
                              TLArg(getXrPath(topLevelUserPath).c_str(), "TopLevelUserPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            CHECK_XRCMD(xrStringToPath(XR_NULL_HANDLE,
                                       "/interaction_profiles/khr/simple_controller",
                                       &interactionProfile->interactionProfile));

            TraceLoggingWrite(g_traceProvider,
                              "xrGetCurrentInteractionProfile",
                              TLArg(getXrPath(interactionProfile->interactionProfile).c_str(), "InteractionProfile"));

            return XR_SUCCESS;
        }

        XrResult xrGetActionStateBoolean(XrSession session,
                                         const XrActionStateGetInfo* getInfo,
                                         XrActionStateBoolean* state) override {
            if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_BOOLEAN) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetActionStateBoolean",
                              TLPArg(session, "Session"),
                              TLPArg(getInfo->action, "Action"),
                              TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            state->isActive = XR_FALSE;

            TraceLoggingWrite(g_traceProvider, "xrGetActionStateBoolean", TLArg(state->isActive, "Active"));

            return XR_SUCCESS;
        }

        XrResult xrGetActionStateFloat(XrSession session,
                                       const XrActionStateGetInfo* getInfo,
                                       XrActionStateFloat* state) override {
            if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_FLOAT) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetActionStateFloat",
                              TLPArg(session, "Session"),
                              TLPArg(getInfo->action, "Action"),
                              TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            state->isActive = XR_FALSE;

            TraceLoggingWrite(g_traceProvider, "xrGetActionStateFloat", TLArg(state->isActive, "Active"));

            return XR_SUCCESS;
        }

        XrResult xrGetActionStateVector2f(XrSession session,
                                          const XrActionStateGetInfo* getInfo,
                                          XrActionStateVector2f* state) override {
            if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_VECTOR2F) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetActionStateVector2f",
                              TLPArg(session, "Session"),
                              TLPArg(getInfo->action, "Action"),
                              TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            state->isActive = XR_FALSE;

            TraceLoggingWrite(g_traceProvider, "xrGetActionStateVector2f", TLArg(state->isActive, "Active"));

            return XR_SUCCESS;
        }

        XrResult xrGetActionStatePose(XrSession session,
                                      const XrActionStateGetInfo* getInfo,
                                      XrActionStatePose* state) override {
            if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_POSE) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetActionStatePose",
                              TLPArg(session, "Session"),
                              TLPArg(getInfo->action, "Action"),
                              TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            state->isActive = XR_TRUE;

            TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose", TLArg(state->isActive, "Active"));

            return XR_SUCCESS;
        }

        XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) override {
            if (syncInfo->type != XR_TYPE_ACTIONS_SYNC_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider, "xrSyncActions", TLPArg(session, "Session"));
            for (uint32_t i = 0; i < syncInfo->countActiveActionSets; i++) {
                TraceLoggingWrite(g_traceProvider,
                                  "xrSyncActions",
                                  TLPArg(syncInfo->activeActionSets[i].actionSet, "ActionSet"),
                                  TLArg(syncInfo->activeActionSets[i].subactionPath, "SubactionPath"));
            }

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            return XR_SUCCESS;
        }

        XrResult xrEnumerateBoundSourcesForAction(XrSession session,
                                                  const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
                                                  uint32_t sourceCapacityInput,
                                                  uint32_t* sourceCountOutput,
                                                  XrPath* sources) override {
            if (enumerateInfo->type != XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateBoundSourcesForAction",
                              TLPArg(session, "Session"),
                              TLPArg(enumerateInfo->action, "Action"),
                              TLArg(sourceCapacityInput, "SourceCapacityInput"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            *sourceCountOutput = 0;
            TraceLoggingWrite(
                g_traceProvider, "xrEnumerateBoundSourcesForAction", TLArg(*sourceCountOutput, "SourceCountOutput"));

            return XR_SUCCESS;
        }

        XrResult xrGetInputSourceLocalizedName(XrSession session,
                                               const XrInputSourceLocalizedNameGetInfo* getInfo,
                                               uint32_t bufferCapacityInput,
                                               uint32_t* bufferCountOutput,
                                               char* buffer) override {
            if (getInfo->type != XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetInputSourceLocalizedName",
                              TLPArg(session, "Session"),
                              TLArg(getXrPath(getInfo->sourcePath).c_str(), "SourcePath"),
                              TLArg(getInfo->whichComponents, "WhichComponents"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            *bufferCountOutput = 0;
            TraceLoggingWrite(
                g_traceProvider, "xrGetInputSourceLocalizedName", TLArg(*bufferCountOutput, "BufferCountOutput"));

            return XR_SUCCESS;
        }

        XrResult xrApplyHapticFeedback(XrSession session,
                                       const XrHapticActionInfo* hapticActionInfo,
                                       const XrHapticBaseHeader* hapticFeedback) override {
            if (hapticActionInfo->type != XR_TYPE_HAPTIC_ACTION_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrApplyHapticFeedback",
                              TLPArg(session, "Session"),
                              TLPArg(hapticActionInfo->action, "Action"),
                              TLArg(getXrPath(hapticActionInfo->subactionPath).c_str(), "SubactionPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            return XR_SUCCESS;
        }

        XrResult xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo) override {
            if (hapticActionInfo->type != XR_TYPE_HAPTIC_ACTION_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrStopHapticFeedback",
                              TLPArg(session, "Session"),
                              TLPArg(hapticActionInfo->action, "Action"),
                              TLArg(getXrPath(hapticActionInfo->subactionPath).c_str(), "SubactionPath"));

            if (!m_sessionCreated || session != (XrSession)1) {
                return XR_ERROR_HANDLE_INVALID;
            }

            return XR_SUCCESS;
        }

      private:
        struct Swapchain {
            std::vector<pvrTextureSwapChain> pvrSwapchain;
            std::vector<std::vector<ID3D11Texture2D*>> images;
            pvrTextureSwapChainDesc pvrDesc;
        };

        struct Space {
            XrReferenceSpaceType referenceType;
            XrPosef poseInSpace;
        };

        void prepareAndCommitSwapchainImage(Swapchain& xrSwapchain,
                                            uint32_t slice,
                                            std::set<std::pair<pvrTextureSwapChain, uint32_t>>& committed) const {
            // If the texture was already committed, do nothing.
            if (committed.count(std::make_pair(xrSwapchain.pvrSwapchain[0], slice))) {
                return;
            }

            // PVR does not seem to support texture arrays and always reads from the first slice. We must do a copy to
            // slice 0 into another swapchain.
            if (slice != 0) {
                // Lazily create a second swapchain for this index.
                if (!xrSwapchain.pvrSwapchain[slice]) {
                    auto desc = xrSwapchain.pvrDesc;
                    desc.ArraySize = 1;
                    CHECK_PVRCMD(pvr_createTextureSwapChainDX(
                        m_pvrSession, m_d3d11Device.Get(), &desc, &xrSwapchain.pvrSwapchain[slice]));

                    int count = -1;
                    CHECK_PVRCMD(pvr_getTextureSwapChainLength(m_pvrSession, xrSwapchain.pvrSwapchain[slice], &count));
                    if (count != xrSwapchain.images[0].size()) {
                        throw std::runtime_error("Swapchain image count mismatch");
                    }

                    // FIXME: Here we assume D3D11 since it's the only API supported for now.

                    // Query the textures for the swapchain.
                    for (int i = 0; i < count; i++) {
                        ID3D11Texture2D* texture = nullptr;
                        CHECK_PVRCMD(pvr_getTextureSwapChainBufferDX(
                            m_pvrSession, xrSwapchain.pvrSwapchain[slice], i, IID_PPV_ARGS(&texture)));

                        xrSwapchain.images[slice].push_back(texture);
                    }
                }

                // Copy from the texture array.
                int pvrSourceIndex = -1;
                CHECK_PVRCMD(
                    pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain.pvrSwapchain[0], &pvrSourceIndex));
                int pvrDestIndex = -1;
                CHECK_PVRCMD(
                    pvr_getTextureSwapChainCurrentIndex(m_pvrSession, xrSwapchain.pvrSwapchain[slice], &pvrDestIndex));

                m_d3d11DeviceContext->CopySubresourceRegion(xrSwapchain.images[slice][pvrDestIndex],
                                                            0,
                                                            0,
                                                            0,
                                                            0,
                                                            xrSwapchain.images[0][pvrSourceIndex],
                                                            slice,
                                                            nullptr);
            }

            // Commit the texture to PVR.
            CHECK_PVRCMD(pvr_commitTextureSwapChain(m_pvrSession, xrSwapchain.pvrSwapchain[slice]));
            committed.insert(std::make_pair(xrSwapchain.pvrSwapchain[0], slice));
        }

        void convertSteamVRToOpenXRHiddenMesh(const pvrFovPort& fov,
                                              XrVector2f* vertices,
                                              uint32_t* indices,
                                              uint32_t count) const {
            const float b = -fov.DownTan;
            const float t = fov.UpTan;
            const float l = -fov.LeftTan;
            const float r = fov.RightTan;

            // z = -1, n = 1
            // pndcx = (2n/(r-l) * pvx - (r+l)/(r-l)) / -z => pvx = (pndcx + (r+l)/(r-l))/(2n/(r-l))
            // pndcy = (2n/(t-b) * pvy - (t+b)/(t-b)) / -z => pvy = (pndcy + (t+b)/(t-b))/(2n/(t-b))
            const float hSpanRcp = 1.0f / (r - l);
            const float vSpanRcp = 1.0f / (t - b);

            // (r+l)/(r-l)
            const float rplOverHSpan = (r + l) * hSpanRcp;
            const float tpbOverVSpan = (t + b) * vSpanRcp;

            const float halfHSpan = (r - l) * 0.5f;
            const float halfVSpan = (t - b) * 0.5f;

            // constTerm = (r+l)/(r-l)/(2n(r-l))
            const float hConstTerm = rplOverHSpan * halfHSpan;
            const float vConstTerm = tpbOverVSpan * halfVSpan;

            for (uint32_t i = 0; i < count; i++) {
                // Screen to NDC.
                XrVector2f ndc{(vertices[i].x - 0.5f) * 2.f, (vertices[i].y - 0.5f) * 2.f};

                // Project the vertex.
                XMStoreFloat2(reinterpret_cast<XMFLOAT2*>(&vertices[i]),
                              XMVectorMultiplyAdd(XMVECTORF32{{{ndc.x, ndc.y, 0.f, 0.f}}},
                                                  XMVECTORF32{{{halfHSpan, halfVSpan, 0.f, 0.f}}},
                                                  XMVECTORF32{{{hConstTerm, vConstTerm, 0.f, 0.f}}}));

                // Record the indices.
                indices[i] = i;
            }
        }

        std::string getXrPath(XrPath path) const {
            if (path == XR_NULL_PATH) {
                return "";
            }

            const auto it = m_strings.find(path);
            if (it == m_strings.cend()) {
                return "<unknown>";
            }

            return it->second;
        }

        // Instance & PVR state.
        pvrEnvHandle m_pvr;
        pvrSessionHandle m_pvrSession{nullptr};
        bool m_instanceCreated{false};
        bool m_systemCreated{false};
        bool m_isD3D11Supported{false};
        bool m_graphicsRequirementQueried{false};
        LUID m_adapterLuid{};
        double m_frameDuration{0};
        pvrEyeRenderInfo m_cachedEyeInfo[xr::StereoView::Count];
        LARGE_INTEGER m_qpcFrequency;
        double m_pvrTimeFromQpcTimeOffset{0};
        XrPath m_stringIndex{0};
        std::map<XrPath, std::string> m_strings;

        // Session state.
        ComPtr<ID3D11Device> m_d3d11Device;
        ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
        bool m_sessionCreated{false};
        XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
        bool m_sessionStateDirty{false};
        double m_sessionStateEventTime{0.0};
        std::set<XrSwapchain> m_swapchains;
        std::set<XrSpace> m_spaces;
        XrSpace m_originSpace{XR_NULL_HANDLE};
        XrSpace m_viewSpace{XR_NULL_HANDLE};

        // Frame state.
        std::mutex m_frameLock;
        std::condition_variable m_frameCondVar;
        bool m_frameWaited{false};
        bool m_frameBegun{false};
        long long m_nextFrameIndex{0};
        long long m_currentFrameIndex;
        std::optional<double> m_lastFrameWaitedTime;

        static XrResult _xrGetD3D11GraphicsRequirementsKHR(XrInstance instance,
                                                           XrSystemId systemId,
                                                           XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
            DebugLog("--> xrGetD3D11GraphicsRequirementsKHR\n");

            XrResult result;
            try {
                result = dynamic_cast<OpenXrRuntime*>(GetInstance())
                             ->xrGetD3D11GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
            } catch (std::exception& exc) {
                Log("xrGetD3D11GraphicsRequirementsKHR: %s\n", exc.what());
                result = XR_ERROR_RUNTIME_FAILURE;
            }

            DebugLog("<-- xrGetD3D11GraphicsRequirementsKHR %s\n", xr::ToCString(result));

            return result;
        }

        static XrResult _xrConvertWin32PerformanceCounterToTimeKHR(XrInstance instance,
                                                                   const LARGE_INTEGER* performanceCounter,
                                                                   XrTime* time) {
            DebugLog("--> xrConvertWin32PerformanceCounterToTimeKHR\n");

            XrResult result;
            try {
                result = dynamic_cast<OpenXrRuntime*>(GetInstance())
                             ->xrConvertWin32PerformanceCounterToTimeKHR(instance, performanceCounter, time);
            } catch (std::exception& exc) {
                Log("xrConvertWin32PerformanceCounterToTimeKHR: %s\n", exc.what());
                result = XR_ERROR_RUNTIME_FAILURE;
            }

            DebugLog("<-- xrConvertWin32PerformanceCounterToTimeKHR %s\n", xr::ToCString(result));

            return result;
        }

        static XrResult _xrConvertTimeToWin32PerformanceCounterKHR(XrInstance instance,
                                                                   XrTime time,
                                                                   LARGE_INTEGER* performanceCounter) {
            DebugLog("--> xrConvertTimeToWin32PerformanceCounterKHR\n");

            XrResult result;
            try {
                result = dynamic_cast<OpenXrRuntime*>(GetInstance())
                             ->xrConvertTimeToWin32PerformanceCounterKHR(instance, time, performanceCounter);
            } catch (std::exception& exc) {
                Log("xrConvertTimeToWin32PerformanceCounterKHR: %s\n", exc.what());
                result = XR_ERROR_RUNTIME_FAILURE;
            }

            DebugLog("<-- xrConvertTimeToWin32PerformanceCounterKHR %s\n", xr::ToCString(result));

            return result;
        }

        static XrResult _xrGetVisibilityMaskKHR(XrSession session,
                                                XrViewConfigurationType viewConfigurationType,
                                                uint32_t viewIndex,
                                                XrVisibilityMaskTypeKHR visibilityMaskType,
                                                XrVisibilityMaskKHR* visibilityMask) {
            DebugLog("--> xrGetVisibilityMaskKHR\n");

            XrResult result;
            try {
                result = dynamic_cast<OpenXrRuntime*>(GetInstance())
                             ->xrGetVisibilityMaskKHR(
                                 session, viewConfigurationType, viewIndex, visibilityMaskType, visibilityMask);
            } catch (std::exception& exc) {
                Log("xrGetVisibilityMaskKHR: %s\n", exc.what());
                result = XR_ERROR_RUNTIME_FAILURE;
            }

            DebugLog("<-- xrGetVisibilityMaskKHR %s\n", xr::ToCString(result));

            return result;
        }
    };

    std::unique_ptr<OpenXrRuntime> g_instance = nullptr;

} // namespace

namespace pimax_openxr {
    OpenXrApi* GetInstance() {
        if (!g_instance) {
            g_instance = std::make_unique<OpenXrRuntime>();
        }
        return g_instance.get();
    }

    void ResetInstance() {
        g_instance.reset();
    }

} // namespace pimax_openxr

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        TraceLoggingRegister(g_traceProvider);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
