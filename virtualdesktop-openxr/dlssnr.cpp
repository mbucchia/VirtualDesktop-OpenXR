// MIT License
//
// Copyright(c) 2026 Matthieu Bucchianeri
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

#include "DlssnrCacheCS.h"
#include "DlssnrFlowCS.h"
#include "DlssnrLumaCS.h"
#include "DlssnrMotionCS.h"

#define NVSDK_NGX_Parameter_DLSSNR_Width "DLSSNR.Width"
#define NVSDK_NGX_Parameter_DLSSNR_Height "DLSSNR.Height"
#define NVSDK_NGX_Parameter_DLSSNR_Hint_Render_Preset "DLSSNR.Hint.Render.Preset"
#define NVSDK_NGX_Parameter_DLSSNR_Enabled "DLSSNR.Enabled"
#define NVSDK_NGX_Parameter_DLSSNR_DepthInverted "DLSSNR.DepthInverted"

#define NVSDK_NGX_Parameter_DLSSNR_Color "DLSSNR.Color"
#define NVSDK_NGX_Parameter_DLSSNR_Output "DLSSNR.Output"
#define NVSDK_NGX_Parameter_DLSSNR_Depth "DLSSNR.Depth"
#define NVSDK_NGX_Parameter_DLSSNR_MVec "DLSSNR.MVec"
#define NVSDK_NGX_Parameter_DLSSNR_MVecScaleX "DLSSNR.MVecScaleX"
#define NVSDK_NGX_Parameter_DLSSNR_MVecScaleY "DLSSNR.MVecScaleY"
#define NVSDK_NGX_Parameter_DLSSNR_Reset "DLSSNR.Reset"
#define NVSDK_NGX_Parameter_DLSSNR_UICorrection "DLSSNR.UICorrection"

#define NVSDK_NGX_Parameter_DLSSNR_Style "DLSSNR.Style"
#define NVSDK_NGX_Parameter_DLSSNR_Intensity "DLSSNR.Intensity"
#define NVSDK_NGX_Parameter_DLSSNR_LocalToneStrength "DLSSNR.LocalToneStrength"
#define NVSDK_NGX_Parameter_DLSSNR_LocalStructureStrength "DLSSNR.LocalStructureStrength"
#define NVSDK_NGX_Parameter_DLSSNR_SkinStructureStrength "DLSSNR.SkinStructureStrength"
#define NVSDK_NGX_Parameter_DLSSNR_UseAutoMask "DLSSNR.UseAutoMask"

#define NVSDK_NGX_Parameter_DLSSNR_ColorSubrectBaseX "DLSSNR.ColorSubrectBaseX"
#define NVSDK_NGX_Parameter_DLSSNR_ColorSubrectBaseY "DLSSNR.ColorSubrectBaseY"
#define NVSDK_NGX_Parameter_DLSSNR_ColorSubrectWidth "DLSSNR.ColorSubrectWidth"
#define NVSDK_NGX_Parameter_DLSSNR_ColorSubrectHeight "DLSSNR.ColorSubrectHeight"

#define NVSDK_NGX_Parameter_DLSSNR_OutputSubrectBaseX "DLSSNR.OutputSubrectBaseX"
#define NVSDK_NGX_Parameter_DLSSNR_OutputSubrectBaseY "DLSSNR.OutputSubrectBaseY"
#define NVSDK_NGX_Parameter_DLSSNR_OutputSubrectWidth "DLSSNR.OutputSubrectWidth"
#define NVSDK_NGX_Parameter_DLSSNR_OutputSubrectHeight "DLSSNR.OutputSubrectHeight"

#define NVSDK_NGX_Parameter_DLSSNR_MVecSubrectBaseX "DLSSNR.MVecSubrectBaseX"
#define NVSDK_NGX_Parameter_DLSSNR_MVecSubrectBaseY "DLSSNR.MVecSubrectBaseY"
#define NVSDK_NGX_Parameter_DLSSNR_MVecSubrectWidth "DLSSNR.MVecSubrectWidth"
#define NVSDK_NGX_Parameter_DLSSNR_MVecSubrectHeight "DLSSNR.MVecSubrectHeight"

#define NVSDK_NGX_Parameter_DLSSNR_DepthSubrectBaseX "DLSSNR.DepthSubrectBaseX"
#define NVSDK_NGX_Parameter_DLSSNR_DepthSubrectBaseY "DLSSNR.DepthSubrectBaseY"
#define NVSDK_NGX_Parameter_DLSSNR_DepthSubrectWidth "DLSSNR.DepthSubrectWidth"
#define NVSDK_NGX_Parameter_DLSSNR_DepthSubrectHeight "DLSSNR.DepthSubrectHeight"

namespace ngx {

    const bool forceManualLoad = false;
    wil::unique_hmodule dlssnrModule;
    auto NVSDK_NGX_D3D12_CreateFeature = &::NVSDK_NGX_D3D12_CreateFeature;
    auto NVSDK_NGX_D3D12_ReleaseFeature = &::NVSDK_NGX_D3D12_ReleaseFeature;
    auto NVSDK_NGX_D3D12_EvaluateFeature = &::NVSDK_NGX_D3D12_EvaluateFeature;

