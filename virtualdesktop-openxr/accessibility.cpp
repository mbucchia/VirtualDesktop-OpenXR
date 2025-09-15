// MIT License
//
// Copyright(c) 2025 Microsoft Corp.
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

#include "accessibility.h"
#include "log.h"
#include "runtime.h"
#include "utils.h"

// Implements hooks to connect accessibility devices.

namespace {

    using namespace virtualdesktop_openxr;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    struct EmulatedControllerState {
        mutable std::shared_mutex mutex;

        bool enabled = false;
        bool active = false;
    };

    class AccessibilityHelperImpl : public AccessibilityHelper {
      public:
        AccessibilityHelperImpl(ovrSession ovrSession) : m_ovrSession(ovrSession) {
            // TODO: For testing, until we have a config file.
            m_controllerState[0].enabled = m_controllerState[1].enabled = true;
            m_controllerState[1].active = true;

            for (xr::side_t side = 0; side < xr::Side::Count; side++) {
                m_toGripPose[side] = m_toAimPose[side] = Pose::Identity();
            }

            if (m_controllerState[0].enabled || m_controllerState[1].enabled) {
                m_isRunning = true;
                m_inputThread = std::thread([this] { InputThread(); });
            }
        }

        ~AccessibilityHelperImpl() {
            m_isRunning = false;
            if (m_inputThread.joinable()) {
                m_inputThread.join();
            }
        }

        bool IsControllerEmulated(xr::side_t side) const override {
            if (side >= xr::Side::Count) {
                return false;
            }

            {
                std::shared_lock lock(m_controllerState[side].mutex);
                return m_controllerState[side].enabled;
            }
        }

        bool GetEmulatedDevicePose(xr::side_t side, double absTime, ovrPoseStatef* outDevicePose) override {
            {
                std::shared_lock lock(m_controllerState[side].mutex);
                if (side >= xr::Side::Count || !m_controllerState[side].enabled || !m_controllerState[side].active) {
                    return false;
                }
            }

            ZeroMemory(outDevicePose, sizeof(ovrPoseStatef));

            // TODO: Nothing here yet, just a quick demo of how to put the virtual controllers in front of the user.

            // Get the head pose.
            ovrPoseStatef headPoseState{};
            ovrTrackedDeviceType hmd = ovrTrackedDevice_HMD;
            const auto result = ovr_GetDevicePoses(m_ovrSession, &hmd, 1, absTime, &headPoseState);

            const auto headPose = ovrPoseToXrPose(headPoseState.ThePose);

            // Left or right 15cm, below 10cm, in front 35cm.
            const auto inFront =
                Pose::MakePose(XrVector3f{side == xr::Side::Left ? -0.15f : 0.15f, -0.1f, -0.35f}, XrVector3f{0, 0, 0});

            // By applying the inverse of the raw->grip pose, we are effectively aligning the pose with the forward
            // vector (head pose), which gives an "en garde" pose for our sword.
            const auto transformedPose = Pose::Invert(m_toGripPose[side]) * inFront * headPose;

            outDevicePose->ThePose = xrPoseToOvrPose(transformedPose);
            outDevicePose->TimeInSeconds = absTime;

            return true;
        }

        bool GetEmulatedInputState(xr::side_t side, ovrInputState* outInputState) override {
            {
                std::shared_lock lock(m_controllerState[side].mutex);
                if (side >= xr::Side::Count || !m_controllerState[side].enabled || !m_controllerState[side].active) {
                    return false;
                }
            }

            // This structure holds the state for both controller buttons, but the caller will recombine the state
            // correctly.
            ZeroMemory(outInputState, sizeof(ovrInputState));

            // TODO: Nothing here yet, for now, passthrough a single button so we can nagivate menus.

            // TODO: Until GameInput is here, we use the actual Touch controller buttons.
            ovrInputState inputState{};
            ovr_GetInputState(m_ovrSession, ovrControllerType_Touch, &inputState);

            outInputState->IndexTrigger[side] = inputState.IndexTrigger[side];
            outInputState->IndexTriggerNoDeadzone[side] = inputState.IndexTriggerNoDeadzone[side];
            outInputState->IndexTriggerRaw[side] = inputState.IndexTriggerRaw[side];

            return true;
        }

        void SendEmulatedHapticPulse(xr::side_t side, float frequency, float amplitude) override {
            // Do nothing.
        }

        void SetOpenXrPoses(xr::side_t side, const XrPosef& rawToGrip, const XrPosef& rawToAim) override {
            if (side >= xr::Side::Count) {
                return;
            }

            m_toGripPose[side] = rawToGrip;
            m_toAimPose[side] = rawToAim;
        }

      private:
        void InputThread() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

            while (m_isRunning.load()) {
                // TODO: Until GameInput is here, we use the actual Touch controller buttons.
                ovrInputState inputState{};
                ovr_GetInputState(m_ovrSession, ovrControllerType_Touch, &inputState);

                {
                    std::unique_lock lock1(m_controllerState[0].mutex), lock2(m_controllerState[1].mutex);

                    // We use the grip button to switch between left/right (or both) being active.
                    const bool wasLeftActive = m_controllerState[0].active;
                    const bool wasRightActive = m_controllerState[1].active;
                    m_controllerState[0].active = inputState.HandTrigger[0] > 0.25f;
                    m_controllerState[1].active = inputState.HandTrigger[1] > 0.25f;

                    // Always leave one controller active.
                    if (!m_controllerState[0].active && !m_controllerState[1].active) {
                        m_controllerState[0].active = wasLeftActive;
                        m_controllerState[1].active = wasRightActive;
                    }
                }

                // TODO: Once we use a better API, we should not make this thread free-running.
                std::this_thread::yield();
            }
        }

        const ovrSession m_ovrSession;
        EmulatedControllerState m_controllerState[xr::Side::Count];
        std::thread m_inputThread;
        std::atomic<bool> m_isRunning;

        XrPosef m_toGripPose[xr::Side::Count];
        XrPosef m_toAimPose[xr::Side::Count];
    };

} // namespace

namespace virtualdesktop_openxr {

    std::unique_ptr<AccessibilityHelper> CreateAccessibilityHelper(ovrSession ovrSession) {
        return std::make_unique<AccessibilityHelperImpl>(ovrSession);
    }

} // namespace virtualdesktop_openxr
