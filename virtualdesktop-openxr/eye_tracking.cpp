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
#include "utils.h"

// Implements the foundations of eye tracking needed for various extensions:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_eye_gaze_interaction
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_eye_tracking_social

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateEyeTrackerFB
    XrResult OpenXrRuntime::xrCreateEyeTrackerFB(XrSession session,
                                                 const XrEyeTrackerCreateInfoFB* createInfo,
                                                 XrEyeTrackerFB* eyeTracker) {
        if (createInfo->type != XR_TYPE_EYE_TRACKER_CREATE_INFO_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrCreateEyeTrackerFB", TLXArg(session, "Session"));

        if (!has_XR_FB_eye_tracking_social) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (m_eyeTrackingType != EyeTracking::Mmf) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        EyeTracker& xrEyeTracker = *new EyeTracker;

        *eyeTracker = (XrEyeTrackerFB)&xrEyeTracker;

        // Maintain a list of known trackers for validation.
        m_eyeTrackers.insert(*eyeTracker);

        TraceLoggingWrite(g_traceProvider, "xrCreateEyeTrackerFB", TLXArg(*eyeTracker, "EyeTracker"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyEyeTrackerFB
    XrResult OpenXrRuntime::xrDestroyEyeTrackerFB(XrEyeTrackerFB eyeTracker) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyEyeTrackerFB", TLXArg(eyeTracker, "EyeTracker"));

        if (!has_XR_FB_eye_tracking_social) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_eyeTrackers.count(eyeTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        EyeTracker* xrEyeTracker = (EyeTracker*)eyeTracker;

        delete xrEyeTracker;
        m_eyeTrackers.erase(eyeTracker);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetEyeGazesFB
    XrResult OpenXrRuntime::xrGetEyeGazesFB(XrEyeTrackerFB eyeTracker,
                                            const XrEyeGazesInfoFB* gazeInfo,
                                            XrEyeGazesFB* eyeGazes) {
        if (gazeInfo->type != XR_TYPE_EYE_GAZES_INFO_FB || eyeGazes->type != XR_TYPE_EYE_GAZES_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetEyeGazesFB",
                          TLXArg(eyeTracker, "EyeTracker"),
                          TLArg(gazeInfo->time),
                          TLXArg(gazeInfo->baseSpace));

        if (!has_XR_FB_eye_tracking_social) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::shared_lock lock(m_bodyTrackersMutex);
        std::shared_lock lock2(m_actionsAndSpacesMutex);

        if (!m_eyeTrackers.count(eyeTracker) || !m_spaces.count(gazeInfo->baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        // Forward the state from the memory mapped file.
        if (m_bodyState) {
            std::shared_lock lock(m_bodyStateMutex);

            eyeGazes->gaze[xr::Side::Left].gazeConfidence = m_cachedBodyState.LeftEyeConfidence;
            eyeGazes->gaze[xr::Side::Right].gazeConfidence = m_cachedBodyState.RightEyeConfidence;

            BodyTracking::Pose leftEyePose = m_cachedBodyState.LeftEyePose;
            BodyTracking::Pose rightEyePose = m_cachedBodyState.RightEyePose;
            XrPosef eyeGaze[] = {
                xr::math::Pose::MakePose(
                    XrQuaternionf{leftEyePose.orientation.x,
                                  leftEyePose.orientation.y,
                                  leftEyePose.orientation.z,
                                  leftEyePose.orientation.w},
                    XrVector3f{leftEyePose.position.x, leftEyePose.position.y, leftEyePose.position.z}),
                xr::math::Pose::MakePose(
                    XrQuaternionf{rightEyePose.orientation.x,
                                  rightEyePose.orientation.y,
                                  rightEyePose.orientation.z,
                                  rightEyePose.orientation.w},
                    XrVector3f{rightEyePose.position.x, rightEyePose.position.y, rightEyePose.position.z})};

            eyeGazes->gaze[xr::Side::Left].isValid = XR_FALSE;
            eyeGazes->gaze[xr::Side::Right].isValid = XR_FALSE;
            if (m_cachedBodyState.LeftEyeIsValid || m_cachedBodyState.RightEyeIsValid) {
                // TODO: Need optimization here, in all likelyhood, the caller is looking for eye gaze relative to VIEW
                // space, in which case we are doing 2 back-to-back getHmdPose() that are cancelling each other.
                Space& xrBaseSpace = *(Space*)gazeInfo->baseSpace;
                XrPosef headPose = Pose::Identity();
                XrPosef baseSpaceToVirtual = Pose::Identity();
                if (Pose::IsPoseValid(getHmdPose(gazeInfo->time, headPose, nullptr)) &&
                    Pose::IsPoseValid(
                        locateSpaceToOrigin(xrBaseSpace, gazeInfo->time, baseSpaceToVirtual, nullptr, nullptr))) {
                    // Combine the poses.
                    if (m_cachedBodyState.LeftEyeIsValid) {
                        eyeGazes->gaze[xr::Side::Left].gazePose = Pose::Multiply(
                            Pose::Multiply(eyeGaze[xr::Side::Left], headPose), Pose::Invert(baseSpaceToVirtual));
                        eyeGazes->gaze[xr::Side::Left].isValid = XR_TRUE;
                    }
                    if (m_cachedBodyState.RightEyeIsValid) {
                        eyeGazes->gaze[xr::Side::Right].gazePose = Pose::Multiply(
                            Pose::Multiply(eyeGaze[xr::Side::Right], headPose), Pose::Invert(baseSpaceToVirtual));
                        eyeGazes->gaze[xr::Side::Right].isValid = XR_TRUE;
                    }
                }
            }
        } else {
            eyeGazes->gaze[xr::Side::Left].isValid = eyeGazes->gaze[xr::Side::Right].isValid = XR_FALSE;
            eyeGazes->gaze[xr::Side::Left].gazeConfidence = eyeGazes->gaze[xr::Side::Right].gazeConfidence = 0.f;
            eyeGazes->gaze[xr::Side::Left].gazePose = eyeGazes->gaze[xr::Side::Right].gazePose = Pose::Identity();
        }

        // We do not do any extrapolation.
        eyeGazes->time = gazeInfo->time;

        TraceLoggingWrite(g_traceProvider,
                          "xrGetEyeGazesFB",
                          TLArg(!!eyeGazes->gaze[xr::Side::Left].isValid, "LeftValid"),
                          TLArg(eyeGazes->gaze[xr::Side::Left].gazeConfidence, "LeftConfidence"),
                          TLArg(xr::ToString(eyeGazes->gaze[xr::Side::Left].gazePose).c_str(), "LeftGazePose"),
                          TLArg(!!eyeGazes->gaze[xr::Side::Right].isValid, "RightValid"),
                          TLArg(eyeGazes->gaze[xr::Side::Right].gazeConfidence, "RightConfidence"),
                          TLArg(xr::ToString(eyeGazes->gaze[xr::Side::Right].gazePose).c_str(), "RightGazePose"),
                          TLArg(eyeGazes->time, "Time"));

        return XR_SUCCESS;
    }

    bool OpenXrRuntime::getEyeGaze(XrTime time, bool getStateOnly, XrVector3f& unitVector, XrTime& sampleTime) {
        if (m_eyeTrackingType == EyeTracking::Mmf) {
            std::shared_lock lock(m_bodyStateMutex);

            TraceLoggingWrite(g_traceProvider,
                              "VirtualDesktopEyeTracker",
                              TLArg(!!m_cachedBodyState.LeftEyeIsValid, "LeftValid"),
                              TLArg(m_cachedBodyState.LeftEyeConfidence, "LeftConfidence"),
                              TLArg(!!m_cachedBodyState.RightEyeIsValid, "RightValid"),
                              TLArg(m_cachedBodyState.RightEyeConfidence, "RightConfidence"));

            if (!(m_cachedBodyState.LeftEyeIsValid && m_cachedBodyState.RightEyeIsValid)) {
                return false;
            }
            if (!(m_cachedBodyState.LeftEyeConfidence > 0.5f && m_cachedBodyState.RightEyeConfidence > 0.5f)) {
                return false;
            }

            BodyTracking::Pose leftEyePose = m_cachedBodyState.LeftEyePose;
            BodyTracking::Pose rightEyePose = m_cachedBodyState.RightEyePose;
            XrPosef eyeGaze[] = {
                xr::math::Pose::MakePose(
                    XrQuaternionf{leftEyePose.orientation.x,
                                  leftEyePose.orientation.y,
                                  leftEyePose.orientation.z,
                                  leftEyePose.orientation.w},
                    XrVector3f{leftEyePose.position.x, leftEyePose.position.y, leftEyePose.position.z}),
                xr::math::Pose::MakePose(
                    XrQuaternionf{rightEyePose.orientation.x,
                                  rightEyePose.orientation.y,
                                  rightEyePose.orientation.z,
                                  rightEyePose.orientation.w},
                    XrVector3f{rightEyePose.position.x, rightEyePose.position.y, rightEyePose.position.z})};

            TraceLoggingWrite(g_traceProvider,
                              "VirtualDesktopEyeTracker",
                              TLArg(xr::ToString(eyeGaze[xr::Side::Left]).c_str(), "LeftGazePose"),
                              TLArg(xr::ToString(eyeGaze[xr::Side::Right]).c_str(), "RightGazePose"));

            // Average the poses from both eyes.
            const auto gaze =
                xr::math::LoadXrPose(xr::math::Pose::Slerp(eyeGaze[xr::Side::Left], eyeGaze[xr::Side::Right], 0.5f));
            const auto gazeProjectedPoint =
                DirectX::XMVector3Transform(DirectX::XMVectorSet(0.f, 0.f, -1.f, 1.f), gaze);

            unitVector = xr::math::Normalize(
                {gazeProjectedPoint.m128_f32[0], gazeProjectedPoint.m128_f32[1], gazeProjectedPoint.m128_f32[2]});

            sampleTime = time;

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

            point = {(float)pt.x / 1000.f, (float)pt.y / 1000.f};
            sampleTime = ovrTimeToXrTime(ovr_GetTimeInSeconds());

            unitVector = Normalize({point.x - 0.5f, 0.5f - point.y, -0.35f});

        } else {
            return false;
        }

        return true;
    }

    bool OpenXrRuntime::getEyeGaze(
        XrTime time, bool getStateOnly, XrVector3f& unitVector, XrTime& sampleTime, bool suppressBlinking) {
        // Clear the cache.
        const auto now = std::chrono::steady_clock::now();
        if ((now - m_lastGoodEyeTrackingData).count() >= 600'000'000) {
            m_lastGoodEyeGaze.reset();
        }

        bool result = getEyeGaze(time, getStateOnly, unitVector, sampleTime);
        if (result) {
            m_lastGoodEyeTrackingData = now;
            if (!getStateOnly) {
                m_lastGoodEyeGaze = unitVector;
            }
        }

        // To avoid warping during blinking, we use a reasonably recent cached gaze vector.
        bool useCache = false;
        if (suppressBlinking) {
            if (!result && m_lastGoodEyeGaze) {
                unitVector = m_lastGoodEyeGaze.value();
                result = useCache = true;
            }
        }

        TraceLoggingWrite(g_traceProvider,
                          "VirtualDesktopEyeTracker",
                          TLArg(result, "Valid"),
                          TLArg(useCache, "UsingCache"),
                          TLArg(xr::ToString(unitVector).c_str(), "GazeUnitVector"));

        return result;
    }

} // namespace virtualdesktop_openxr