    // Hack to avoid NVSDK_NGX_Result_FAIL_PlatformError when loading the DLSS-NR DLL outside of NGX. Pretend that the
    // caller is NGX.
    DEFINE_DETOUR_FUNCTION(DWORD, GetModuleFileNameW, HMODULE hModule, LPWSTR lpFilename, DWORD nSize) {
        HMODULE callerModule;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)_ReturnAddress(),
                           &callerModule);
        if (callerModule == dlssnrModule.get()) {
            static const std::wstring nvngx = L"nvngx.dll";
            _snwprintf_s(lpFilename, nSize, _TRUNCATE, nvngx.c_str());
            if (nSize < nvngx.size() + 1) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
            return nSize;
        }

        return original_GetModuleFileNameW(hModule, lpFilename, nSize);
    }

} // namespace ngx

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;

    namespace {

        struct BOX {
            int32_t left;
            int32_t top;
            int32_t right;
            int32_t bottom;
        };

        // Root constants of DlssnrCacheCS.hlsl.
        struct DlssnrCacheConstants {
            int32_t srcOffset[2];
            int32_t dstOffset[2];
            int32_t size[2];
            int32_t corrShift[2];
            uint32_t mode;
            uint32_t inputIsLinear;
            uint32_t outputIsUnorm;
            uint32_t borderWidth;
            float borderColor[4];
            float correctionScale;
            int32_t correctionSize[2];
            uint32_t padding;
        };
        static_assert(sizeof(DlssnrCacheConstants) == 20 * sizeof(uint32_t));

        enum DlssnrCachePass : uint32_t {
            StoreCorrection = 0,
            ApplyCorrection = 1,
            DrawBorder = 2,
            Downscale = 3,
            FadeEdge = 4
        };

        // Root constants of the motion passes (DlssnrMotion.hlsli).
        struct DlssnrMotionConstants {
            float currentTan[4];
            float previousTan[4];
            float row0[4];
            float row1[4];
            float row2[4];
            float size[2];
            float radius;
            float penalty;
            int32_t origin[2];
            int32_t regionSize[2];
            int32_t regionOrigin[2];
            float shift[2];
            float scale;
            float padding[3];
        };
        static_assert(sizeof(DlssnrMotionConstants) == 36 * sizeof(uint32_t));

        constexpr uint32_t DlssnrMotionDescriptorsPerPass = 8;
        constexpr uint32_t DlssnrMotionDescriptorRing = 32;

        constexpr uint32_t DlssnrCacheDescriptorsPerPass = 8;
        constexpr uint32_t DlssnrCacheDescriptorRing = 32;

        // A view format that reads/writes the values as stored. UNKNOWN when unsupported.
        DXGI_FORMAT GetDlssnrViewFormat(DXGI_FORMAT format, bool forUav, bool& decodesSrgb) {
            decodesSrgb = false;
            switch (format) {
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
                return forUav ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_B8G8R8X8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                // A fully-typed sRGB texture can only be read decoded, and cannot be a UAV.
                decodesSrgb = true;
                return forUav ? DXGI_FORMAT_UNKNOWN : format;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                return DXGI_FORMAT_R10G10B10A2_UNORM;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R11G11B10_FLOAT:
                return DXGI_FORMAT_R11G11B10_FLOAT;
            default:
                return DXGI_FORMAT_UNKNOWN;
            }
        }

        bool IsUnormFormat(DXGI_FORMAT format) {
            return format != DXGI_FORMAT_R16G16B16A16_FLOAT && format != DXGI_FORMAT_R11G11B10_FLOAT;
        }

        DirectX::XMVECTOR LoadOvrQuat(const ovrQuatf& q) {
            return DirectX::XMVectorSet(q.x, q.y, q.z, q.w);
        }

        // Project a direction (eye space, -Z forward) onto an eye's image, in pixels of an image of the given size.
        XrVector2f ProjectToImage(const XrVector3f& direction, const ovrFovPort& fov, float width, float height) {
            const float tx = direction.x / -direction.z;
            const float ty = direction.y / -direction.z;
            return {(tx + fov.LeftTan) / (fov.LeftTan + fov.RightTan) * width,
                    (fov.UpTan - ty) / (fov.UpTan + fov.DownTan) * height};
        }

    } // namespace

    // One gaze direction per eye, relative to the head (-Z forward).
    bool OpenXrRuntime::getDlssnrEyeGazes(XrVector3f gaze[xr::StereoView::Count]) const {
        if (m_eyeTrackingType == EyeTracking::Mmf && m_dlssnrGazePerEye) {
            std::shared_lock lock(m_bodyStateMutex);

            if (!(m_cachedBodyState.LeftEyeIsValid && m_cachedBodyState.RightEyeIsValid)) {
                return false;
            }
            if (!(m_cachedBodyState.LeftEyeConfidence > 0.5f && m_cachedBodyState.RightEyeConfidence > 0.5f)) {
                return false;
            }

            // Each eye's own gaze: this accounts for vergence on near objects.
            const BodyTracking::Pose* eyePoses[] = {&m_cachedBodyState.LeftEyePose, &m_cachedBodyState.RightEyePose};
            for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                const auto& o = eyePoses[eye]->orientation;
                const auto forward = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.f, 0.f, -1.f, 0.f),
                                                              DirectX::XMVectorSet(o.x, o.y, o.z, o.w));
                xr::math::StoreXrVector3(&gaze[eye], forward);
            }
            return true;
        }

        XrVector3f unitVector{};
        XrTime sampleTime;
        if (!getEyeGaze(0, false /* getStateOnly */, unitVector, sampleTime)) {
            return false;
        }
        gaze[xr::StereoView::Left] = gaze[xr::StereoView::Right] = unitVector;
        return true;
    }

    void OpenXrRuntime::upliftLayer(const XrSwapchainSubImage** views,
                                    const XrSwapchainSubImage** depth,
                                    float nearZ,
                                    float farZ,
                                    ovrLayerEyeFov& layer) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "DLSSNR");

        const auto now = std::chrono::steady_clock::now();

        const ovrSizei resolution = {std::max(layer.Viewport[0].Size.w, layer.Viewport[1].Size.w),
                                     std::max(layer.Viewport[0].Size.h, layer.Viewport[1].Size.h)};
        // The foveated region's width and height, as fractions of each eye's image.
        const float foveatedWidth = std::clamp(m_dlssnrFoveationWidth, 0.1f, 1.f);
        const float foveatedHeight = std::clamp(m_dlssnrFoveationHeight, 0.1f, 1.f);
        const float foveatedScale = std::min(foveatedWidth, foveatedHeight) < 1.f ? 0.f : 1.f;  // < 1: a region
        const ovrSizei foveatedResolution = {
            std::min(resolution.w, (int)xr::math::AlignTo<4>((uint32_t)(resolution.w * foveatedWidth))),
            std::min(resolution.h, (int)xr::math::AlignTo<4>((uint32_t)(resolution.h * foveatedHeight)))};

        // NR's working resolution: the foveated region, or a downscaled copy of it whose NR correction is then
        // applied, upscaled, to the full-size region (needs the cache/debug passes).
        const float workingScale = std::clamp(m_dlssnrWorkingScale, 0.25f, 1.f);
        const bool reducedResolution = workingScale < 0.995f && m_dlssnrCachePSO;
        const ovrSizei nrResolution =
            reducedResolution
                ? ovrSizei{(int)xr::math::AlignTo<4>((uint32_t)std::ceil(foveatedResolution.w * workingScale)),
                           (int)xr::math::AlignTo<4>((uint32_t)std::ceil(foveatedResolution.h * workingScale))}
                : foveatedResolution;

        // Place the foveated region of each eye: centered, or following the gaze.
        ovrVector2i offset[xr::StereoView::Count]{};
        bool resetHistory[xr::StereoView::Count]{};
        const bool followGaze =
            m_dlssnrEyeTracking && m_eyeTrackingType != EyeTracking::None && foveatedScale < 1.f;
        XrVector3f gaze[xr::StereoView::Count]{};
        const bool gazeValid = followGaze && getDlssnrEyeGazes(gaze) && gaze[0].z < -0.1f && gaze[1].z < -0.1f;
        if (gazeValid) {
            m_dlssnrLastGazeTime = now;
        }
        // Gaze lost (blink): keep the last position for a moment before going back to the center.
        const bool holdGaze = followGaze && !gazeValid &&
                              now - m_dlssnrLastGazeTime < std::chrono::milliseconds(m_dlssnrGazeHoldMs);
        // The fixed center, stereo-aligned: the same point of the scene in both eyes, straight ahead at
        // m_dlssnrFixedDistance (infinity when 0). The image centers differ between eyes (asymmetric fields of
        // view), so centering each image would show two regions that do not overlap.
        XrVector2f alignedCenter[xr::StereoView::Count]{};
        {
            const auto& left = layer.RenderPose[xr::StereoView::Left].Position;
            const auto& right = layer.RenderPose[xr::StereoView::Right].Position;
            const float dx = right.x - left.x, dy = right.y - left.y, dz = right.z - left.z;
            const float halfIpd = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
            for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                // Eye render poses are assumed parallel to the head (no canted displays).
                const float eyeX = eye == xr::StereoView::Left ? -halfIpd : halfIpd;
                const XrVector3f ahead = m_dlssnrFixedDistance > 0.f ? XrVector3f{-eyeX, 0.f, -m_dlssnrFixedDistance}
                                                                     : XrVector3f{0.f, 0.f, -1.f};
                const XrVector2f point = ProjectToImage(ahead, layer.Fov[eye], 1.f, 1.f);
                alignedCenter[eye] = {std::clamp(point.x, 0.f, 1.f), std::clamp(point.y, 0.f, 1.f)};
            }
        }
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            if (!followGaze) {
                const ovrSizei& viewSize = layer.Viewport[eye].Size;
                offset[eye] = {
                    std::clamp((int)(alignedCenter[eye].x * viewSize.w) - foveatedResolution.w / 2,
                               0,
                               std::max(0, viewSize.w - foveatedResolution.w)),
                    std::clamp((int)(alignedCenter[eye].y * viewSize.h) - foveatedResolution.h / 2,
                               0,
                               std::max(0, viewSize.h - foveatedResolution.h))};
                m_dlssnrFoveaOffset[eye] = {-1, -1};
                continue;
            }

            if (!holdGaze) {
                XrVector2f target = alignedCenter[eye];
                if (gazeValid) {
                    // Eye render poses are assumed parallel to the head (no canted displays).
                    const XrVector2f point = ProjectToImage(gaze[eye], layer.Fov[eye], 1.f, 1.f);
                    target = {std::clamp(point.x, 0.f, 1.f), std::clamp(point.y, 0.f, 1.f)};
                }
                const float follow = 1.f - m_dlssnrGazeSmoothing;
                m_dlssnrGazeCenter[eye].x += (target.x - m_dlssnrGazeCenter[eye].x) * follow;
                m_dlssnrGazeCenter[eye].y += (target.y - m_dlssnrGazeCenter[eye].y) * follow;
            }

            const ovrSizei& viewSize = layer.Viewport[eye].Size;
            const int maxX = std::max(0, viewSize.w - foveatedResolution.w);
            const int maxY = std::max(0, viewSize.h - foveatedResolution.h);
            const int x =
                std::clamp((int)(m_dlssnrGazeCenter[eye].x * viewSize.w) - foveatedResolution.w / 2, 0, maxX);
            const int y =
                std::clamp((int)(m_dlssnrGazeCenter[eye].y * viewSize.h) - foveatedResolution.h / 2, 0, maxY);

            // Move the region in steps only, so gaze jitter does not keep shifting the NR input.
            ovrVector2i& current = m_dlssnrFoveaOffset[eye];
            if (current.x >= 0) {
                current = {std::min(current.x, maxX), std::min(current.y, maxY)};
            }
            const int step = m_dlssnrGazeQuantization;
            if (current.x < 0 || std::abs(x - current.x) >= step || std::abs(y - current.y) >= step) {
                const ovrVector2i next = {std::clamp((x + step / 2) / step * step, 0, maxX),
                                          std::clamp((y + step / 2) / step * step, 0, maxY)};
                // Large jump (saccade): restart the NR history instead of blending with the old location.
                if (current.x >= 0 && m_dlssnrGazeJumpReset > 0.f &&
                    (std::abs(next.x - current.x) > m_dlssnrGazeJumpReset * foveatedResolution.w ||
                     std::abs(next.y - current.y) > m_dlssnrGazeJumpReset * foveatedResolution.h)) {
                    resetHistory[eye] = true;
                }
                current = next;
            }
            offset[eye] = current;
        }

        const auto SafeClamp = [](BOX& box, const BOX& limit) {
            box.left = std::max(limit.left, box.left);
            box.right = std::min(limit.right, box.right);
            box.top = std::max(limit.top, box.top);
            box.bottom = std::min(limit.bottom, box.bottom);
        };

        const BOX inputBox[2] = {{views[0]->imageRect.offset.x,
                                  views[0]->imageRect.offset.y,
                                  views[0]->imageRect.offset.x + views[0]->imageRect.extent.width,
                                  views[0]->imageRect.offset.y + views[0]->imageRect.extent.height},
                                 {views[1]->imageRect.offset.x,
                                  views[1]->imageRect.offset.y,
                                  views[1]->imageRect.offset.x + views[1]->imageRect.extent.width,
                                  views[1]->imageRect.offset.y + views[1]->imageRect.extent.height}};

        const BOX outputBox[2] = {{0, 0, resolution.w, resolution.h},
                                  {resolution.w, 0, 2 * resolution.w, resolution.h}};

        // Resize resources if needed.
        if (!m_dlssnrOutputSwapchain || m_dlssnrOutputSwapchainResolution.w < resolution.w ||
            m_dlssnrOutputSwapchainResolution.h < resolution.h) {
            // Finish in-flight work before releasing the resources.
            m_dlssnrContext->Flush();

            Swapchain& xrSwapchain = *(Swapchain*)views[0]->swapchain;
            ensureDlssnrSwapchainResources(xrSwapchain.ovrDesc.Format, resolution);

            m_dlssnrOutputSwapchainResolution = resolution;
        }
        if (m_dlssnrFeatureResolution.w < nrResolution.w || m_dlssnrFeatureResolution.h < nrResolution.h ||
            m_dlssnrFeaturePreset != m_dlssnrPreset) {
            // Finish in-flight work before releasing the resources.
            m_dlssnrContext->Flush();

            // Destroy DLSS-NR feature - it will be re-created in the loop further below.
            for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                if (m_dlssnrFeature[eye]) {
                    ngx::NVSDK_NGX_D3D12_ReleaseFeature(m_dlssnrFeature[eye]);
                    m_dlssnrFeature[eye] = nullptr;
                }
                m_dlssnrCacheValid[eye] = false;
            }

            m_dlssnrFeatureResolution = nrResolution;
            m_dlssnrFeaturePreset = m_dlssnrPreset;
        }

        // Correction cache: decide whether to run NR this frame or reuse its last correction.
        const bool cacheEnabled = m_dlssnrCacheMode != 0 && m_dlssnrCachePSO;
        if (cacheEnabled || reducedResolution) {
            ensureDlssnrCorrectionResources(foveatedResolution);
        }
        if (reducedResolution) {
            // Working textures in a format that keeps the input's values (encoding and range).
            Swapchain& xrSwapchain = *(Swapchain*)views[0]->swapchain;
            const auto& slice = xrSwapchain.resolvedSlices[views[0]->imageArrayIndex];
            const auto inputFormat = slice.dlssnrImages[slice.lastCommittedIndex]->GetDesc().Format;
            bool decodes = false;
            const auto viewFormat = GetDlssnrViewFormat(inputFormat, false, decodes);
            const DXGI_FORMAT workFormat =
                (viewFormat == DXGI_FORMAT_R16G16B16A16_FLOAT || viewFormat == DXGI_FORMAT_R11G11B10_FLOAT)
                    ? DXGI_FORMAT_R16G16B16A16_FLOAT
                : viewFormat == DXGI_FORMAT_R10G10B10A2_UNORM ? DXGI_FORMAT_R10G10B10A2_UNORM
                                                              : DXGI_FORMAT_R8G8B8A8_UNORM;
            ensureDlssnrWorkResources(nrResolution, workFormat);
        }
        if (!cacheEnabled) {
            m_dlssnrCacheValid[0] = m_dlssnrCacheValid[1] = false;
            m_dlssnrFramesSinceFullEval = 0;
        }
        if (m_dlssnrGpuTimerPending) {
            const uint64_t durationUs = m_dlssnrGpuTimer->query(false /* reset */);
            if (durationUs) {
                m_dlssnrLastGpuMs = durationUs / 1000.f;
                m_dlssnrGpuTimerPending = false;
            }
        }
        bool useCache = false;
        ovrVector2i correctionShift[xr::StereoView::Count]{};
        if (cacheEnabled && m_dlssnrCacheValid[0] && m_dlssnrCacheValid[1]) {
            // "Evaluate every N frames".
            const uint32_t maxInterval = (uint32_t)m_dlssnrCacheMaxInterval;
            // Adaptive mode with a GPU budget: while NR fits in the budget, run it every frame.
            const bool nrFitsBudget = m_dlssnrCacheMode == 2 && m_dlssnrCacheGpuBudgetMs > 0.f &&
                                      m_dlssnrLastGpuMs > 0.f && m_dlssnrLastGpuMs <= m_dlssnrCacheGpuBudgetMs;
            useCache = m_dlssnrFramesSinceFullEval + 1 < maxInterval && !nrFitsBudget;

            for (uint32_t eye = 0; eye < xr::StereoView::Count && useCache; eye++) {
                // The region moved: the stored correction does not cover it.
                if (resetHistory[eye] || std::abs(m_dlssnrCorrectionScale[eye] - workingScale) > 0.001f ||
                    offset[eye].x != m_dlssnrCacheOffset[eye].x ||
                    offset[eye].y != m_dlssnrCacheOffset[eye].y) {
                    useCache = false;
                    break;
                }

                // Head rotation since the correction was computed.
                const auto cached = LoadOvrQuat(m_dlssnrCacheOrientation[eye]);
                const auto current = LoadOvrQuat(layer.RenderPose[eye].Orientation);
                const float dot = std::abs(DirectX::XMVectorGetX(DirectX::XMQuaternionDot(cached, current)));
                const float angleDeg = 2.f * std::acos(std::min(dot, 1.f)) * 180.f / DirectX::XM_PI;
                if (angleDeg > m_dlssnrCacheMaxRotationDeg) {
                    useCache = false;
                    break;
                }

                // Shift the correction to follow the image: find where the old view center is seen now.
                const auto world = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.f, 0.f, -1.f, 0.f), cached);
                XrVector3f seenNow;
                xr::math::StoreXrVector3(&seenNow,
                                         DirectX::XMVector3Rotate(world, DirectX::XMQuaternionInverse(current)));
                if (seenNow.z > -0.1f) {
                    useCache = false;
                    break;
                }
                const ovrSizei& viewSize = layer.Viewport[eye].Size;
                const XrVector2f moved =
                    ProjectToImage(seenNow, layer.Fov[eye], (float)viewSize.w, (float)viewSize.h);
                const XrVector2f center =
                    ProjectToImage({0.f, 0.f, -1.f}, layer.Fov[eye], (float)viewSize.w, (float)viewSize.h);
                correctionShift[eye] = {-(int)std::lround(moved.x - center.x),
                                        -(int)std::lround(moved.y - center.y)};
            }
        }

        // Statistics, logged every 10 seconds.
        if (m_dlssnrStatStart.time_since_epoch().count() == 0) {
            m_dlssnrStatStart = now;
        }
        m_dlssnrStatFrames++;
        m_dlssnrStatCached += useCache ? 1 : 0;
        m_dlssnrStatGaze += gazeValid ? 1 : 0;
        m_dlssnrStatResets += (resetHistory[0] || resetHistory[1]) ? 1 : 0;
        if (now - m_dlssnrStatStart >= std::chrono::seconds(10)) {
            Log("DLSS-NR: %u frames, gaze %s %.0f%%, cached %.0f%%, motion %.0f%%, NR GPU %.2f ms, %u resets, fovea L(%d,%d) "
                "R(%d,%d) %dx%d, depth %s\n",
                m_dlssnrStatFrames,
                followGaze ? "on" : "off",
                100.f * m_dlssnrStatGaze / m_dlssnrStatFrames,
                100.f * m_dlssnrStatCached / m_dlssnrStatFrames,
                50.f * m_dlssnrStatMotion / m_dlssnrStatFrames,
                m_dlssnrLastGpuMs,
                m_dlssnrStatResets,
                offset[0].x,
                offset[0].y,
                offset[1].x,
                offset[1].y,
                foveatedResolution.w,
                foveatedResolution.h,
                !(depth[0] && depth[1]) ? "none" : m_dlssnrUseDepth ? "used" : "off");
            m_dlssnrStatFrames = m_dlssnrStatCached = m_dlssnrStatGaze = m_dlssnrStatResets = m_dlssnrStatMotion = 0;
            m_dlssnrStatStart = now;
        }

        // Serialize inputs.
