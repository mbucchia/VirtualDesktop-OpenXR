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

// clang-format off
#define LOG_TELEMETRY_ONCE(call)        \
    {                                   \
        static bool logged = false;     \
        if (!logged) {                  \
            m_telemetry.call;           \
            logged = true;              \
        }                               \
    }
// clang-format on

#define CHECK_PVRCMD(cmd) xr::detail::_CheckPVRResult(cmd, #cmd, FILE_AND_LINE)
#define CHECK_VKCMD(cmd) xr::detail::_CheckVKResult(cmd, #cmd, FILE_AND_LINE)

namespace xr {
    static inline std::string ToString(XrVersion version) {
        return fmt::format("{}.{}.{}", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));
    }

    static inline std::string ToString(pvrPosef pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.Position.x,
                           pose.Position.y,
                           pose.Position.z,
                           pose.Orientation.x,
                           pose.Orientation.y,
                           pose.Orientation.z,
                           pose.Orientation.w);
    }

    static inline std::string ToString(XrPosef pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.position.x,
                           pose.position.y,
                           pose.position.z,
                           pose.orientation.x,
                           pose.orientation.y,
                           pose.orientation.z,
                           pose.orientation.w);
    }

    static inline std::string ToString(pvrVector3f vec) {
        return fmt::format("({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z);
    }

    static inline std::string ToString(XrVector3f vec) {
        return fmt::format("({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z);
    }

    static inline std::string ToString(XrFovf fov) {
        return fmt::format(
            "(l:{:.3f}, r:{:.3f}, u:{:.3f}, d:{:.3f})", fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown);
    }

    static inline std::string ToString(XrRect2Di rect) {
        return fmt::format("x:{}, y:{} w:{} h:{}", rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
    }

    namespace detail {

        [[noreturn]] static inline void _ThrowPVRResult(pvrResult pvr,
                                                        const char* originator = nullptr,
                                                        const char* sourceLocation = nullptr) {
            xr::detail::_Throw(xr::detail::_Fmt("pvrResult failure [%d]", pvr), originator, sourceLocation);
        }

        static inline HRESULT _CheckPVRResult(pvrResult pvr,
                                              const char* originator = nullptr,
                                              const char* sourceLocation = nullptr) {
            if (pvr != pvr_success) {
                xr::detail::_ThrowPVRResult(pvr, originator, sourceLocation);
            }

            return pvr;
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

} // namespace xr

namespace pimax_openxr::utils {

    // A generic timer.
    struct ITimer {
        virtual ~ITimer() = default;

        virtual void start() = 0;
        virtual void stop() = 0;

        virtual uint64_t query(bool reset = true) const = 0;
    };

    // A CPU synchronous timer.
    class CpuTimer : public ITimer {
        using clock = std::chrono::high_resolution_clock;

      public:
        void start() override {
            m_timeStart = clock::now();
        }

        void stop() override {
            m_duration = clock::now() - m_timeStart;
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

    // A GPU asynchronous timer.
    struct GpuTimer : public ITimer {
        GpuTimer(ID3D11Device* device, ID3D11DeviceContext* context) : m_context(context) {
            D3D11_QUERY_DESC queryDesc;
            ZeroMemory(&queryDesc, sizeof(D3D11_QUERY_DESC));
            queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
            CHECK_HRCMD(device->CreateQuery(&queryDesc, m_timeStampDis.ReleaseAndGetAddressOf()));
            queryDesc.Query = D3D11_QUERY_TIMESTAMP;
            CHECK_HRCMD(device->CreateQuery(&queryDesc, m_timeStampStart.ReleaseAndGetAddressOf()));
            CHECK_HRCMD(device->CreateQuery(&queryDesc, m_timeStampEnd.ReleaseAndGetAddressOf()));
        }

        void start() override {
            m_context->Begin(m_timeStampDis.Get());
            m_context->End(m_timeStampStart.Get());
        }

        void stop() override {
            m_context->End(m_timeStampEnd.Get());
            m_context->End(m_timeStampDis.Get());
            m_valid = true;
        }

        uint64_t query(bool reset = true) const override {
            uint64_t duration = 0;
            if (m_valid) {
                UINT64 startime = 0, endtime = 0;
                D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disData = {0};

                if (m_context->GetData(m_timeStampStart.Get(), &startime, sizeof(UINT64), 0) == S_OK &&
                    m_context->GetData(m_timeStampEnd.Get(), &endtime, sizeof(UINT64), 0) == S_OK &&
                    m_context->GetData(
                        m_timeStampDis.Get(), &disData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) == S_OK &&
                    !disData.Disjoint) {
                    duration = static_cast<uint64_t>(((endtime - startime) * 1e6) / disData.Frequency);
                }
                m_valid = !reset;
            }
            return duration;
        }

      private:
        const ComPtr<ID3D11DeviceContext> m_context;
        ComPtr<ID3D11Query> m_timeStampDis;
        ComPtr<ID3D11Query> m_timeStampStart;
        ComPtr<ID3D11Query> m_timeStampEnd;

        // Can the timer be queried (it might still only read 0).
        mutable bool m_valid{false};
    };

    struct GlContext {
        HDC glDC;
        HGLRC glRC;

        bool valid{false};
    };

    class GlContextSwitch {
      public:
        GlContextSwitch(const GlContext& context) : m_valid(context.valid) {
            if (m_valid) {
                m_glDC = wglGetCurrentDC();
                m_glRC = wglGetCurrentContext();

                wglMakeCurrent(context.glDC, context.glRC);

                // Reset error codes.
                (void)glGetError();
            }
        }

        ~GlContextSwitch() noexcept(false) {
            if (m_valid) {
                const auto lastError = glGetError();

                wglMakeCurrent(m_glDC, m_glRC);

                CHECK_MSG(lastError == GL_NO_ERROR, fmt::format("OpenGL error: 0x{:x}", lastError));
            }
        }

      private:
        const bool m_valid;
        HDC m_glDC;
        HGLRC m_glRC;
    };

    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2017/may/c-use-modern-c-to-access-the-windows-registry
    static std::optional<int> RegGetDword(HKEY hKey, const std::string& subKey, const std::string& value) {
        DWORD data{};
        DWORD dataSize = sizeof(data);
        LONG retCode = ::RegGetValue(hKey,
                                     std::wstring(subKey.begin(), subKey.end()).c_str(),
                                     std::wstring(value.begin(), value.end()).c_str(),
                                     RRF_RT_REG_DWORD,
                                     nullptr,
                                     &data,
                                     &dataSize);
        if (retCode != ERROR_SUCCESS) {
            return {};
        }
        return data;
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

    static inline XrTime pvrTimeToXrTime(double pvrTime) {
        return (XrTime)(pvrTime * 1e9);
    }

    static inline double xrTimeToPvrTime(XrTime xrTime) {
        return xrTime / 1e9;
    }

    static inline XrPosef pvrPoseToXrPose(const pvrPosef& pvrPose) {
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

    static inline pvrPosef xrPoseToPvrPose(const XrPosef& xrPose) {
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

    static inline XrVector3f pvrVector3dToXrVector3f(const pvrVector3f& pvrVector3f) {
        XrVector3f xrVector3f;
        xrVector3f.x = pvrVector3f.x;
        xrVector3f.y = pvrVector3f.y;
        xrVector3f.z = pvrVector3f.z;

        return xrVector3f;
    }

    static pvrTextureFormat dxgiToPvrTextureFormat(DXGI_FORMAT format) {
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

    static pvrTextureFormat vkToPvrTextureFormat(VkFormat format) {
        switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return PVR_FORMAT_R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return PVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return PVR_FORMAT_B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return PVR_FORMAT_B8G8R8A8_UNORM_SRGB;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return PVR_FORMAT_R16G16B16A16_FLOAT;
        case VK_FORMAT_D16_UNORM:
            return PVR_FORMAT_D16_UNORM;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return PVR_FORMAT_D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT:
            return PVR_FORMAT_D32_FLOAT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return PVR_FORMAT_D32_FLOAT_S8X24_UINT;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return PVR_FORMAT_BC1_UNORM;
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return PVR_FORMAT_BC1_UNORM_SRGB;
        case VK_FORMAT_BC2_UNORM_BLOCK:
            return PVR_FORMAT_BC2_UNORM;
        case VK_FORMAT_BC2_SRGB_BLOCK:
            return PVR_FORMAT_BC2_UNORM_SRGB;
        case VK_FORMAT_BC3_UNORM_BLOCK:
            return PVR_FORMAT_BC3_UNORM;
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return PVR_FORMAT_BC3_UNORM_SRGB;
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            return PVR_FORMAT_BC6H_UF16;
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            return PVR_FORMAT_BC6H_SF16;
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return PVR_FORMAT_BC7_UNORM;
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return PVR_FORMAT_BC7_UNORM_SRGB;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return PVR_FORMAT_R11G11B10_FLOAT;
        default:
            return PVR_FORMAT_UNKNOWN;
        }
    }

    static pvrTextureFormat glToPvrTextureFormat(GLenum format) {
        switch (format) {
        case GL_RGBA8:
            return PVR_FORMAT_R8G8B8A8_UNORM;
        case GL_SRGB8_ALPHA8:
            return PVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        case GL_RGBA16F:
            return PVR_FORMAT_R16G16B16A16_FLOAT;
        case GL_DEPTH_COMPONENT16:
            return PVR_FORMAT_D16_UNORM;
        case GL_DEPTH24_STENCIL8:
            return PVR_FORMAT_D24_UNORM_S8_UINT;
        case GL_DEPTH_COMPONENT32F:
            return PVR_FORMAT_D32_FLOAT;
        case GL_DEPTH32F_STENCIL8:
            return PVR_FORMAT_D32_FLOAT_S8X24_UINT;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return PVR_FORMAT_BC1_UNORM;
        case GL_R11F_G11F_B10F:
            return PVR_FORMAT_R11G11B10_FLOAT;
        default:
            return PVR_FORMAT_UNKNOWN;
        }
    }

    static size_t glGetBytePerPixels(GLenum format) {
        switch (format) {
        case GL_DEPTH_COMPONENT16:
            return 2;
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

    static inline bool isValidSwapchainRect(pvrTextureSwapChainDesc desc, XrRect2Di rect) {
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
        return str.find(substr) == str.size() - substr.size();
    }

    template <typename TMethod>
    void DetourDllAttach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (original) {
            // Already hooked.
            return;
        }

        HMODULE handle;
        CHECK_MSG(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN, dll, &handle), "Failed to get DLL handle");

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        original = (TMethod)GetProcAddress(handle, target);
        CHECK_MSG(original, "Failed to resolve symbol");
        DetourAttach((PVOID*)&original, hooked);

        CHECK_MSG(DetourTransactionCommit() == NO_ERROR, "Detour failed");
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

        CHECK_MSG(DetourTransactionCommit() == NO_ERROR, "Detour failed");

        original = nullptr;
    }

    // https://stackoverflow.com/questions/60700302/win32-api-to-get-machine-uuid
    static std::string getMachineUuid() {
        typedef struct _dmi_header {
            BYTE type;
            BYTE length;
            WORD handle;
        } dmi_header;

        typedef struct _RawSMBIOSData {
            BYTE Used20CallingMethod;
            BYTE SMBIOSMajorVersion;
            BYTE SMBIOSMinorVersion;
            BYTE DmiRevision;
            DWORD Length;
#pragma warning(push)
#pragma warning(disable : 4200)
            BYTE SMBIOSTableData[];
#pragma warning(pop)
        } RawSMBIOSData;

        DWORD bufsize = 0;
        static BYTE buf[65536] = {0};
        int ret = 0;
        RawSMBIOSData* Smbios;
        dmi_header* h = NULL;
        int flag = 1;

        ret = GetSystemFirmwareTable('RSMB', 0, 0, 0);
        if (!ret) {
            return "";
        }

        bufsize = ret;

        ret = GetSystemFirmwareTable('RSMB', 0, buf, bufsize);
        if (!ret) {
            return "";
        }

        Smbios = (RawSMBIOSData*)buf;
        BYTE* p = Smbios->SMBIOSTableData;

        if (Smbios->Length != bufsize - 8) {
            return "";
        }

        for (DWORD i = 0; i < Smbios->Length; i++) {
            h = (dmi_header*)p;
            if (h->type == 1) {
                const BYTE* p2 = p + 0x8;
                short ver = Smbios->SMBIOSMajorVersion * 0x100 + Smbios->SMBIOSMinorVersion;

                int only0xFF = 1, only0x00 = 1;
                int i;

                for (i = 0; i < 16 && (only0x00 || only0xFF); i++) {
                    if (p2[i] != 0x00)
                        only0x00 = 0;
                    if (p2[i] != 0xFF)
                        only0xFF = 0;
                }

                if (only0xFF) {
                    return "";
                }

                if (only0x00) {
                    return "";
                }

                char buf[128];
                if (ver >= 0x0206) {
                    sprintf_s(buf,
                              sizeof(buf),
                              "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                              p2[3],
                              p2[2],
                              p2[1],
                              p2[0],
                              p2[5],
                              p2[4],
                              p2[7],
                              p2[6],
                              p2[8],
                              p2[9],
                              p2[10],
                              p2[11],
                              p2[12],
                              p2[13],
                              p2[14],
                              p2[15]);
                } else {
                    sprintf_s(buf,
                              sizeof(buf),
                              "-%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                              p2[0],
                              p2[1],
                              p2[2],
                              p2[3],
                              p2[4],
                              p2[5],
                              p2[6],
                              p2[7],
                              p2[8],
                              p2[9],
                              p2[10],
                              p2[11],
                              p2[12],
                              p2[13],
                              p2[14],
                              p2[15]);
                }
                return buf;
            }

            p += h->length;
            while ((*(WORD*)p) != 0) {
                p++;
            }
            p += 2;
        }

        return "";
    }

} // namespace pimax_openxr::utils
