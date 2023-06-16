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

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;
    using namespace xr::math;

    bool OpenXrRuntime::getEyeGaze(XrTime time, bool getStateOnly, XrVector3f& unitVector, double& sampleTime) const {
        if (!m_isEyeTrackingAvailable) {
            return false;
        }

        if (m_eyeTrackingType == EyeTracking::PVR) {
            pvrEyeTrackingInfo state{};
            CHECK_PVRCMD(pvr_getEyeTrackingInfo(m_pvrSession, xrTimeToPvrTime(time), &state));
            TraceLoggingWrite(g_traceProvider,
                              "PVR_EyeTrackerPoseState",
                              TLArg(xr::ToString(state.GazeTan[xr::StereoView::Left]).c_str(), "LeftGaze"),
                              TLArg(xr::ToString(state.GazeTan[xr::StereoView::Right]).c_str(), "RightGaze"),
                              TLArg(state.TimeInSeconds, "TimeInSeconds"));

            // According to Pimax, this is how we detect gaze not valid.
            if (state.TimeInSeconds == 0) {
                return false;
            }

            // Compute the gaze pitch/yaw angles by averaging both eyes.
            // TODO: Find the convergence point instead.
            const float angleHorizontal =
                atan((state.GazeTan[xr::StereoView::Left].x + state.GazeTan[xr::StereoView::Right].x) / 2.f);
            const float angleVertical =
                atan((state.GazeTan[xr::StereoView::Left].y + state.GazeTan[xr::StereoView::Right].y) / 2.f);

            // Use polar coordinates to create a unit vector.
            unitVector = {
                sin(angleHorizontal) * cos(angleVertical),
                -sin(angleVertical),
                -cos(angleHorizontal) * cos(angleVertical),
            };

            sampleTime = state.TimeInSeconds;

        } else if (m_eyeTrackingType == EyeTracking::aSeeVR || m_eyeTrackingType == EyeTracking::Simulated) {
            XrVector2f point{0.5f, 0.5f};
#ifndef NOASEEVRCLIENT
            if (m_eyeTrackingType == EyeTracking::aSeeVR) {
                std::unique_lock lock(m_droolonMutex);

                TraceLoggingWrite(g_traceProvider,
                                  "aSeeVR_EyeTrackerState",
                                  TLArg(m_isDroolonReady, "Ready"),
                                  TLArg(xr::ToString(m_droolonGaze).c_str(), "Gaze"),
                                  TLArg(m_droolonTimestamp, "Timestamp"));

                if (!m_isDroolonReady) {
                    return false;
                }

                point = m_droolonGaze;
                sampleTime = m_droolonTimestamp;
            } else
#endif
            {
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
                sampleTime = pvr_getTimeSeconds(m_pvr);
            }

            // Experimentally determined that Z should be 0.35m in front for Droolon.
            unitVector = Normalize({point.x - 0.5f, 0.5f - point.y, -m_droolonProjectionDistance});

        } else {
            return false;
        }

        return true;
    }