#if 1
        m_fenceValue++;
        CHECK_HRCMD(m_ovrSubmissionContext->Signal(m_ovrSubmissionFence.Get(), m_fenceValue));
        CHECK_HRCMD(m_dlssnrContext->GetCommandQueue()->Wait(m_dlssnrInFence.Get(), m_fenceValue));
#else
        flushSubmissionContext();
#endif

        // Time to apply the effect.
        auto cmdList = m_dlssnrContext->GetCommandList();

        // Descriptors for the cache/debug passes of one eye.
        const auto PrepareCachePass = [&](uint32_t eye,
                                          ID3D12Resource* input,
                                          ID3D12Resource* output,
                                          D3D12_GPU_DESCRIPTOR_HANDLE& srvTable,
                                          D3D12_GPU_DESCRIPTOR_HANDLE& uavTable,
                                          D3D12_GPU_DESCRIPTOR_HANDLE& workSrvTable,
                                          D3D12_GPU_DESCRIPTOR_HANDLE& workUavTable,
                                          DlssnrCacheConstants& constants) -> bool {
            if (!m_dlssnrCachePSO) {
                return false;
            }
            const auto inputDesc = input->GetDesc();
            const auto outputDesc = output->GetDesc();
            // The input may be an array (a texture-array swapchain's slice 0 is the app's own image).
            if (outputDesc.DepthOrArraySize != 1) {
                return false;
            }
            const UINT inputSlice = inputDesc.DepthOrArraySize > 1 ? views[eye]->imageArrayIndex : 0;
            bool inputIsLinear = false, outputDecodes = false;
            const DXGI_FORMAT inputFormat = GetDlssnrViewFormat(inputDesc.Format, false, inputIsLinear);
            const DXGI_FORMAT outputSrvFormat = GetDlssnrViewFormat(outputDesc.Format, false, outputDecodes);
            const DXGI_FORMAT outputUavFormat = GetDlssnrViewFormat(outputDesc.Format, true, outputDecodes);
            if (inputFormat == DXGI_FORMAT_UNKNOWN || outputSrvFormat == DXGI_FORMAT_UNKNOWN ||
                outputUavFormat == DXGI_FORMAT_UNKNOWN || outputDecodes) {
                return false;
            }
            constants.inputIsLinear = inputIsLinear;
            constants.outputIsUnorm = IsUnormFormat(outputUavFormat);

            const uint32_t slot = (m_dlssnrCacheHeapSlot++ % DlssnrCacheDescriptorRing) * DlssnrCacheDescriptorsPerPass;
            const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBase(
                m_dlssnrCacheHeap->GetCPUDescriptorHandleForHeapStart(), slot, m_dlssnrCacheDescriptorSize);
            const CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBase(
                m_dlssnrCacheHeap->GetGPUDescriptorHandleForHeapStart(), slot, m_dlssnrCacheDescriptorSize);

            D3D12_SHADER_RESOURCE_VIEW_DESC inputSrv{};
            inputSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            inputSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            inputSrv.Texture2DArray = {0, 1, inputSlice, 1, 0, 0.f};
            inputSrv.Format = inputFormat;
            m_dlssnrDevice->CreateShaderResourceView(
                input, &inputSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 0, m_dlssnrCacheDescriptorSize));
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv.Texture2D.MipLevels = 1;
            srv.Format = outputSrvFormat;
            m_dlssnrDevice->CreateShaderResourceView(
                output, &srv, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 1, m_dlssnrCacheDescriptorSize));

            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav.Format = outputUavFormat;
            m_dlssnrDevice->CreateUnorderedAccessView(
                output, nullptr, &uav, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 2, m_dlssnrCacheDescriptorSize));
            uav.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            m_dlssnrDevice->CreateUnorderedAccessView(m_dlssnrCorrection[eye].Get(),
                                                      nullptr,
                                                      &uav,
                                                      CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 3, m_dlssnrCacheDescriptorSize));

            // Reduced working resolution: [4] working input SRV, [5] working output SRV, [6] working input UAV,
            // [7] correction UAV (null views without it).
            const DXGI_FORMAT workFormat =
                m_dlssnrWorkFormat != DXGI_FORMAT_UNKNOWN ? m_dlssnrWorkFormat : DXGI_FORMAT_R8G8B8A8_UNORM;
            srv.Format = workFormat;
            m_dlssnrDevice->CreateShaderResourceView(m_dlssnrWorkInput[eye].Get(),
                                                     &srv,
                                                     CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 4, m_dlssnrCacheDescriptorSize));
            m_dlssnrDevice->CreateShaderResourceView(m_dlssnrWorkOutput[eye].Get(),
                                                     &srv,
                                                     CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 5, m_dlssnrCacheDescriptorSize));
            uav.Format = workFormat;
            m_dlssnrDevice->CreateUnorderedAccessView(m_dlssnrWorkInput[eye].Get(),
                                                      nullptr,
                                                      &uav,
                                                      CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 6, m_dlssnrCacheDescriptorSize));
            uav.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            m_dlssnrDevice->CreateUnorderedAccessView(m_dlssnrCorrection[eye].Get(),
                                                      nullptr,
                                                      &uav,
                                                      CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 7, m_dlssnrCacheDescriptorSize));

            srvTable = gpuBase;
            uavTable = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuBase, 2, m_dlssnrCacheDescriptorSize);
            workSrvTable = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuBase, 4, m_dlssnrCacheDescriptorSize);
            workUavTable = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuBase, 6, m_dlssnrCacheDescriptorSize);
            return true;
        };
        const auto DispatchCachePass = [&](const DlssnrCacheConstants& constants,
                                           const D3D12_GPU_DESCRIPTOR_HANDLE& srvTable,
                                           const D3D12_GPU_DESCRIPTOR_HANDLE& uavTable) {
            // NGX binds its own state: always set ours again.
            ID3D12DescriptorHeap* const heaps[] = {m_dlssnrCacheHeap.Get()};
            cmdList.Commands->SetDescriptorHeaps(1, heaps);
            cmdList.Commands->SetComputeRootSignature(m_dlssnrCacheRootSignature.Get());
            cmdList.Commands->SetPipelineState(m_dlssnrCachePSO.Get());
            cmdList.Commands->SetComputeRoot32BitConstants(
                0, sizeof(constants) / sizeof(uint32_t), &constants, 0);
            cmdList.Commands->SetComputeRootDescriptorTable(1, srvTable);
            cmdList.Commands->SetComputeRootDescriptorTable(2, uavTable);
            cmdList.Commands->Dispatch((constants.size[0] + 7) / 8, (constants.size[1] + 7) / 8, 1);
        };
        const auto UavBarrier = [&]() {
            const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
            cmdList.Commands->ResourceBarrier(1, &barrier);
        };

        // Motion vectors for NR from optical flow (luminance, flow, then full-size vectors over the foveated
        // region). `region` is the foveated region in the eye's image. Returns false when there are none
        // (first frame, or the image changed size): NR then runs without motion, as before.
        const bool motionEnabled = m_dlssnrMotion && m_dlssnrMotionPSO;
        if (motionEnabled) {
            ensureDlssnrMotionResources(resolution, foveatedResolution);
        } else {
            m_dlssnrHistoryValid[0] = m_dlssnrHistoryValid[1] = false;
        }
        const auto RecordMotionPasses = [&](uint32_t eye,
                                            ID3D12Resource* input,
                                            const BOX& eyeBox,
                                            const ovrVector2i& region,
                                            const ovrSizei& regionSize,
                                            const ovrSizei& outputSize,
                                            float scale) -> bool {
            const ovrSizei imageSize = {eyeBox.right - eyeBox.left, eyeBox.bottom - eyeBox.top};
            float currentTan[4] = {-layer.Fov[eye].LeftTan,
                                   layer.Fov[eye].RightTan,
                                   layer.Fov[eye].UpTan,
                                   -layer.Fov[eye].DownTan};

            const auto inputDesc = input->GetDesc();
            bool decodes = false;
            const DXGI_FORMAT inputFormat = GetDlssnrViewFormat(inputDesc.Format, false, decodes);
            if (inputFormat == DXGI_FORMAT_UNKNOWN || imageSize.w <= 0 || imageSize.h <= 0) {
                m_dlssnrHistoryValid[eye] = false;
                return false;
            }

            // Descriptors: [0] input SRV; [1..3] luma current, luma previous, flow; [4..6] flow, motion, null.
            const uint32_t slot = (m_dlssnrMotionHeapSlot++ % DlssnrMotionDescriptorRing) * DlssnrMotionDescriptorsPerPass;
            const UINT increment = m_dlssnrCacheDescriptorSize;
            const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBase(m_dlssnrMotionHeap->GetCPUDescriptorHandleForHeapStart(), slot, increment);
            const CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBase(m_dlssnrMotionHeap->GetGPUDescriptorHandleForHeapStart(), slot, increment);
            const uint32_t current = m_dlssnrLumaCurrent[eye], previous = current ^ 1;

            // One slice of the input (an array for a texture-array swapchain's slice 0).
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv.Texture2DArray = {0, 1, inputDesc.DepthOrArraySize > 1 ? views[eye]->imageArrayIndex : 0u, 1, 0, 0.f};
            srv.Format = inputFormat;
            m_dlssnrDevice->CreateShaderResourceView(input, &srv, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 0, increment));
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            const std::pair<ID3D12Resource*, DXGI_FORMAT> uavs[] = {
                {m_dlssnrLuma[eye][current].Get(), DXGI_FORMAT_R16_FLOAT},
                {m_dlssnrLuma[eye][previous].Get(), DXGI_FORMAT_R16_FLOAT},
                {m_dlssnrFlow[eye].Get(), DXGI_FORMAT_R16G16_FLOAT},
                {m_dlssnrFlow[eye].Get(), DXGI_FORMAT_R16G16_FLOAT},
                {m_dlssnrMotionVectors[eye].Get(), DXGI_FORMAT_R16G16_FLOAT},
                {nullptr, DXGI_FORMAT_R16G16_FLOAT}};
            for (uint32_t i = 0; i < std::size(uavs); i++) {
                uav.Format = uavs[i].second;
                m_dlssnrDevice->CreateUnorderedAccessView(
                    uavs[i].first, nullptr, &uav, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuBase, 1 + i, increment));
            }

            ID3D12DescriptorHeap* const heaps[] = {m_dlssnrMotionHeap.Get()};
            cmdList.Commands->SetDescriptorHeaps(1, heaps);
            cmdList.Commands->SetComputeRootSignature(m_dlssnrMotionRootSignature.Get());
            cmdList.Commands->SetComputeRootDescriptorTable(1, gpuBase);

            DlssnrMotionConstants constants{};
            std::copy(currentTan, currentTan + 4, constants.currentTan);
            constants.size[0] = (float)imageSize.w;
            constants.size[1] = (float)imageSize.h;
            const int blocksX = (imageSize.w + 3) / 4, blocksY = (imageSize.h + 3) / 4;

            // Luminance of the whole eye image, at a quarter of its size.
            constants.origin[0] = eyeBox.left;
            constants.origin[1] = eyeBox.top;
            cmdList.Commands->SetPipelineState(m_dlssnrLumaPSO.Get());
            cmdList.Commands->SetComputeRoot32BitConstants(0, sizeof(constants) / sizeof(uint32_t), &constants, 0);
            cmdList.Commands->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuBase, 1, increment));
            cmdList.Commands->Dispatch((blocksX + 7) / 8, (blocksY + 7) / 8, 1);

            const bool haveHistory = m_dlssnrHistoryValid[eye] && m_dlssnrHistoryImageSize[eye].w == imageSize.w &&
                                     m_dlssnrHistoryImageSize[eye].h == imageSize.h;
            if (haveHistory) {
                UavBarrier();

                // Previous view from current view: each column is a current-view axis seen from the previous view.
                const auto qc = LoadOvrQuat(layer.RenderPose[eye].Orientation);
                const auto qpInverse = DirectX::XMQuaternionInverse(LoadOvrQuat(m_dlssnrHistoryOrientation[eye]));
                float* rows[3] = {constants.row0, constants.row1, constants.row2};
                for (int j = 0; j < 3; j++) {
                    XrVector3f column;
                    xr::math::StoreXrVector3(
                        &column,
                        DirectX::XMVector3Rotate(
                            DirectX::XMVector3Rotate(
                                DirectX::XMVectorSet(j == 0 ? 1.f : 0.f, j == 1 ? 1.f : 0.f, j == 2 ? 1.f : 0.f, 0.f),
                                qc),
                            qpInverse));
                    rows[0][j] = column.x;
                    rows[1][j] = column.y;
                    rows[2][j] = column.z;
                }
                std::copy(m_dlssnrHistoryTan[eye], m_dlssnrHistoryTan[eye] + 4, constants.previousTan);
                constants.radius = m_dlssnrFlowRadius;
                constants.penalty = m_dlssnrFlowPenalty;

                // Flow for the blocks under the foveated region (plus one around, for the bilinear filter).
                const int firstX = std::max(0, region.x / 4 - 1), firstY = std::max(0, region.y / 4 - 1);
                const int lastX = std::min(blocksX, (region.x + regionSize.w + 3) / 4 + 1);
                const int lastY = std::min(blocksY, (region.y + regionSize.h + 3) / 4 + 1);
                constants.regionOrigin[0] = firstX;
                constants.regionOrigin[1] = firstY;
                constants.regionSize[0] = std::max(0, lastX - firstX);
                constants.regionSize[1] = std::max(0, lastY - firstY);
                cmdList.Commands->SetPipelineState(m_dlssnrFlowPSO.Get());
                cmdList.Commands->SetComputeRoot32BitConstants(0, sizeof(constants) / sizeof(uint32_t), &constants, 0);
                cmdList.Commands->Dispatch((constants.regionSize[0] + 7) / 8, (constants.regionSize[1] + 7) / 8, 1);
                UavBarrier();

                // Vectors over the foveated region, at NR's working resolution. NR's history follows the region:
                // when the region moved with the gaze, the content also moved by that much in NR's coordinates.
                constants.origin[0] = region.x;
                constants.origin[1] = region.y;
                constants.regionSize[0] = outputSize.w;
                constants.regionSize[1] = outputSize.h;
                constants.scale = scale;
                constants.shift[0] = (float)(region.x - m_dlssnrHistoryOffset[eye].x);
                constants.shift[1] = (float)(region.y - m_dlssnrHistoryOffset[eye].y);
                cmdList.Commands->SetPipelineState(m_dlssnrMotionPSO.Get());
                cmdList.Commands->SetComputeRoot32BitConstants(0, sizeof(constants) / sizeof(uint32_t), &constants, 0);
                cmdList.Commands->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuBase, 4, increment));
                cmdList.Commands->Dispatch((outputSize.w + 7) / 8, (outputSize.h + 7) / 8, 1);
            }

            // This frame becomes the history of the next frame NR runs on.
            m_dlssnrHistoryValid[eye] = true;
            m_dlssnrHistoryOrientation[eye] = layer.RenderPose[eye].Orientation;
            std::copy(currentTan, currentTan + 4, m_dlssnrHistoryTan[eye]);
            m_dlssnrHistoryOffset[eye] = region;
            m_dlssnrHistoryImageSize[eye] = imageSize;
            m_dlssnrLumaCurrent[eye] = previous;
            return haveHistory;
        };

        // The periphery of both eyes first. The output is one double-wide texture: copies into it must not follow
        // the GPU writes (NR, cache and debug passes) to the other eye's half.
        int outputIndex = 0;
        CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(m_ovrSession, m_dlssnrOutputSwapchain, &outputIndex));
        ID3D12Resource* const outputResource = m_dlssnrOutputSwapchainImages[outputIndex].Get();
        bool outputCopied = false;
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            const int imageIndex = outputIndex;
            Swapchain& xrSwapchain = *(Swapchain*)views[eye]->swapchain;
            ID3D12Resource* inputImage =
                xrSwapchain.resolvedSlices[views[eye]->imageArrayIndex]
                    .dlssnrImages[xrSwapchain.resolvedSlices[views[eye]->imageArrayIndex].lastCommittedIndex]
                    .Get();
                if (foveatedScale < 1.f) {
                    D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
                    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    src.SubresourceIndex = 0;
                    src.pResource = inputImage;
                    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dst.SubresourceIndex = 0;
                    dst.pResource = m_dlssnrOutputSwapchainImages[imageIndex].Get();

                    // The foveated region may touch an edge of the image: skip empty strips.
                    const auto CopyRegion = [&](const D3D12_TEXTURE_COPY_LOCATION* dstLocation,
                                                UINT dstX,
                                                UINT dstY,
                                                UINT dstZ,
                                                const D3D12_TEXTURE_COPY_LOCATION* srcLocation,
                                                const D3D12_BOX* srcBox) {
                        if (srcBox->right > srcBox->left && srcBox->bottom > srcBox->top) {
                            cmdList.Commands->CopyTextureRegion(dstLocation, dstX, dstY, dstZ, srcLocation, srcBox);
                            outputCopied = true;
                        }
                    };

                    const auto ToD3dBox = [](D3D12_BOX& d3dBox, const BOX& box) {
                        d3dBox.left = box.left;
                        d3dBox.top = box.top;
                        d3dBox.front = 0;
                        d3dBox.right = box.right;
                        d3dBox.bottom = box.bottom;
                        d3dBox.back = 1;
                    };

                    D3D12_BOX d3dBox;
                    BOX box{};
                    box.left = views[eye]->imageRect.offset.x;
                    box.top = views[eye]->imageRect.offset.y;
                    box.right = box.left + resolution.w;
                    box.bottom = box.top + offset[eye].y;
                    SafeClamp(box, inputBox[eye]);
                    ToD3dBox(d3dBox, box);
                    CopyRegion(&dst, eye * resolution.w, 0, 0, &src, &d3dBox);
                    box.top = views[eye]->imageRect.offset.y + offset[eye].y + foveatedResolution.h;
                    box.bottom = box.top + resolution.h;
                    SafeClamp(box, inputBox[eye]);
                    ToD3dBox(d3dBox, box);
                    CopyRegion(
                        &dst, eye * resolution.w, resolution.h - (box.bottom - box.top), 0, &src, &d3dBox);

                    box.left = views[eye]->imageRect.offset.x;
                    box.top = views[eye]->imageRect.offset.y + offset[eye].y;
                    box.right = box.left + offset[eye].x;
                    box.bottom = box.top + foveatedResolution.h;
                    SafeClamp(box, inputBox[eye]);
                    ToD3dBox(d3dBox, box);
                    CopyRegion(
                        &dst, eye * resolution.w, box.top - views[eye]->imageRect.offset.y, 0, &src, &d3dBox);
                    box.left = views[eye]->imageRect.offset.x + offset[eye].x + foveatedResolution.w;
                    box.right = box.left + resolution.w;
                    SafeClamp(box, inputBox[eye]);
                    ToD3dBox(d3dBox, box);
                    CopyRegion(&dst,
                                                        (eye + 1) * resolution.w - (box.right - box.left),
                                                        box.top - views[eye]->imageRect.offset.y,
                                                        0,
                                                        &src,
                                                        &d3dBox);
                }

        }
        {
            // From here on, the GPU writes the output (shared with the compositor: back to COMMON at the end).
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                outputResource,
                outputCopied ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdList.Commands->ResourceBarrier(1, &barrier);
        }
        // A copy into the output in the middle of the GPU writes (debug split, unsupported format).
        const auto CopyIntoOutput = [&](ID3D12Resource* source, const D3D12_BOX& box, UINT x, UINT y) {
            if (!(box.right > box.left && box.bottom > box.top)) {
                return;
            }
            D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
            src.Type = dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src.pResource = source;
            dst.pResource = outputResource;
            const auto toCopy = CD3DX12_RESOURCE_BARRIER::Transition(
                outputResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList.Commands->ResourceBarrier(1, &toCopy);
            cmdList.Commands->CopyTextureRegion(&dst, x, y, 0, &src, &box);
            const auto toWrite = CD3DX12_RESOURCE_BARRIER::Transition(
                outputResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdList.Commands->ResourceBarrier(1, &toWrite);
        };

        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_DepthInverted, nearZ > farZ);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Enabled, true);

            // (Re)create with the desired resolution.
            if (!m_dlssnrFeature[eye]) {
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Width, nrResolution.w);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Height, nrResolution.h);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Hint_Render_Preset, m_dlssnrPreset);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_CreationNodeMask, 1u);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_VisibilityNodeMask, 1u);

                if (ngx::forceManualLoad ||
                    NVSDK_NGX_FAILED(ngx::NVSDK_NGX_D3D12_CreateFeature(cmdList.Commands.Get(),
                                                                        NVSDK_NGX_Feature_Reserved18 /* DLSS-NR */,
                                                                        m_ngxParameters,
                                                                        &m_dlssnrFeature[eye]))) {
                    // User might be using an unsigned nvngx_dlssnr.dll that gets bounced by the NGX SDK. Try loading it
                    // manually.
                    if (!ngx::dlssnrModule) {
                        NVSDK_NGX_Result(NVSDK_CONV * Init_Ext)(unsigned long long,
                                                                const wchar_t*,
                                                                ID3D12Device*,
                                                                NVSDK_NGX_Version,
                                                                const NVSDK_NGX_Parameter*);
                        DetourDllAttach("Kernel32.dll",
                                        "GetModuleFileNameW",
                                        ngx::hooked_GetModuleFileNameW,
                                        ngx::original_GetModuleFileNameW);
                        // The detour stays in Kernel32 for the process's life: this runtime must stay loaded with
                        // it. Games that destroy and recreate their OpenXR instance (Luke Ross's R.E.A.L. VR) would
                        // otherwise unload it and crash on the next GetModuleFileNameW call.
                        {
                            HMODULE self;
                            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                                               reinterpret_cast<LPCWSTR>(&ngx::hooked_GetModuleFileNameW),
                                               &self);
                        }

                        const auto nvngx_dlssnrPath = virtualdesktop_openxr::dllHome / L"nvngx_dlssnr.dll";
                        *ngx::dlssnrModule.put() = LoadLibraryW(nvngx_dlssnrPath.c_str());
                        // clang-format off
                        Init_Ext = (decltype(Init_Ext))GetProcAddress(ngx::dlssnrModule.get(), "NVSDK_NGX_D3D12_Init_Ext");
                        ngx::NVSDK_NGX_D3D12_CreateFeature = (decltype(ngx::NVSDK_NGX_D3D12_CreateFeature))GetProcAddress(
                            ngx::dlssnrModule.get(), "NVSDK_NGX_D3D12_CreateFeature");
                        ngx::NVSDK_NGX_D3D12_ReleaseFeature = (decltype(ngx::NVSDK_NGX_D3D12_ReleaseFeature))GetProcAddress(
                            ngx::dlssnrModule.get(), "NVSDK_NGX_D3D12_ReleaseFeature");
                        ngx::NVSDK_NGX_D3D12_EvaluateFeature = (decltype(ngx::NVSDK_NGX_D3D12_EvaluateFeature))GetProcAddress(
                            ngx::dlssnrModule.get(), "NVSDK_NGX_D3D12_EvaluateFeature");
                        // clang-format on

                        CHECK_NGXCMD(Init_Ext(123456,
                                              virtualdesktop_openxr::programData.c_str(),
                                              m_dlssnrDevice.Get(),
                                              NVSDK_NGX_Version_API,
                                              nullptr));
                    }

                    CHECK_NGXCMD(ngx::NVSDK_NGX_D3D12_CreateFeature(cmdList.Commands.Get(),
                                                                    NVSDK_NGX_Feature_Reserved18 /* DLSS-NR */,
                                                                    m_ngxParameters,
                                                                    &m_dlssnrFeature[eye]));
                }
            }

            // Prepare swapchain outputs.
            int imageIndex = 0;
            CHECK_OVRCMD(ovr_GetTextureSwapChainCurrentIndex(m_ovrSession, m_dlssnrOutputSwapchain, &imageIndex));

            // Prepare feature evaluation.
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Output, m_dlssnrOutputSwapchainImages[imageIndex].Get());

            Swapchain& xrSwapchain = *(Swapchain*)views[eye]->swapchain;
            ID3D12Resource* inputImage =
                xrSwapchain.resolvedSlices[views[eye]->imageArrayIndex]
                    .dlssnrImages[xrSwapchain.resolvedSlices[views[eye]->imageArrayIndex].lastCommittedIndex]
                    .Get();
            BOX colorBox{};
            colorBox.left = views[eye]->imageRect.offset.x + offset[eye].x;
            colorBox.top = views[eye]->imageRect.offset.y + offset[eye].y;
            colorBox.right = colorBox.left + foveatedResolution.w;
            colorBox.bottom = colorBox.top + foveatedResolution.h;
            SafeClamp(colorBox, inputBox[eye]);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Color, inputImage);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectBaseX, colorBox.left);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectBaseY, colorBox.top);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectWidth, colorBox.right - colorBox.left);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectHeight, colorBox.bottom - colorBox.top);

            BOX destBox{};
            destBox.left = eye * resolution.w + offset[eye].x; // Double-wide.
            destBox.top = offset[eye].y;
            destBox.right = destBox.left + foveatedResolution.w;
            destBox.bottom = destBox.top + foveatedResolution.h;
            SafeClamp(destBox, outputBox[eye]);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectBaseX, destBox.left);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectBaseY, destBox.top);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectWidth, destBox.right - destBox.left);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectHeight, destBox.bottom - destBox.top);

