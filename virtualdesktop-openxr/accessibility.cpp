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
    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    using namespace std::chrono_literals;

    // The interval we will poll for inputs from GameInput.
    static constexpr auto k_PollingInterval = 2ms;

    struct EmulatedControllerState {
        mutable std::shared_mutex mutex;

        // Whether we will emulate this controller.
        bool enabled = false;

        // Whether the controller is following gaze or now.
        bool followGaze = false;

        // Latest pose reported to the runtime.
        std::optional<XrPosef> latestReportedPose;
    };

    class AccessibilityHelperImpl : public AccessibilityHelper {
      public:
        AccessibilityHelperImpl(ovrSession ovrSession,
                                const std::wstring& configPath,
                                const std::string& applicationName)
            : m_ovrSession(ovrSession) {
            // Parse our configuration.
            {
                std::ifstream inputFile(configPath.c_str(), std::ios::in);
                std::string config;
                if (inputFile.is_open()) {
                    std::ostringstream sstr;
                    sstr << inputFile.rdbuf();
                    config = sstr.str();
                }

                cJSON* json = nullptr;
                try {
                    json = cJSON_ParseWithLength(config.c_str(), config.size());
                    if (!json) {
                        throw std::runtime_error("Failed to parse JSON");
                    }

                    ParseConfiguration(json, applicationName);

                } catch (std::runtime_error& exc) {
                    Log("Error parsing configuration file %ws: %s", configPath.c_str(), exc.what());
                }
                cJSON_Delete(json);
            }

            // Always start with one controller following gaze (if possible).
            // TODO: We disable this for now because it complicates re-centering. We will show hands at the time of
            // first A/B input.
#if 0
            if (m_controllerState[1].enabled) {
                m_controllerState[1].followGaze = true;
            } else if (m_controllerState[0].enabled) {
                m_controllerState[0].followGaze = true;
            }
#endif

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
            CHECK_MSG(side < xr::Side::Count, "Invalid controller");

            {
                std::shared_lock lock(m_controllerState[side].mutex);

                // Returning false here tells the runtime to use the physical controller (if available).
                return m_controllerState[side].enabled;
            }
        }

        bool GetEmulatedDevicePose(xr::side_t side, double absTime, ovrPoseStatef* outDevicePose) override {
            CHECK_MSG(side < xr::Side::Count, "Invalid controller");
            outDevicePose->TimeInSeconds = absTime;

            {
                std::shared_lock lock(m_controllerState[side].mutex);

                if (!m_controllerState[side].enabled) {
                    // Returning false here tells the runtime to mark the controller as not tracked and not valid.
                    // NOTE: This doesn't mean that the controller will "disappear", some applications might retain the
                    // most recent pose and continue to use it.
                    return false;
                }

                // If the other controller is visible but not this one, let's make sure we spawn it anyway.
                const bool shouldSpawn =
                    m_controllerState[side ^ 1].latestReportedPose && !m_controllerState[side].latestReportedPose;

                if (!m_controllerState[side].followGaze && !shouldSpawn) {
                    if (!m_controllerState[side].latestReportedPose) {
                        return false;
                    } else {
                        // TODO: Return last good pose? Or a "parking" pose out of the screen?
                        outDevicePose->ThePose = xrPoseToOvrPose(m_controllerState[side].latestReportedPose.value());
                        return true;
                    }
                }
            }

            ZeroMemory(outDevicePose, sizeof(ovrPoseStatef));

            // DEMO CODE: Move the emulated controllers in front of the user.

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

            // Store the last reported pose. We can use it as a starting point for pre-recorded sequences.
            {
                std::unique_lock lock(m_controllerState[side].mutex);

                m_controllerState[side].latestReportedPose = transformedPose;
            }

            return true;
        }

        bool GetEmulatedInputState(xr::side_t side, ovrInputState* outInputState) override {
            CHECK_MSG(side < xr::Side::Count, "Invalid controller");

            {
                std::shared_lock lock(m_controllerState[side].mutex);

                if (!m_controllerState[side].enabled || !m_controllerState[side].followGaze) {
                    // Returning false here tells the runtime to set all inputs as inactive.
                    return false;
                }
            }

            // This structure holds the state for both controller buttons, but the caller will recombine the state
            // correctly based on which controller is real or emulated.
            ZeroMemory(outInputState, sizeof(ovrInputState));

            // DEMO CODE: Passthrough the trigger (so we can click in menus).
            outInputState->IndexTrigger[side] = outInputState->IndexTriggerNoDeadzone[side] =
                outInputState->IndexTriggerRaw[side] = m_controllerInputState.IndexTrigger[m_dominantHand];

            return true;
        }

        void SendEmulatedHapticPulse(xr::side_t side, float frequency, float amplitude) override {
            CHECK_MSG(side < xr::Side::Count, "Invalid controller");

            // Do nothing.
        }

        void SetOpenXrPoses(xr::side_t side, const XrPosef& rawToGrip, const XrPosef& rawToAim) override {
            CHECK_MSG(side < xr::Side::Count, "Invalid controller");

            m_toGripPose[side] = rawToGrip;
            m_toAimPose[side] = rawToAim;
        }

      private:
        void InputThread() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

            double lastOvrTime = ovr_GetTimeInSeconds();
            while (m_isRunning.load()) {
                const auto nextInterval = std::chrono::high_resolution_clock::now() + k_PollingInterval;

                // TODO: Until GameInput is here, we use the actual Touch controller buttons.
                ovr_GetInputState(m_ovrSession, ovrControllerType_Touch, &m_controllerInputState);

                // We will use this time to latch the start time of an animation, so we can replay data timely. This is
                // the same clock that is passed to GetEmulatedDevicePose()'s absTime.
                const double ovrNow = ovr_GetTimeInSeconds();
                const double deltaTime = ovrNow - lastOvrTime;

                {
                    std::unique_lock lock1(m_controllerState[0].mutex), lock2(m_controllerState[1].mutex);

                    // DEMO CODE: Use the A/B button to switch between left/right (or both) being followGaze.
                    const bool wasLeftFollowingGaze = m_controllerState[0].followGaze;
                    const bool wasRightFollowingGaze = m_controllerState[1].followGaze;
                    m_controllerState[0].followGaze =
                        m_controllerInputState.Buttons & ((m_dominantHand == 0) ? ovrButton_X : ovrButton_A);
                    m_controllerState[1].followGaze =
                        m_controllerInputState.Buttons & ((m_dominantHand == 0) ? ovrButton_Y : ovrButton_B);

                    // Always leave at least one controller following gaze.
                    if (!m_controllerState[0].followGaze && !m_controllerState[1].followGaze) {
                        m_controllerState[0].followGaze = wasLeftFollowingGaze;
                        m_controllerState[1].followGaze = wasRightFollowingGaze;
                    }

                    // DEMO CODE: Use the joystick input to "move" the other controller (the one not following gaze).
                    if (!(m_controllerState[0].followGaze && m_controllerState[1].followGaze)) {
                        const xr::side_t otherSide =
                            !m_controllerState[0].followGaze ? xr::Side::Left : xr::Side::Right;
                        if (m_controllerState[otherSide].latestReportedPose) {
                            m_controllerState[otherSide].latestReportedPose.value().position.x +=
                                (float)(m_controllerInputState.Thumbstick[m_dominantHand].x *
                                        m_joystickHorizontalSensitivity * deltaTime);
                        }
                    }
                }

                // Record the last time the inputs were polled, so we can scale inputs with time.
                lastOvrTime = ovrNow;

                std::this_thread::sleep_until(nextInterval);
            }
        }

        void ParseConfiguration(cJSON* json, const std::string& applicationName) {
            // Try to use game-specific config first, otherwise fallback to default.
            const cJSON* top = cJSON_GetObjectItemCaseSensitive(json, applicationName.c_str());
            if (!top) {
                top = cJSON_GetObjectItemCaseSensitive(json, "default");
            }

            if (!top) {
                throw std::runtime_error("Failed to get top-level configuration item");
            }

            const auto emulateLeft = cJSON_GetObjectItemCaseSensitive(top, "emulate_left");
            m_controllerState[0].enabled = emulateLeft && emulateLeft->valueint;
            const auto emulateRight = cJSON_GetObjectItemCaseSensitive(top, "emulate_right");
            m_controllerState[1].enabled = emulateRight && emulateRight->valueint;
            const auto dominantHand = cJSON_GetObjectItemCaseSensitive(top, "dominant_hand");
            if (dominantHand) {
                m_dominantHand = std::min(1, dominantHand->valueint);
            }
            const auto joystickHorizontalSensitivity =
                cJSON_GetObjectItemCaseSensitive(top, "joystick_horizontal_sensitivity");
            if (joystickHorizontalSensitivity) {
                m_joystickHorizontalSensitivity = (float)joystickHorizontalSensitivity->valuedouble;
            }
        }

        const ovrSession m_ovrSession;
        EmulatedControllerState m_controllerState[xr::Side::Count];
        std::thread m_inputThread;
        std::atomic<bool> m_isRunning;

        ovrInputState m_controllerInputState{};
        xr::side_t m_dominantHand = xr::Side::Right;
        float m_joystickHorizontalSensitivity = 0.1f; // m/s at full joystick swing.

        // OVR to OpenXR poses. Useful if we want to emulate a pose relative to the standard grip or aim pose.
        XrPosef m_toGripPose[xr::Side::Count];
        XrPosef m_toAimPose[xr::Side::Count];
    };

} // namespace

namespace virtualdesktop_openxr {
    std::unique_ptr<AccessibilityHelper> CreateAccessibilityHelper(ovrSession ovrSession,
                                                                   const std::wstring& configPath,
                                                                   const std::string& applicationName) {
        return std::make_unique<AccessibilityHelperImpl>(ovrSession, configPath, applicationName);
    }

} // namespace virtualdesktop_openxr
