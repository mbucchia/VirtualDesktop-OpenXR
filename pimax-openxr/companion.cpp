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

struct RuntimeStatus {
    bool valid;

    float refreshRate;
    uint32_t resolutionWidth;
    uint32_t resolutionHeight;
    uint8_t fovLevel;
    float fov;
    float floorHeight;
    bool useParallelProjection;
    bool useSmartSmoothing;
    bool useLighthouseTracking;
    float fps;
};

extern "C" __declspec(dllexport) void WINAPI getRuntimeStatus(RuntimeStatus* status) {
    pimax_openxr::log::Log("Hello\n");

    pvrEnvHandle pvr;
    CHECK_PVRCMD(pvr_initialise(&pvr));

    pvrSessionHandle pvrSession;
    CHECK_PVRCMD(pvr_createSession(pvr, &pvrSession));

    pvrHmdInfo hmdInfo{};
    CHECK_PVRCMD(pvr_getHmdInfo(pvrSession, &hmdInfo));
    // Pimax 4K is the only device without canted displays.
    const bool hasCantedDisplays = !(hmdInfo.VendorId == 1155 && hmdInfo.ProductId == 33);

    pvrDisplayInfo displayInfo{};
    CHECK_PVRCMD(pvr_getEyeDisplayInfo(pvrSession, pvrEye_Left, &displayInfo));

    pvrEyeRenderInfo eyeInfo[xr::StereoView::Count];
    CHECK_PVRCMD(pvr_getEyeRenderInfo(pvrSession, pvrEye_Left, &eyeInfo[0]));
    CHECK_PVRCMD(pvr_getEyeRenderInfo(pvrSession, pvrEye_Right, &eyeInfo[1]));
    // Add the canting angle. All Pimax headsets with canted displays have a 10 degrees canting on each size.
    const auto fov = PVR::RadToDegree(atan(eyeInfo[1].Fov.RightTan)) + PVR::RadToDegree(atan(eyeInfo[0].Fov.LeftTan)) +
                     (hasCantedDisplays ? 20.0f : 0.0f);
    const auto useParallelProjection = hasCantedDisplays && !pvr_getIntConfig(pvrSession, "steamvr_use_native_fov", 0);

    pvrFovPort fovForResolution = eyeInfo[0].Fov;
    if (useParallelProjection) {
        // Shift FOV by 10 degrees. All Pimax headsets have a 10 degrees canting.
        const float angle = -PVR::DegreeToRad(10.f);
        fovForResolution.LeftTan = tan(atan(eyeInfo[0].Fov.LeftTan) - angle);
        fovForResolution.RightTan = tan(atan(eyeInfo[0].Fov.RightTan) + angle);
    }

    pvrSizei viewportSize;
    CHECK_PVRCMD(pvr_getFovTextureSize(pvrSession, pvrEye_Left, eyeInfo[0].Fov, 1.f, &viewportSize));

    status->refreshRate = displayInfo.refresh_rate;
    status->resolutionWidth = viewportSize.w;
    status->resolutionHeight = viewportSize.h;
    status->fovLevel = pvr_getIntConfig(pvrSession, "fov_level", 1);
    status->fov = fov;
    status->floorHeight = pvr_getFloatConfig(pvrSession, CONFIG_KEY_EYE_HEIGHT, 0.f);
    status->useParallelProjection = useParallelProjection;
    status->useSmartSmoothing = pvr_getIntConfig(pvrSession, "dbg_asw_enable", 0);
    status->useLighthouseTracking = pvr_getIntConfig(pvrSession, "enable_lighthouse_tracking", 0);
    status->fps = pvr_getFloatConfig(pvrSession, "client_fps", 0);

    status->valid = true;

    pvr_destroySession(pvrSession);
    pvr_shutdown(pvr);
}