#if 0
            // TODO: Use NVOFA to estimate motion.
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVec, motion);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectBaseX, 0);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectBaseY, 0);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectWidth, w);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectHeight, h);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecScaleX, motionVectorScaleX);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecScaleY, motionVectorScaleY);
#endif

            // The game's depth may be turned off (it can be misaligned or late for one eye).
            if (!(depth[0] && depth[1] && m_dlssnrUseDepth)) {
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Depth, (ID3D12Resource*)nullptr);
            }
            if (depth[0] && depth[1] && m_dlssnrUseDepth) {
                Swapchain& xrDepthSwapchain = *(Swapchain*)depth[eye]->swapchain;
                m_ngxParameters->Set(
                    NVSDK_NGX_Parameter_DLSSNR_Depth,
                    xrDepthSwapchain.resolvedSlices[depth[eye]->imageArrayIndex]
                        .dlssnrImages[xrDepthSwapchain.resolvedSlices[depth[eye]->imageArrayIndex].lastCommittedIndex]
                        .Get());
                BOX depthBox{};
                depthBox.left = depth[eye]->imageRect.offset.x + offset[eye].x;
                depthBox.top = depth[eye]->imageRect.offset.y + offset[eye].y;
                depthBox.right = depthBox.left + foveatedResolution.w;
                depthBox.bottom = depthBox.top + foveatedResolution.h;
                SafeClamp(depthBox, inputBox[eye]);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Color, inputImage);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_DepthSubrectBaseX, depthBox.left);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_DepthSubrectBaseY, depthBox.top);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_DepthSubrectWidth, depthBox.right - depthBox.left);
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_DepthSubrectHeight, depthBox.bottom - depthBox.top);
            }

            // Back from Toggle NR off: NR's history is from before.
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Reset, resetHistory[eye]);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_UseAutoMask, true);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Style, m_dlssnrStyle);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Intensity, m_dlssnrIntensity);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_LocalToneStrength, m_dlssnrLocalToneStrength);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_LocalStructureStrength, m_dlssnrLocalStructureStrength);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_SkinStructureStrength, m_dlssnrSkinStructureStrength);
            m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_UICorrection, 0);

            // Cache, reduced-resolution and debug passes work on the foveated region only.
            ID3D12Resource* const outputImage = m_dlssnrOutputSwapchainImages[imageIndex].Get();
            DlssnrCacheConstants constants{};
            constants.srcOffset[0] = colorBox.left;
            constants.srcOffset[1] = colorBox.top;
            constants.dstOffset[0] = destBox.left;
            constants.dstOffset[1] = destBox.top;
            constants.size[0] = std::min(colorBox.right - colorBox.left, destBox.right - destBox.left);
            constants.size[1] = std::min(colorBox.bottom - colorBox.top, destBox.bottom - destBox.top);
            constants.correctionScale = 1.f;
            constants.correctionSize[0] = constants.size[0];
            constants.correctionSize[1] = constants.size[1];
            D3D12_GPU_DESCRIPTOR_HANDLE srvTable{}, uavTable{}, workSrvTable{}, workUavTable{};
            const bool passesReady =
                (cacheEnabled || reducedResolution || m_dlssnrDebugFovea ||
                 m_dlssnrTransition > 0.f) &&
                constants.size[0] > 0 &&
                constants.size[1] > 0 &&
                PrepareCachePass(eye, inputImage, outputImage, srvTable, uavTable, workSrvTable, workUavTable, constants);
            const bool cachedEye = useCache && passesReady;
            // At a reduced working resolution, NR always goes through the correction (it cannot write the output).
            const bool reducedEye = reducedResolution && passesReady;
            const ovrSizei workSize = {std::min(nrResolution.w, (int)std::ceil(constants.size[0] * workingScale)),
                                       std::min(nrResolution.h, (int)std::ceil(constants.size[1] * workingScale))};

            const auto ApplyCorrectionPass = [&](const ovrVector2i& shift) {
                DlssnrCacheConstants apply = constants;
                apply.mode = ApplyCorrection;
                apply.corrShift[0] = shift.x;
                apply.corrShift[1] = shift.y;
                apply.correctionScale = m_dlssnrCorrectionScale[eye];
                apply.correctionSize[0] = m_dlssnrCorrectionSize[eye].w;
                apply.correctionSize[1] = m_dlssnrCorrectionSize[eye].h;
                DispatchCachePass(apply, srvTable, uavTable);
            };
            const auto Transition = [&](ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
                const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, from, to);
                cmdList.Commands->ResourceBarrier(1, &barrier);
            };

            if (cachedEye) {
                // Reuse the last NR correction on the new image instead of running NR.
                ApplyCorrectionPass(correctionShift[eye]);
            } else if (reducedResolution && !reducedEye) {
                // Unsupported format for the reduced resolution: show the region without NR.
                const D3D12_BOX box = {
                    (UINT)colorBox.left, (UINT)colorBox.top, 0, (UINT)colorBox.right, (UINT)colorBox.bottom, 1};
                CopyIntoOutput(inputImage, box, destBox.left, destBox.top);
                m_dlssnrCacheValid[eye] = false;
            } else {
                if (reducedEye) {
                    // Downscale the region to NR's working resolution.
                    DlssnrCacheConstants downscale = constants;
                    downscale.mode = Downscale;
                    downscale.dstOffset[0] = downscale.dstOffset[1] = 0;
                    downscale.size[0] = workSize.w;
                    downscale.size[1] = workSize.h;
                    downscale.correctionScale = workingScale;
                    DispatchCachePass(downscale, srvTable, workUavTable);
                    Transition(m_dlssnrWorkInput[eye].Get(),
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Color, m_dlssnrWorkInput[eye].Get());
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectBaseX, 0);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectBaseY, 0);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectWidth, workSize.w);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_ColorSubrectHeight, workSize.h);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Output, m_dlssnrWorkOutput[eye].Get());
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectBaseX, 0);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectBaseY, 0);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectWidth, workSize.w);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_OutputSubrectHeight, workSize.h);
                    // The game's depth is full-size: not used at a reduced resolution.
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_Depth, (ID3D12Resource*)nullptr);
                }

                // Motion vectors, from the frame NR last ran on to this one.
                const ovrVector2i region = {colorBox.left - inputBox[eye].left, colorBox.top - inputBox[eye].top};
                const ovrSizei regionSize = {colorBox.right - colorBox.left, colorBox.bottom - colorBox.top};
                const ovrSizei motionSize = reducedEye ? workSize : regionSize;
                const bool haveMotion = motionEnabled && RecordMotionPasses(eye,
                                                                            inputImage,
                                                                            inputBox[eye],
                                                                            region,
                                                                            regionSize,
                                                                            motionSize,
                                                                            reducedEye ? workingScale : 1.f);
                ID3D12Resource* const motionVectors = haveMotion ? m_dlssnrMotionVectors[eye].Get() : nullptr;
                m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVec, motionVectors);
                if (haveMotion) {
                    m_dlssnrStatMotion++;
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectBaseX, 0);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectBaseY, 0);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectWidth, motionSize.w);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecSubrectHeight, motionSize.h);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecScaleX, m_dlssnrMotionScale[0]);
                    m_ngxParameters->Set(NVSDK_NGX_Parameter_DLSSNR_MVecScaleY, m_dlssnrMotionScale[1]);
                    Transition(motionVectors,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                }

                // Go!
                CHECK_NGXCMD(ngx::NVSDK_NGX_D3D12_EvaluateFeature(
                    cmdList.Commands.Get(), m_dlssnrFeature[eye], m_ngxParameters, nullptr));

                if (haveMotion) {
                    Transition(motionVectors,
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }

                if (reducedEye) {
                    // Correction at the working resolution, then applied (upscaled) to the full-size region.
                    Transition(m_dlssnrWorkOutput[eye].Get(),
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    DlssnrCacheConstants store = constants;
                    store.mode = StoreCorrection;
                    store.srcOffset[0] = store.srcOffset[1] = store.dstOffset[0] = store.dstOffset[1] = 0;
                    store.size[0] = workSize.w;
                    store.size[1] = workSize.h;
                    store.inputIsLinear = 0;
                    DispatchCachePass(store, workSrvTable, workUavTable);
                    Transition(m_dlssnrWorkInput[eye].Get(),
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    Transition(m_dlssnrWorkOutput[eye].Get(),
                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    UavBarrier();
                    m_dlssnrCorrectionScale[eye] = workingScale;
                    m_dlssnrCorrectionSize[eye] = workSize;
                    ApplyCorrectionPass({0, 0});
                } else if (cacheEnabled && passesReady) {
                    // Keep the correction (NR output - NR input) for the next frame(s).
                    UavBarrier();
                    constants.mode = StoreCorrection;
                    DispatchCachePass(constants, srvTable, uavTable);
                    m_dlssnrCorrectionScale[eye] = 1.f;
                    m_dlssnrCorrectionSize[eye] = {constants.size[0], constants.size[1]};
                }
                if (cacheEnabled && passesReady) {
                    m_dlssnrCacheValid[eye] = true;
                    m_dlssnrCacheOffset[eye] = offset[eye];
                    m_dlssnrCacheOrientation[eye] = layer.RenderPose[eye].Orientation;
                } else {
                    m_dlssnrCacheValid[eye] = false;
                }
            }

            if (m_dlssnrDebugSplit) {
                // A/B view: the right half of the region shows the image without NR.
                const int half = (colorBox.right - colorBox.left) / 2;
                const D3D12_BOX box = {(UINT)(colorBox.left + half),
                                       (UINT)colorBox.top,
                                       0,
                                       (UINT)colorBox.right,
                                       (UINT)colorBox.bottom,
                                       1};
                CopyIntoOutput(inputImage, box, destBox.left + half, destBox.top);
            }

            if (m_dlssnrTransition > 0.f && passesReady) {
                // The region's edge fades from NR's output to the game's image (the transition width).
                DlssnrCacheConstants fade = constants;
                fade.mode = FadeEdge;
                std::memcpy(&fade.padding, &m_dlssnrTransition, sizeof(float));
                UavBarrier();
                DispatchCachePass(fade, srvTable, uavTable);
            }

            if (m_dlssnrDebugFovea && passesReady) {
                // Outline: green = NR ran, yellow = cached correction, orange = no gaze (centered or held).
                UavBarrier();
                constants.mode = DrawBorder;
                constants.borderWidth = 4;
                const float green[] = {0.f, 1.f, 0.f, 1.f}, yellow[] = {1.f, 1.f, 0.f, 1.f},
                            orange[] = {1.f, 0.5f, 0.f, 1.f};
                const float* color = cachedEye ? yellow : (followGaze && !gazeValid ? orange : green);
                std::copy(color, color + 4, constants.borderColor);
                DispatchCachePass(constants, srvTable, uavTable);
            }

            // This eye's writes are done before the other eye's.
            UavBarrier();
        }

        {
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                outputResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
            cmdList.Commands->ResourceBarrier(1, &barrier);
        }

        if (useCache) {
            m_dlssnrFramesSinceFullEval++;
        } else {
            m_dlssnrFramesSinceFullEval = 0;
        }

        // Measure the GPU time of frames where NR runs (for the adaptive cache).
        const bool timeThisFrame = !useCache && m_dlssnrGpuTimer && !m_dlssnrGpuTimerPending;
        if (timeThisFrame) {
            m_dlssnrGpuTimer->start();
        }

        // Serialize and commit output.
        const auto fenceValue = m_dlssnrContext->SubmitCommandList(cmdList);
        if (timeThisFrame) {
            m_dlssnrGpuTimer->stop();
            m_dlssnrGpuTimerPending = true;
        }
        CHECK_HRCMD(m_ovrSubmissionContext->Wait(m_dlssnrOutFence.Get(), fenceValue));
        CHECK_OVRCMD(ovr_CommitTextureSwapChain(m_ovrSession, m_dlssnrOutputSwapchain));

        // Patch the layer.
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            layer.ColorTexture[eye] = m_dlssnrOutputSwapchain;
            layer.Viewport[eye].Pos = {(int)eye * resolution.w, 0};
            layer.Viewport[eye].Size = resolution;
        }

        TraceLoggingWriteStop(local, "DLSSNR");
    }

    // For output swapchain resources.
    void OpenXrRuntime::ensureDlssnrSwapchainResources(ovrTextureFormat format, const ovrSizei& resolution) {
        if (m_dlssnrOutputSwapchain) {
            m_dlssnrOutputSwapchainImages.clear();
            ovr_DestroyTextureSwapChain(m_ovrSession, m_dlssnrOutputSwapchain);
        }

        ovrTextureSwapChainDesc desc{};
        desc.Type = ovrTexture_2D;
        desc.ArraySize = 1;
        desc.Width = resolution.w * 2; // Double-wide.
        desc.Height = resolution.h;
        desc.MipLevels = 1;
        desc.SampleCount = 1;
        desc.Format = format;
        desc.BindFlags = ovrTextureBind_DX_RenderTarget | ovrTextureBind_DX_UnorderedAccess;
        desc.MiscFlags = ovrTextureMisc_DX_Typeless;
        CHECK_OVRCMD(
            ovr_CreateTextureSwapChainDX(m_ovrSession, m_ovrSubmissionDevice.Get(), &desc, &m_dlssnrOutputSwapchain));

        int count = -1;
        CHECK_OVRCMD(ovr_GetTextureSwapChainLength(m_ovrSession, m_dlssnrOutputSwapchain, &count));

        // Query the textures for the swapchain.
        for (int i = 0; i < count; i++) {
            ComPtr<ID3D11Texture2D> texture;
            CHECK_OVRCMD(ovr_GetTextureSwapChainBufferDX(
                m_ovrSession, m_dlssnrOutputSwapchain, i, IID_PPV_ARGS(texture.ReleaseAndGetAddressOf())));

            // Export to the DLSS context.
            ComPtr<IDXGIResource1> dxgiResource;
            CHECK_HRCMD(texture->QueryInterface(IID_PPV_ARGS(dxgiResource.ReleaseAndGetAddressOf())));

            D3D11_TEXTURE2D_DESC desc{};
            texture->GetDesc(&desc);

            wil::unique_handle ntHandle;
            HANDLE textureHandle;
            if (!(desc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE)) {
                CHECK_HRCMD(dxgiResource->GetSharedHandle(&textureHandle));
            } else {
                CHECK_HRCMD(dxgiResource->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, ntHandle.put()));
                textureHandle = ntHandle.get();
            }
            ComPtr<ID3D12Resource> d3d12Resource;
            CHECK_HRCMD(
                m_dlssnrDevice->OpenSharedHandle(textureHandle, IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())));

            m_dlssnrOutputSwapchainImages.push_back(std::move(d3d12Resource));
        }
    }

    // For input swapchain(s) resources.
    void OpenXrRuntime::ensureSwapchainDlssnrResources(Swapchain& xrSwapchain, uint32_t slice) {
        if (xrSwapchain.resolvedSlices[slice].dlssnrImages.empty()) {
            // Query the textures for the swapchain.
            for (int i = 0; i < xrSwapchain.resolvedSlices[slice].images.size(); i++) {
                // Export to the DLSS context.
                wil::unique_handle handle;
                ComPtr<IDXGIResource1> dxgiResource;
                CHECK_HRCMD(xrSwapchain.resolvedSlices[slice].images[i]->QueryInterface(
                    IID_PPV_ARGS(dxgiResource.ReleaseAndGetAddressOf())));

                D3D11_TEXTURE2D_DESC desc{};
                xrSwapchain.resolvedSlices[slice].images[i]->GetDesc(&desc);

                wil::unique_handle ntHandle;
                HANDLE textureHandle;
                if (!(desc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE)) {
                    CHECK_HRCMD(dxgiResource->GetSharedHandle(&textureHandle));
                } else {
                    CHECK_HRCMD(dxgiResource->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, ntHandle.put()));
                    textureHandle = ntHandle.get();
                }
                ComPtr<ID3D12Resource> d3d12Resource;
                CHECK_HRCMD(m_dlssnrDevice->OpenSharedHandle(textureHandle,
                                                             IID_PPV_ARGS(d3d12Resource.ReleaseAndGetAddressOf())));

                xrSwapchain.resolvedSlices[slice].dlssnrImages.push_back(std::move(d3d12Resource));
            }
        }
    }

    void OpenXrRuntime::initializeDlssnrResources() {
        // DLSS-NR requires a D3D12 context. Create one on the matching adapter.
        ComPtr<IDXGIDevice> dxgiDevice;
        CHECK_HRCMD(m_ovrSubmissionDevice->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter> dxgiAdapter;
        CHECK_HRCMD(D3D12CreateDevice(
            dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_dlssnrDevice.ReleaseAndGetAddressOf())));

        m_dlssnrContext = std::make_unique<D3D12Utils::CommandContext>(m_dlssnrDevice.Get());

        // We'll use a fence to serialize pre-compositor output to DLSS-NR input.
        {
            wil::unique_handle fenceHandle;
            CHECK_HRCMD(m_ovrSubmissionFence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, fenceHandle.put()));
            CHECK_HRCMD(m_dlssnrDevice->OpenSharedHandle(fenceHandle.get(),
                                                         IID_PPV_ARGS(m_dlssnrInFence.ReleaseAndGetAddressOf())));
        }

        // We'll use a fence to serialize DLSS-NR output to OVR input.
        {
            wil::unique_handle handle;
            CHECK_HRCMD(m_dlssnrDevice->CreateSharedHandle(
                m_dlssnrContext->GetCompletionFence(), nullptr, GENERIC_ALL, nullptr, handle.put()));
            CHECK_HRCMD(m_ovrSubmissionDevice->OpenSharedFence(
                handle.get(), IID_PPV_ARGS(m_dlssnrOutFence.ReleaseAndGetAddressOf())));
        }

        NVSDK_NGX_FeatureCommonInfo commonInfo{};
        const wchar_t* paths[] = {virtualdesktop_openxr::dllHome.c_str()};
