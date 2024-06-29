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

#pragma once

#include "pch.h"

#include "BodyState.h"

#define CHECK_OVRCMD(cmd) xr::detail::_CheckOVRResult(cmd, #cmd, FILE_AND_LINE)
#define CHECK_VKCMD(cmd) xr::detail::_CheckVKResult(cmd, #cmd, FILE_AND_LINE)

namespace xr {
    static inline std::string ToString(XrVersion version) {
        return fmt::format("{}.{}.{}", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));
    }

    static inline std::string ToString(const ovrPosef& pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.Position.x,
                           pose.Position.y,
                           pose.Position.z,
                           pose.Orientation.x,
                           pose.Orientation.y,
                           pose.Orientation.z,
                           pose.Orientation.w);
    }

    static inline std::string ToString(const XrPosef& pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.position.x,
                           pose.position.y,
                           pose.position.z,
                           pose.orientation.x,
                           pose.orientation.y,
                           pose.orientation.z,
                           pose.orientation.w);
    }

    static inline std::string ToString(const XrQuaternionf& quaternion) {
        return fmt::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", quaternion.x, quaternion.y, quaternion.z, quaternion.w);
    }

    static inline std::string ToString(const ovrVector3f& vec) {
        return fmt::format("({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z);
    }

    static inline std::string ToString(const XrVector3f& vec) {
        return fmt::format("({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z);
    }

    static inline std::string ToString(const ovrVector2f& vec) {
        return fmt::format("({:.3f}, {:.3f})", vec.x, vec.y);
    }

    static inline std::string ToString(const XrVector2f& vec) {
        return fmt::format("({:.3f}, {:.3f})", vec.x, vec.y);
    }

    static inline std::string ToString(const XrFovf& fov) {
        return fmt::format(
            "(l:{:.3f}, r:{:.3f}, u:{:.3f}, d:{:.3f})", fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown);
    }

    static inline std::string ToString(const XrRect2Di& rect) {
        return fmt::format("x:{}, y:{} w:{} h:{}", rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
    }

    static inline std::string ToString(const virtualdesktop_openxr::BodyTracking::Pose& pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.position.x,
                           pose.position.y,
                           pose.position.z,
                           pose.orientation.x,
                           pose.orientation.y,
                           pose.orientation.z,
                           pose.orientation.w);
    }

    namespace math {

        namespace Pose {

            static inline XrPosef Orientation(const XrVector3f& anglesInRadians) {
                XrPosef pose = Identity();
                StoreXrQuaternion(
                    &pose.orientation,
                    DirectX::XMQuaternionRotationRollPitchYaw(anglesInRadians.x, anglesInRadians.y, anglesInRadians.z));
                return pose;
            }

            static inline XrPosef MakePose(const XrVector3f& translation, const XrVector3f& anglesInRadians) {
                XrPosef pose{{}, translation};
                StoreXrQuaternion(
                    &pose.orientation,
                    DirectX::XMQuaternionRotationRollPitchYaw(anglesInRadians.x, anglesInRadians.y, anglesInRadians.z));
                return pose;
            }

            static inline bool Equals(const XrPosef& a, const XrPosef& b) {
                return std::abs(b.position.x - a.position.x) < 0.00001f &&
                       std::abs(b.position.y - a.position.y) < 0.00001f &&
                       std::abs(b.position.z - a.position.z) < 0.00001f &&
                       std::abs(b.orientation.x - a.orientation.x) < 0.00001f &&
                       std::abs(b.orientation.y - a.orientation.y) < 0.00001f &&
                       std::abs(b.orientation.z - a.orientation.z) < 0.00001f &&
                       std::abs(b.orientation.w - a.orientation.w) < 0.00001f;
            }

        } // namespace Pose

        static XrFovf ComputeBoundingFov(const XrFovf& fullFov, const XrVector2f& min, const XrVector2f& max) {
            const float width = std::max(0.01f, max.x - min.x);
            const float height = std::max(0.01f, max.y - min.y);
            const XrVector2f center = (min + max) / 2.f;

            const auto fullProjection = ComposeProjectionMatrix(fullFov, {0.001f, 100.f});
            // clang-format off
            const auto boundingFov =
                DirectX::XMMATRIX(2.f / width,                0.f,                          0.f, 0.f,
                                  0.f,                        2.f / height,                 0.f, 0.f,
                                  0.f,                        0.f,                          1.f, 0.f,
                                  -(2.f * center.x) / width,  1*-(2.f * center.y) / height, 0.f, 1.f);
            // clang-format on
            DirectX::XMFLOAT4X4 projection;
            DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixMultiply(fullProjection, boundingFov));
            XrFovf fov = DecomposeProjectionMatrix(projection);
            fov.angleLeft = std::clamp(fov.angleLeft, fullFov.angleLeft, fullFov.angleRight);
            fov.angleRight = std::clamp(fov.angleRight, fullFov.angleLeft, fullFov.angleRight);
            fov.angleUp = std::clamp(fov.angleUp, fullFov.angleDown, fullFov.angleUp);
            fov.angleDown = std::clamp(fov.angleDown, fullFov.angleDown, fullFov.angleUp);
            return fov;
        }

        static bool ProjectPoint(const XrView& eyeInViewSpace,
                                 const XrVector3f& forward,
                                 XrVector2f& projectedPosition) {
            // 1) Compute the view space to camera transform for this eye.
            const auto cameraProjection = ComposeProjectionMatrix(eyeInViewSpace.fov, {0.001f, 100.f});
            const auto cameraView = LoadXrPose(eyeInViewSpace.pose);
            const auto viewToCamera = DirectX::XMMatrixMultiply(cameraProjection, cameraView);

            // 2) Transform the 3D point to camera space.
            const auto projectedInCameraSpace =
                DirectX::XMVector3Transform(DirectX::XMVectorSet(forward.x, forward.y, forward.z, 1.f), viewToCamera);

            // 3) Project the 3D point in camera space to a 2D point in normalized device coordinates.
            // 4) Output NDC (-1,+1)
            XrVector4f point;
            xr::math::StoreXrVector4(&point, projectedInCameraSpace);
            if (std::abs(point.w) < FLT_EPSILON) {
                return false;
            }
            projectedPosition.x = point.x / point.w;
            projectedPosition.y = point.y / point.w;

            return true;
        }

    } // namespace math

    namespace detail {

        [[noreturn]] static inline void _ThrowOVRResult(ovrResult ovr,
                                                        const char* originator = nullptr,
                                                        const char* sourceLocation = nullptr) {
            xr::detail::_Throw(xr::detail::_Fmt("ovrResult failure [%d]", ovr), originator, sourceLocation);
        }

        static inline HRESULT _CheckOVRResult(ovrResult ovr,
                                              const char* originator = nullptr,
                                              const char* sourceLocation = nullptr) {
            if (OVR_FAILURE(ovr)) {
                xr::detail::_ThrowOVRResult(ovr, originator, sourceLocation);
            }

            return ovr;
        }

        [[noreturn]] static inline void _ThrowVKResult(VkResult vks,
                                                       const char* originator = nullptr,
                                                       const char* sourceLocation = nullptr) {
            xr::detail::_Throw(xr::detail::_Fmt("VkStatus failure [%d]", vks), originator, sourceLocation);
        }

        static inline HRESULT _CheckVKResult(VkResult vks,
                                             const char* originator = nullptr,
                                             const char* sourceLocation = nullptr) {
            if ((vks) != VK_SUCCESS) {
                xr::detail::_ThrowVKResult(vks, originator, sourceLocation);
            }

            return vks;
        }
    } // namespace detail

    namespace QuadView {
        constexpr uint32_t Left = 0;
        constexpr uint32_t Right = 1;
        constexpr uint32_t FocusLeft = 2;
        constexpr uint32_t FocusRight = 3;
        constexpr uint32_t Count = 4;
    } // namespace QuadView

} // namespace xr

