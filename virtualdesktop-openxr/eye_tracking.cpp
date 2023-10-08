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

// Implements the foundations of eye tracking needed for various extensions.

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    bool OpenXrRuntime::initializeEyeTrackingMmf() {
        *m_faceStateFile.put() = OpenFileMapping(FILE_MAP_READ, false, L"VirtualDesktop.FaceState");
        if (!m_faceStateFile) {
            TraceLoggingWrite(g_traceProvider, "VirtualDesktopEyeTracker_NotAvailable");
            return false;
        }

        m_faceState = reinterpret_cast<FaceTracking::FaceState*>(
            MapViewOfFile(m_faceStateFile.get(), FILE_MAP_READ, 0, 0, sizeof(FaceTracking::FaceState)));
        if (!m_faceState) {
            TraceLoggingWrite(g_traceProvider, "VirtualDesktopEyeTracker_MappingError");
            return false;
        }

        return true;
    }

    bool OpenXrRuntime::getEyeGaze(XrTime time, bool getStateOnly, XrVector3f& unitVector, double& sampleTime) const {
        if (m_eyeTrackingType == EyeTracking::Mmf) {
            TraceLoggingWrite(g_traceProvider,
                              "VirtualDesktopEyeTracker",
                              TLArg(!!m_faceState->LeftEyeIsValid, "LeftValid"),
                              TLArg(m_faceState->LeftEyeConfidence, "LeftConfidence"),
                              TLArg(!!m_faceState->RightEyeIsValid, "RightValid"),
                              TLArg(m_faceState->RightEyeConfidence, "RightConfidence"));

            if (!(m_faceState->LeftEyeIsValid && m_faceState->RightEyeIsValid)) {
                return false;
            }
            if (!(m_faceState->LeftEyeConfidence > 0.5f && m_faceState->RightEyeConfidence > 0.5f)) {
                return false;
            }

            // TODO: Any file locking scheme?
            FaceTracking::Pose leftEyePose = m_faceState->LeftEyePose;
            FaceTracking::Pose rightEyePose = m_faceState->RightEyePose;
            XrPosef eyeGaze[] = {
                xr::math::Pose::MakePose(
                    XrQuaternionf{leftEyePose.orientation.x,
                                  leftEyePose.orientation.y,
                                  leftEyePose.orientation.z,
                                  leftEyePose.orientation.w},
                    XrVector3f{leftEyePose.position.x, leftEyePose.position.y, leftEyePose.position.z}),
                xr::math::Pose::MakePose(
                    XrQuaternionf{leftEyePose.orientation.x,
                                  leftEyePose.orientation.y,
                                  leftEyePose.orientation.z,
                                  leftEyePose.orientation.w},
                    XrVector3f{leftEyePose.position.x, leftEyePose.position.y, leftEyePose.position.z})};

            TraceLoggingWrite(g_traceProvider,
                              "VirtualDesktopEyeTracker",
                              TLArg(xr::ToString(eyeGaze[xr::StereoView::Left]).c_str(), "LeftGazePose"),
                              TLArg(xr::ToString(eyeGaze[xr::StereoView::Right]).c_str(), "RightGazePose"));

            // Average the poses from both eyes.
            const auto gaze = xr::math::LoadXrPose(
                xr::math::Pose::Slerp(eyeGaze[xr::StereoView::Left], eyeGaze[xr::StereoView::Right], 0.5f));
            const auto gazeProjectedPoint =
                DirectX::XMVector3Transform(DirectX::XMVectorSet(0.f, 0.f, -1.f, 1.f), gaze);

            unitVector = xr::math::Normalize(
                {gazeProjectedPoint.m128_f32[0], gazeProjectedPoint.m128_f32[1], gazeProjectedPoint.m128_f32[2]});

        } else if (m_eyeTrackingType == EyeTracking::Simulated) {
            XrVector2f point{0.5f, 0.5f};

            // Use the mouse to simulate eye tracking.
            RECT rect;
            rect.left = 1;
            rect.right = 999;
            rect.top = 1;
            rect.bottom = 999;
            ClipCursor(&rect);

            POINT pt{};
            GetCursorPos(&pt);

            point = {(float)pt.x / 1000.f, (1000.f - pt.y) / 1000.f};
            sampleTime = ovr_GetTimeInSeconds();

            unitVector = Normalize({point.x - 0.5f, 0.5f - point.y, -0.35f});

        } else {
            return false;
        }

        return true;
    }

} // namespace virtualdesktop_openxr