#ifdef _DEBUG
        commonInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_VERBOSE;
#else
        commonInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_ON;
#endif
        commonInfo.PathListInfo.Path = paths;
        commonInfo.PathListInfo.Length = std::size(paths);
        CHECK_NGXCMD(NVSDK_NGX_D3D12_Init(
            123456, virtualdesktop_openxr::programData.c_str(), m_dlssnrDevice.Get(), &commonInfo));

        CHECK_NGXCMD(NVSDK_NGX_D3D12_GetCapabilityParameters(&m_ngxParameters));

        // Optional: without these, NR still runs (no correction cache and no debug outline).
        try {
            initializeDlssnrCacheResources();
        } catch (std::exception& exc) {
            Log("DLSS-NR: correction cache unavailable: %s\n", exc.what());
            m_dlssnrCachePSO.Reset();
        }
        try {
            initializeDlssnrMotionResources();
        } catch (std::exception& exc) {
            Log("DLSS-NR: motion vectors unavailable: %s\n", exc.what());
            m_dlssnrMotionPSO.Reset();
        }
    }

    // For the motion vectors (luminance, optical flow, vectors).
    void OpenXrRuntime::initializeDlssnrMotionResources() {
        // Root signature: root constants (b0), SRV table (t0), UAV table (u0-u2).
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0);
        CD3DX12_ROOT_PARAMETER parameters[3];
        parameters[0].InitAsConstants(sizeof(DlssnrMotionConstants) / sizeof(uint32_t), 0);
        parameters[1].InitAsDescriptorTable(1, &ranges[0]);
        parameters[2].InitAsDescriptorTable(1, &ranges[1]);
        const CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(parameters), parameters);

        ComPtr<ID3DBlob> serialized, errors;
        CHECK_HRCMD(D3D12SerializeRootSignature(&rootSignatureDesc,
                                                D3D_ROOT_SIGNATURE_VERSION_1,
                                                serialized.ReleaseAndGetAddressOf(),
                                                errors.ReleaseAndGetAddressOf()));
        CHECK_HRCMD(m_dlssnrDevice->CreateRootSignature(0,
                                                        serialized->GetBufferPointer(),
                                                        serialized->GetBufferSize(),
                                                        IID_PPV_ARGS(m_dlssnrMotionRootSignature.ReleaseAndGetAddressOf())));

        const auto CreatePSO = [&](const void* code, size_t size, ComPtr<ID3D12PipelineState>& pso) {
            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = m_dlssnrMotionRootSignature.Get();
            psoDesc.CS = {code, size};
            CHECK_HRCMD(m_dlssnrDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso.ReleaseAndGetAddressOf())));
        };
        CreatePSO(g_DlssnrLumaCS, sizeof(g_DlssnrLumaCS), m_dlssnrLumaPSO);
        CreatePSO(g_DlssnrFlowCS, sizeof(g_DlssnrFlowCS), m_dlssnrFlowPSO);
        CreatePSO(g_DlssnrMotionCS, sizeof(g_DlssnrMotionCS), m_dlssnrMotionPSO);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = DlssnrMotionDescriptorsPerPass * DlssnrMotionDescriptorRing;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        CHECK_HRCMD(m_dlssnrDevice->CreateDescriptorHeap(&heapDesc,
                                                         IID_PPV_ARGS(m_dlssnrMotionHeap.ReleaseAndGetAddressOf())));
        m_dlssnrCacheDescriptorSize =
            m_dlssnrDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // NR's input and output at a reduced working resolution (one each per eye).
    void OpenXrRuntime::ensureDlssnrWorkResources(const ovrSizei& resolution, DXGI_FORMAT format) {
        if (m_dlssnrWorkInput[0] && m_dlssnrWorkFormat == format && m_dlssnrWorkResolution.w >= resolution.w &&
            m_dlssnrWorkResolution.h >= resolution.h) {
            return;
        }

        // Finish in-flight work before releasing the resources.
        m_dlssnrContext->Flush();

        const ovrSizei size = {std::max(resolution.w, m_dlssnrWorkFormat == format ? m_dlssnrWorkResolution.w : 0),
                               std::max(resolution.h, m_dlssnrWorkFormat == format ? m_dlssnrWorkResolution.h : 0)};
        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
            format, size.w, size.h, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            ComPtr<ID3D12Resource>* const resources[] = {std::addressof(m_dlssnrWorkInput[eye]),
                                                         std::addressof(m_dlssnrWorkOutput[eye])};
            for (auto* resource : resources) {
                CHECK_HRCMD(m_dlssnrDevice->CreateCommittedResource(&heapProperties,
                                                                    D3D12_HEAP_FLAG_NONE,
                                                                    &desc,
                                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                    nullptr,
                                                                    IID_PPV_ARGS(resource->ReleaseAndGetAddressOf())));
            }
            m_dlssnrCacheValid[eye] = false;
        }
        m_dlssnrWorkResolution = size;
        m_dlssnrWorkFormat = format;
    }

    // Luminance (quarter of the eye image, two per eye), flow (quarter) and motion vectors (foveated region).
    void OpenXrRuntime::ensureDlssnrMotionResources(const ovrSizei& imageResolution,
                                                    const ovrSizei& foveatedResolution) {
        const ovrSizei quarter = {(imageResolution.w + 3) / 4, (imageResolution.h + 3) / 4};
        if (m_dlssnrMotionVectors[0] && m_dlssnrLumaResolution.w >= quarter.w &&
            m_dlssnrLumaResolution.h >= quarter.h && m_dlssnrMotionVectorsResolution.w >= foveatedResolution.w &&
            m_dlssnrMotionVectorsResolution.h >= foveatedResolution.h) {
            return;
        }

        // Finish in-flight work before releasing the resources.
        m_dlssnrContext->Flush();

        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        const auto Create = [&](DXGI_FORMAT format, const ovrSizei& size, ComPtr<ID3D12Resource>& resource) {
            const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                format, size.w, size.h, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            CHECK_HRCMD(m_dlssnrDevice->CreateCommittedResource(&heapProperties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &desc,
                                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                nullptr,
                                                                IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())));
        };
        const ovrSizei lumaSize = {std::max(quarter.w, m_dlssnrLumaResolution.w),
                                   std::max(quarter.h, m_dlssnrLumaResolution.h)};
        const ovrSizei vectorsSize = {std::max(foveatedResolution.w, m_dlssnrMotionVectorsResolution.w),
                                      std::max(foveatedResolution.h, m_dlssnrMotionVectorsResolution.h)};
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            Create(DXGI_FORMAT_R16_FLOAT, lumaSize, m_dlssnrLuma[eye][0]);
            Create(DXGI_FORMAT_R16_FLOAT, lumaSize, m_dlssnrLuma[eye][1]);
            Create(DXGI_FORMAT_R16G16_FLOAT, lumaSize, m_dlssnrFlow[eye]);
            Create(DXGI_FORMAT_R16G16_FLOAT, vectorsSize, m_dlssnrMotionVectors[eye]);
            m_dlssnrHistoryValid[eye] = false;
        }
        m_dlssnrLumaResolution = lumaSize;
        m_dlssnrMotionVectorsResolution = vectorsSize;
    }

    // For the correction cache and the debug outline.
    void OpenXrRuntime::initializeDlssnrCacheResources() {
        // Root signature: root constants (b0), SRV table (t0-t1), UAV table (u0-u1).
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);
        CD3DX12_ROOT_PARAMETER parameters[3];
        parameters[0].InitAsConstants(sizeof(DlssnrCacheConstants) / sizeof(uint32_t), 0);
        parameters[1].InitAsDescriptorTable(1, &ranges[0]);
        parameters[2].InitAsDescriptorTable(1, &ranges[1]);
        const CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(parameters), parameters);

        ComPtr<ID3DBlob> serialized, errors;
        CHECK_HRCMD(D3D12SerializeRootSignature(&rootSignatureDesc,
                                                D3D_ROOT_SIGNATURE_VERSION_1,
                                                serialized.ReleaseAndGetAddressOf(),
                                                errors.ReleaseAndGetAddressOf()));
        CHECK_HRCMD(m_dlssnrDevice->CreateRootSignature(0,
                                                        serialized->GetBufferPointer(),
                                                        serialized->GetBufferSize(),
                                                        IID_PPV_ARGS(m_dlssnrCacheRootSignature.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = m_dlssnrCacheRootSignature.Get();
        psoDesc.CS = {g_DlssnrCacheCS, sizeof(g_DlssnrCacheCS)};
        CHECK_HRCMD(m_dlssnrDevice->CreateComputePipelineState(
            &psoDesc, IID_PPV_ARGS(m_dlssnrCachePSO.ReleaseAndGetAddressOf())));

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = DlssnrCacheDescriptorsPerPass * DlssnrCacheDescriptorRing;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        CHECK_HRCMD(m_dlssnrDevice->CreateDescriptorHeap(&heapDesc,
                                                         IID_PPV_ARGS(m_dlssnrCacheHeap.ReleaseAndGetAddressOf())));
        m_dlssnrCacheDescriptorSize =
            m_dlssnrDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_dlssnrGpuTimer = std::make_unique<D3D12GpuTimer>(m_dlssnrDevice.Get(), m_dlssnrContext->GetCommandQueue());
        m_dlssnrGpuTimerPending = false;
    }

    // One correction texture per eye, the size of the foveated region.
    void OpenXrRuntime::ensureDlssnrCorrectionResources(const ovrSizei& resolution) {
        if (m_dlssnrCorrection[0] && m_dlssnrCorrectionResolution.w >= resolution.w &&
            m_dlssnrCorrectionResolution.h >= resolution.h) {
            return;
        }

        // Finish in-flight work before releasing the resources.
        m_dlssnrContext->Flush();

        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT,
                                                       resolution.w,
                                                       resolution.h,
                                                       1,
                                                       1,
                                                       1,
                                                       0,
                                                       D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            CHECK_HRCMD(m_dlssnrDevice->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_PPV_ARGS(m_dlssnrCorrection[eye].ReleaseAndGetAddressOf())));
            m_dlssnrCorrection[eye]->SetName(L"DLSS-NR Correction");
            m_dlssnrCacheValid[eye] = false;
        }
        m_dlssnrCorrectionResolution = resolution;
    }

    void OpenXrRuntime::cleanupDlssnrResources() {
        if (m_ngxParameters) {
            NVSDK_NGX_D3D12_DestroyParameters(m_ngxParameters);
            m_ngxParameters = nullptr;
        }

        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            if (m_dlssnrFeature[eye]) {
                ngx::NVSDK_NGX_D3D12_ReleaseFeature(m_dlssnrFeature[eye]);
                m_dlssnrFeature[eye] = nullptr;
            }
        }
        m_dlssnrFeaturePreset = -1;

        m_dlssnrGpuTimer.reset();
        m_dlssnrGpuTimerPending = false;
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            m_dlssnrCorrection[eye].Reset();
            m_dlssnrCacheValid[eye] = false;
            m_dlssnrFoveaOffset[eye] = {-1, -1};
        }
        m_dlssnrCorrectionResolution = {0, 0};
        m_dlssnrCacheHeap.Reset();
        m_dlssnrCachePSO.Reset();
        m_dlssnrCacheRootSignature.Reset();

        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            m_dlssnrLuma[eye][0].Reset();
            m_dlssnrLuma[eye][1].Reset();
            m_dlssnrFlow[eye].Reset();
            m_dlssnrMotionVectors[eye].Reset();
            m_dlssnrHistoryValid[eye] = false;
        }
        m_dlssnrLumaResolution = m_dlssnrMotionVectorsResolution = {0, 0};
        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
            m_dlssnrWorkInput[eye].Reset();
            m_dlssnrWorkOutput[eye].Reset();
        }
        m_dlssnrWorkResolution = {0, 0};
        m_dlssnrWorkFormat = DXGI_FORMAT_UNKNOWN;
        m_dlssnrMotionHeap.Reset();
        m_dlssnrLumaPSO.Reset();
        m_dlssnrFlowPSO.Reset();
        m_dlssnrMotionPSO.Reset();
        m_dlssnrMotionRootSignature.Reset();

        // Don't shutdown NGX - the host app might be using DLSS!
    }

} // namespace virtualdesktop_openxr