#ifndef NOASEEVRCLIENT
    bool OpenXrRuntime::initializeDroolon() {
        m_isDroolonReady = false;
        aSeeVRInitParam param;
        param.ports[0] = getSetting("droolon_port").value_or(5347);
        {
            TraceLocalActivity(connect);
            TraceLoggingWriteStart(connect, "aSeeVRClient", TLArg("Connect", "Operation"));
            const aSeeVRReturnCode status = aSeeVR_connect_server(&param);
            TraceLoggingWriteStop(
                connect, "aSeeVRClient", TLArg("Connect", "Operation"), TLArg(xr::ToString(status).c_str(), "Status"));
            if (status != aSeeVRReturnCode::success) {
                Log(fmt::format("Failed to connect to Droolon service: {}\n", xr::ToString(status).c_str()).c_str());
                return false;
            }
        }

        aSeeVR_register_callback(aSeeVRCallbackType::coefficient, aSeeVRgetCoefficientCallback, this);
        aSeeVR_register_callback(aSeeVRCallbackType::state, aSeeVRstateCallback, this);
        aSeeVR_register_callback(aSeeVRCallbackType::eye_data, aSeeVReyeDataCallback, this);
        return true;
    }

    void OpenXrRuntime::startDroolonTracking() {
        TraceLoggingWrite(g_traceProvider, "aSeeVRClient", TLArg("GetCoefficient", "Operation"));
        const aSeeVRReturnCode status = aSeeVR_get_coefficient();
        if (status != aSeeVRReturnCode::success) {
            TraceLoggingWrite(g_traceProvider,
                              "aSeeVRClient",
                              TLArg("GetCoefficient", "Operation"),
                              TLArg(xr::ToString(status).c_str(), "Status"));
        }
    }

    void OpenXrRuntime::stopDroolonTracking() {
        TraceLoggingWrite(g_traceProvider, "aSeeVRClient", TLArg("Stop", "Operation"));
        const aSeeVRReturnCode status = aSeeVR_stop();
        if (status != aSeeVRReturnCode::success) {
            TraceLoggingWrite(g_traceProvider,
                              "aSeeVRClient",
                              TLArg("Stop", "Operation"),
                              TLArg(xr::ToString(status).c_str(), "Status"));
        }
    }

    void OpenXrRuntime::setDroolonCoefficients(const aSeeVRCoefficient& coefficients) {
        m_droolonCoefficients = coefficients;
        TraceLoggingWrite(g_traceProvider, "aSeeVRClient", TLArg("Start", "Operation"));
        const aSeeVRReturnCode status = aSeeVR_start(&m_droolonCoefficients);
        if (status != aSeeVRReturnCode::success) {
            TraceLoggingWrite(g_traceProvider,
                              "aSeeVRClient",
                              TLArg("Start", "Operation"),
                              TLArg(xr::ToString(status).c_str(), "Status"));
            Log(fmt::format("Failed to start Droolon eye tracking: {}", xr::ToString(status).c_str()).c_str());
        }
    }

    void OpenXrRuntime::setDroolonReady(bool ready) {
        std::unique_lock lock(m_droolonMutex);

        m_isDroolonReady = ready;
    }

    void OpenXrRuntime::setDroolonData(int64_t timestamp, const XrVector2f& gaze) {
        std::unique_lock lock(m_droolonMutex);

        // There is no direct translation between the timestamp from the eye tracking service and the rest of the
        // system. We capture the "time of arrival" as a best effort.
        m_droolonTimestamp = pvr_getTimeSeconds(m_pvr);
        m_droolonGaze = gaze;
    }

    void OpenXrRuntime::aSeeVRgetCoefficientCallback(const aSeeVRCoefficient* data, void* context) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "aSeeVRgetCoefficientCallback", TLPArg(data), TLPArg(context));

        OpenXrRuntime* const runtime = reinterpret_cast<OpenXrRuntime*>(context);

        TraceLoggingWrite(g_traceProvider,
                          "aSeeVRClient",
                          TLArg("GetCoefficient", "Operation"),
                          TLArg(xr::ToString(aSeeVRReturnCode::success).c_str(), "Status"));
        runtime->setDroolonCoefficients(*data);

        TraceLoggingWriteStop(local, "aSeeVRgetCoefficientCallback");
    }

    void OpenXrRuntime::aSeeVRstateCallback(const aSeeVRState* state, void* context) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "aSeeVRstateCallback", TLPArg(state), TLPArg(context));

        OpenXrRuntime* const runtime = reinterpret_cast<OpenXrRuntime*>(context);

        switch (state->code) {
        case aSeeVRStateCode::api_start:
            TraceLoggingWrite(g_traceProvider,
                              "aSeeVRClient",
                              TLArg("Start", "Operation"),
                              TLArg(xr::ToString((aSeeVRReturnCode)state->error).c_str(), "Status"));
            if (state->error == aSeeVRReturnCode::success) {
                runtime->setDroolonReady(true);
            } else {
                Log(fmt::format("Failed to start Droolon eye tracking: {}", xr::ToString(state->error).c_str())
                        .c_str());
                runtime->setDroolonReady(false);
            }
            break;

        case aSeeVRStateCode::api_stop:
            TraceLoggingWrite(g_traceProvider,
                              "aSeeVRClient",
                              TLArg("Stop", "Operation"),
                              TLArg(xr::ToString((aSeeVRReturnCode)state->error).c_str(), "Status"));
            runtime->setDroolonReady(false);
            break;

        default:
            break;
        }

        TraceLoggingWriteStop(local, "aSeeVRstateCallback");
    }

    void OpenXrRuntime::aSeeVReyeDataCallback(const aSeeVREyeData* eyeData, void* context) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "aSeeVReyeDataCallback", TLPArg(eyeData), TLPArg(context));

        OpenXrRuntime* const runtime = reinterpret_cast<OpenXrRuntime*>(context);

        if (!eye_data) {
            return;
        }

        int64_t timestamp = 0;
        aSeeVR_get_int64(eyeData, aSeeVREye::undefine_eye, aSeeVREyeDataItemType::timestamp, &timestamp);

        aSeeVRPoint2D point2D = {0};
        aSeeVR_get_point2d(eyeData, aSeeVREye::undefine_eye, aSeeVREyeDataItemType::gaze, &point2D);

        runtime->setDroolonData(timestamp, XrVector2f{point2D.x, point2D.y});

        TraceLoggingWriteStop(local, "aSeeVReyeDataCallback");
    }
#endif

} // namespace pimax_openxr