namespace virtualdesktop_openxr::utils {

    namespace {

        extern "C" NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution,
                                                                BOOLEAN SetResolution,
                                                                PULONG CurrentResolution);
        extern "C" NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution,
                                                                  PULONG MaximumResolution,
                                                                  PULONG CurrentResolution);
    } // namespace

    static void InitializeHighPrecisionTimer() {
        // https://stackoverflow.com/questions/3141556/how-to-setup-timer-resolution-to-0-5-ms
        ULONG min, max, current;
        NtQueryTimerResolution(&min, &max, &current);

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
    static bool IsServiceRunning(const std::wstring_view& name) {
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

    // A generic timer.
    struct ITimer {
        virtual ~ITimer() = default;

        virtual void start() = 0;
        virtual void stop() = 0;

        virtual uint64_t query(bool reset = true) const = 0;
    };

    // A synchronous CPU timer.
    class CpuTimer : public ITimer {
        using clock = std::chrono::high_resolution_clock;

      public:
        void start() override {
            m_timeStart = clock::now();
        }

        void stop() override {
            m_duration += clock::now() - m_timeStart;
        }

        uint64_t query(bool reset = true) const override {
            const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_duration);
            if (reset)
                m_duration = clock::duration::zero();
            return duration.count();
        }

      private:
        clock::time_point m_timeStart;
        mutable clock::duration m_duration{0};
    };

    // API dispatch table for Vulkan.
    struct VulkanDispatch {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{nullptr};

        PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2{nullptr};
        PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{nullptr};
        PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR{nullptr};
        PFN_vkGetDeviceQueue vkGetDeviceQueue{nullptr};
        PFN_vkQueueSubmit vkQueueSubmit{nullptr};
        PFN_vkCreateImage vkCreateImage{nullptr};
        PFN_vkDestroyImage vkDestroyImage{nullptr};
        PFN_vkAllocateMemory vkAllocateMemory{nullptr};
        PFN_vkFreeMemory vkFreeMemory{nullptr};
        PFN_vkCreateCommandPool vkCreateCommandPool{nullptr};
        PFN_vkDestroyCommandPool vkDestroyCommandPool{nullptr};
        PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers{nullptr};
        PFN_vkFreeCommandBuffers vkFreeCommandBuffers{nullptr};
        PFN_vkResetCommandBuffer vkResetCommandBuffer{nullptr};
        PFN_vkBeginCommandBuffer vkBeginCommandBuffer{nullptr};
        PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier{nullptr};
        PFN_vkCmdResetQueryPool vkCmdResetQueryPool{nullptr};
        PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp{nullptr};
        PFN_vkEndCommandBuffer vkEndCommandBuffer{nullptr};
        PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR{nullptr};
        PFN_vkBindImageMemory vkBindImageMemory{nullptr};
        PFN_vkCreateSemaphore vkCreateSemaphore{nullptr};
        PFN_vkDestroySemaphore vkDestroySemaphore{nullptr};
        PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR{nullptr};
        PFN_vkCreateFence vkCreateFence{nullptr};
        PFN_vkDestroyFence vkDestroyFence{nullptr};
        PFN_vkResetFences vkResetFences{nullptr};
        PFN_vkWaitForFences vkWaitForFences{nullptr};
        PFN_vkDeviceWaitIdle vkDeviceWaitIdle{nullptr};
        PFN_vkCreateQueryPool vkCreateQueryPool{nullptr};
        PFN_vkDestroyQueryPool vkDestroyQueryPool{nullptr};
        PFN_vkGetQueryPoolResults vkGetQueryPoolResults{nullptr};
    };

    // API dispatch table for OpenGL.
    struct GlDispatch {
        PFNGLGETUNSIGNEDBYTEVEXTPROC glGetUnsignedBytevEXT{nullptr};
        PFNGLCREATETEXTURESPROC glCreateTextures{nullptr};
        PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT{nullptr};
        PFNGLDELETEMEMORYOBJECTSEXTPROC glDeleteMemoryObjectsEXT{nullptr};
        PFNGLTEXTURESTORAGEMEM2DEXTPROC glTextureStorageMem2DEXT{nullptr};
        PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC glTextureStorageMem2DMultisampleEXT{nullptr};
        PFNGLTEXTURESTORAGEMEM3DEXTPROC glTextureStorageMem3DEXT{nullptr};
        PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC glTextureStorageMem3DMultisampleEXT{nullptr};
        PFNGLGENSEMAPHORESEXTPROC glGenSemaphoresEXT{nullptr};
        PFNGLDELETESEMAPHORESEXTPROC glDeleteSemaphoresEXT{nullptr};
        PFNGLSEMAPHOREPARAMETERUI64VEXTPROC glSemaphoreParameterui64vEXT{nullptr};
        PFNGLSIGNALSEMAPHOREEXTPROC glSignalSemaphoreEXT{nullptr};
        PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC glImportMemoryWin32HandleEXT{nullptr};
        PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC glImportSemaphoreWin32HandleEXT{nullptr};
        PFNGLGENQUERIESPROC glGenQueries{nullptr};
        PFNGLDELETEQUERIESPROC glDeleteQueries{nullptr};
        PFNGLQUERYCOUNTERPROC glQueryCounter{nullptr};
        PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv{nullptr};
        PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v{nullptr};

#ifdef _DEBUG
        PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback{nullptr};
#endif
    };

    struct GlContext {
        HDC glDC;
        HGLRC glRC;

        bool valid{false};
    };

    class GlContextSwitch {
      public:
        GlContextSwitch(const GlContext& context, bool ignoreErrors = false)
            : m_valid(context.valid), m_ignoreErrors(ignoreErrors) {
            if (m_valid) {
                m_glDC = wglGetCurrentDC();
                m_glRC = wglGetCurrentContext();

                wglMakeCurrent(context.glDC, context.glRC);

                if (!m_ignoreErrors) {
                    // Reset error codes.
                    while (glGetError() != GL_NO_ERROR)
                        ;
                }
            }
        }

        ~GlContextSwitch() noexcept(false) {
            if (m_valid) {
                const auto error = glGetError();

                wglMakeCurrent(m_glDC, m_glRC);

                if (!m_ignoreErrors) {
                    CHECK_MSG(error == GL_NO_ERROR, fmt::format("OpenGL error: 0x{:x}", error));
                }
            }
        }

      private:
        const bool m_valid;
        const bool m_ignoreErrors;
        HDC m_glDC;
        HGLRC m_glRC;
    };

    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2017/may/c-use-modern-c-to-access-the-windows-registry
    static std::optional<int> RegGetDword(HKEY hKey, const std::string& subKey, const std::string& value) {
        DWORD data{};
        DWORD dataSize = sizeof(data);
        LONG retCode = ::RegGetValue(hKey,
                                     xr::utf8_to_wide(subKey).c_str(),
                                     xr::utf8_to_wide(value).c_str(),
                                     RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_DWORD,
                                     nullptr,
                                     &data,
                                     &dataSize);
        if (retCode != ERROR_SUCCESS) {
            return {};
        }
        return data;
    }

    static std::optional<std::wstring> RegGetString(HKEY hKey, const std::string& subKey, const std::string& value) {
        DWORD dataSize = 0;
        LONG retCode = ::RegGetValue(hKey,
                                     xr::utf8_to_wide(subKey).c_str(),
                                     xr::utf8_to_wide(value).c_str(),
                                     RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_SZ,
                                     nullptr,
                                     nullptr,
                                     &dataSize);
        if (retCode != ERROR_SUCCESS || !dataSize) {
            return {};
        }

        std::wstring data(dataSize / sizeof(wchar_t), 0);
        retCode = ::RegGetValue(hKey,
                                xr::utf8_to_wide(subKey).c_str(),
                                xr::utf8_to_wide(value).c_str(),
                                RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_SZ,
                                nullptr,
                                data.data(),
                                &dataSize);
        if (retCode != ERROR_SUCCESS) {
            return {};
        }

        return data.substr(0, dataSize / sizeof(wchar_t) - 1);
    }

    static std::vector<const char*> ParseExtensionString(char* names) {
        std::vector<const char*> list;
        while (*names != 0) {
            list.push_back(names);
            while (*(++names) != 0) {
                if (*names == ' ') {
                    *names++ = '\0';
                    break;
                }
            }
        }
        return list;
    }

    static inline XrPosef ovrPoseToXrPose(const ovrPosef& ovrPose) {
        XrPosef xrPose;
        xrPose.position.x = ovrPose.Position.x;
        xrPose.position.y = ovrPose.Position.y;
        xrPose.position.z = ovrPose.Position.z;
        xrPose.orientation.x = ovrPose.Orientation.x;
        xrPose.orientation.y = ovrPose.Orientation.y;
        xrPose.orientation.z = ovrPose.Orientation.z;
        xrPose.orientation.w = ovrPose.Orientation.w;

        return xrPose;
    }

    static inline ovrPosef xrPoseToOvrPose(const XrPosef& xrPose) {
        ovrPosef ovrPose;
        ovrPose.Position.x = xrPose.position.x;
        ovrPose.Position.y = xrPose.position.y;
        ovrPose.Position.z = xrPose.position.z;
        ovrPose.Orientation.x = xrPose.orientation.x;
        ovrPose.Orientation.y = xrPose.orientation.y;
        ovrPose.Orientation.z = xrPose.orientation.z;
        ovrPose.Orientation.w = xrPose.orientation.w;

        return ovrPose;
    }

    static inline XrVector3f ovrVector3fToXrVector3f(const ovrVector3f& ovrVector3f) {
        XrVector3f xrVector3f;
        xrVector3f.x = ovrVector3f.x;
        xrVector3f.y = ovrVector3f.y;
        xrVector3f.z = ovrVector3f.z;

        return xrVector3f;
    }

    static DXGI_FORMAT getTypelessFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_TYPELESS;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_TYPELESS;
        }

        return format;
    }

    static bool isSRGBFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return true;
        }

        return false;
    }

    static DXGI_FORMAT getShaderResourceViewFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        }

        return format;
    }

    static DXGI_FORMAT getUnorderedAccessViewFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        }

        return format;
    }

    static ovrTextureFormat dxgiToOvrTextureFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return OVR_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return OVR_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return OVR_FORMAT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return OVR_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return OVR_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_D16_UNORM:
            return OVR_FORMAT_D16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return OVR_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_D32_FLOAT:
            return OVR_FORMAT_D32_FLOAT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return OVR_FORMAT_D32_FLOAT_S8X24_UINT;
        default:
            return OVR_FORMAT_UNKNOWN;
        }
    }

    static DXGI_FORMAT ovrToDxgiTextureFormat(ovrTextureFormat format) {
        switch (format) {
        case OVR_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case OVR_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case OVR_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case OVR_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case OVR_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        case OVR_FORMAT_B8G8R8X8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case OVR_FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case OVR_FORMAT_D16_UNORM:
            return DXGI_FORMAT_D16_UNORM;
        case OVR_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case OVR_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        case OVR_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    static ovrTextureFormat vkToOvrTextureFormat(VkFormat format) {
        switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return OVR_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return OVR_FORMAT_B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return OVR_FORMAT_R16G16B16A16_FLOAT;
        case VK_FORMAT_D16_UNORM:
            return OVR_FORMAT_D16_UNORM;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return OVR_FORMAT_D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT:
            return OVR_FORMAT_D32_FLOAT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return OVR_FORMAT_D32_FLOAT_S8X24_UINT;
        default:
            return OVR_FORMAT_UNKNOWN;
        }
    }

    static ovrTextureFormat glToOvrTextureFormat(GLenum format) {
        switch (format) {
        case GL_RGBA8:
            return OVR_FORMAT_R8G8B8A8_UNORM;
        case GL_SRGB8_ALPHA8:
            return OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        case GL_RGBA16F:
            return OVR_FORMAT_R16G16B16A16_FLOAT;
        case GL_DEPTH_COMPONENT16:
            return OVR_FORMAT_D16_UNORM;
        case GL_DEPTH24_STENCIL8:
            return OVR_FORMAT_D24_UNORM_S8_UINT;
        case GL_DEPTH_COMPONENT32F:
            return OVR_FORMAT_D32_FLOAT;
        case GL_DEPTH32F_STENCIL8:
            return OVR_FORMAT_D32_FLOAT_S8X24_UINT;
        default:
            return OVR_FORMAT_UNKNOWN;
        }
    }

    static size_t glGetBytePerPixels(GLenum format) {
        switch (format) {
        case GL_DEPTH_COMPONENT16:
            // TODO: This should be 2, but fails with "GL_INVALID_VALUE error generated. Memory object too small".
            return 4;
        case GL_RGBA8:
        case GL_SRGB8_ALPHA8:
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH_COMPONENT32F:
        case GL_R11F_G11F_B10F:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return 4;
        case GL_RGBA16F:
        case GL_DEPTH32F_STENCIL8:
            return 8;
        default:
            return 0;
        }
    }

    static inline bool isValidSwapchainRect(ovrTextureSwapChainDesc desc, const XrRect2Di& rect) {
        if (rect.offset.x < 0 || rect.offset.y < 0 || rect.extent.width <= 0 || rect.extent.height <= 0) {
            return false;
        }

        if (rect.offset.x + rect.extent.width > desc.Width || rect.offset.y + rect.extent.height > desc.Height) {
            return false;
        }

        return true;
    }

    static inline void setDebugName(ID3D11DeviceChild* resource, std::string_view name) {
        if (resource && !name.empty()) {
            resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.data());
        }
    }

    static inline void setDebugName(ID3D12Object* resource, std::string_view name) {
        if (resource && !name.empty()) {
            resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.data());
        }
    }

    static inline bool startsWith(const std::string& str, const std::string& substr) {
        return str.find(substr) == 0;
    }

    static inline bool endsWith(const std::string& str, const std::string& substr) {
        const auto pos = str.find(substr);
        return pos != std::string::npos && pos == str.size() - substr.size();
    }

    #define DEFINE_DETOUR_FUNCTION(ReturnType, FunctionName, ...)                                                          \
    ReturnType (*original_##FunctionName)(##__VA_ARGS__) = nullptr;                                                    \
    ReturnType hooked_##FunctionName(##__VA_ARGS__)

    template <typename TMethod>
    void DetourDllAttach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (original) {
            // Already hooked.
            return;
        }

        HMODULE handle;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN, dll, &handle);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        original = (TMethod)GetProcAddress(handle, target);
        DetourAttach((PVOID*)&original, hooked);

        DetourTransactionCommit();
    }

    template <typename TMethod>
    void DetourDllDetach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (!original) {
            // Not hooked.
            return;
        }

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourDetach((PVOID*)&original, hooked);

        DetourTransactionCommit();

        original = nullptr;
    }

} // namespace virtualdesktop_openxr::utils

#include "gpu_timers.h"
