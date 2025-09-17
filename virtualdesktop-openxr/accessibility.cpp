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

    struct PosePlayback {
        // Whether to reset to grip pose prior to starting the animation. Useful when a controller is in gripAsAim mode.
        bool startFromGrip = false;

        // A time-series of relative poses.
        std::vector<std::pair<double, XrPosef>> poses;
    };

    struct EmulatedControllerState {
        mutable std::shared_mutex mutex;

        // Whether we will emulate this controller.
        bool enabled = false;

        // Whether to make the grip pose akin to an aim pose. For example, if holding a sword, gripAsAim will make the
        // virtual controller hold the sword pointing forward ("en garde").
        bool gripAsAim = false;

        // Whether the controller is following gaze or now.
        bool followGaze = false;

        // Latest pose reported to the runtime.
        std::optional<XrPosef> latestReportedPose;

        // The current animation.
        const PosePlayback* animation = nullptr;

        // The start time for the playback.
        double animationStartTime = -1;

        // The current base frame for the playback.
        size_t animationFrame = -1;

        // An offset to apply to the running animation.
        XrPosef poseAnimationOffset = Pose::Identity();
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

            XrPosef finalPose = Pose::Identity();
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
                        // TODO: Return last good pose? Or a "parking" pose out of the screen? Do we also need to apply
                        // the animation here?
                        outDevicePose->ThePose = xrPoseToOvrPose(m_controllerState[side].latestReportedPose.value());
                        return true;
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
                const auto inFront = Pose::MakePose(XrVector3f{side == xr::Side::Left ? -0.15f : 0.15f, -0.1f, -0.35f},
                                                    XrVector3f{0, 0, 0});

                // Either leave as grip, or apply transform into aim.
                finalPose = inFront * headPose;
                if (m_controllerState[side].gripAsAim) {
                    finalPose = Pose::Invert(m_toGripPose[side]) * finalPose;
                }

                // DEMO CODE: Replay a pre-recorded sequence (animation).

                if (m_controllerState[side].animation) {
                    const auto currentPlaybackTime = absTime - m_controllerState[side].animationStartTime;

                    while (m_controllerState[side].animationFrame < m_controllerState[side].animation->poses.size() &&
                           m_controllerState[side].animation->poses[m_controllerState[side].animationFrame].first <
                               currentPlaybackTime) {
                        m_controllerState[side].animationFrame++;
                    }

                    const auto currentFrameIndex = m_controllerState[side].animationFrame;
                    const auto nextFrameIndex = m_controllerState[side].animationFrame + 1;
                    if (nextFrameIndex >= m_controllerState[side].animation->poses.size()) {
                        Log("Reached end of animation %s side\n", side == xr::Side::Left ? "left" : "right");
                        m_controllerState[side].animation = nullptr;
                    } else {
                        const auto currentFrame = m_controllerState[side].animation->poses[currentFrameIndex];
                        const auto currentTimeStamp = currentFrame.first;
                        const auto currentPose = currentFrame.second;

                        // Interpolate between this frame and the next
                        const auto nextFrame = m_controllerState[side].animation->poses[nextFrameIndex];
                        const auto nextFrameTimeStamp = nextFrame.first;
                        const auto nextFramePose = nextFrame.second;

                        const float alpha =
                            (float)((currentPlaybackTime - currentTimeStamp) / (nextFrameTimeStamp - currentTimeStamp));
                        finalPose = xr::math::Pose::Slerp(currentPose, nextFramePose, alpha) *
                                    m_controllerState[side].poseAnimationOffset * finalPose;
                    }
                }
            }

            outDevicePose->ThePose = xrPoseToOvrPose(finalPose);
            outDevicePose->TimeInSeconds = absTime;

            // Store the last reported pose. We can use it as a starting point for pre-recorded sequences.
            {
                std::unique_lock lock(m_controllerState[side].mutex);

                m_controllerState[side].latestReportedPose = finalPose;
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

                // TODO: Until GameInput is here, we use the LibOVR API to query Xbox. This only works with Quest Link,
                // not Virtual Desktop.
                ovr_GetInputState(m_ovrSession,
                                  !m_useTouchControllerButtons ? ovrControllerType_XBox : ovrControllerType_Touch,
                                  &m_controllerInputState);

                // We will use this time to latch the start time of an animation, so we can replay data timely. This is
                // the same clock that is passed to GetEmulatedDevicePose()'s absTime.
                const double ovrNow = ovr_GetTimeInSeconds();
                const double deltaTime = ovrNow - lastOvrTime;

                {
                    std::unique_lock lock1(m_controllerState[0].mutex), lock2(m_controllerState[1].mutex);

                    // DEMO CODE: Use the A/B button to switch between left/right (or both) being followGaze.
                    const bool wasFollowingGaze[2] = {m_controllerState[0].followGaze, m_controllerState[1].followGaze};
                    m_controllerState[0].followGaze =
                        m_controllerInputState.Buttons & ((m_dominantHand == 0) ? ovrButton_X : ovrButton_A);
                    m_controllerState[1].followGaze =
                        m_controllerInputState.Buttons & ((m_dominantHand == 0) ? ovrButton_Y : ovrButton_B);

                    // Always leave at least one controller following gaze.
                    if (!m_controllerState[0].followGaze && !m_controllerState[1].followGaze) {
                        m_controllerState[m_dominantHand].followGaze = wasFollowingGaze[m_dominantHand];
                        m_controllerState[m_dominantHand ^ 1].followGaze =
                            !m_controllerState[m_dominantHand].followGaze && wasFollowingGaze[m_dominantHand ^ 1];
                    }

                    // DEMO CODE: Use the shoulder button to start replay.
                    if (!m_useTouchControllerButtons
                            ? (m_controllerInputState.Buttons &
                               ((m_dominantHand == 0) ? ovrButton_LShoulder : ovrButton_RShoulder))
                            : m_controllerInputState.HandTrigger[m_dominantHand] > 0.25f) {
                        // Sample the joystick on the dominant hand to apply an additional transform to the replay.
                        const auto direction = XrVector2f{m_controllerInputState.Thumbstick[m_dominantHand].x,
                                                          m_controllerInputState.Thumbstick[m_dominantHand].y};

                        // Normalize the direction. If the joystick is untouched, assume direction is Down.
                        const auto lengthDirection = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                        const auto normalizedDirection =
                            (lengthDirection > FLT_EPSILON) ? direction / lengthDirection : XrVector2f{0.f, -1.f};

                        for (xr::side_t side = 0; side < xr::Side::Count; side++) {
                            if (!m_controllerState[side].followGaze) {
                                continue;
                            }

                            // TODO: We want to make this configurable.
                            const auto cit = m_playback.cbegin();
                            m_controllerState[side].animation = &cit->second;

                            Log("Starting replay of %s on %s side\n",
                                cit->first.c_str(),
                                side == xr::Side::Left ? "left" : "right");

                            m_controllerState[side].poseAnimationOffset = Pose::MakePose(
                                {},
                                XrVector3f{0.f,
                                           0.f,
                                           (float)M_PI_2 + std::atan2(normalizedDirection.y, normalizedDirection.x)});

                            if (m_controllerState[side].animation->startFromGrip && m_controllerState[side].gripAsAim) {
                                m_controllerState[side].poseAnimationOffset =
                                    m_controllerState[side].poseAnimationOffset * m_toGripPose[side];
                            }

                            m_controllerState[side].animationStartTime = ovrNow;
                            m_controllerState[side].animationFrame = 0;
                        }

                        // DEMO CODE: Use the joystick input on non-dominant hand to "move" the other controller (the
                        // one not following gaze).
                        if (!(m_controllerState[0].followGaze && m_controllerState[1].followGaze)) {
                            const xr::side_t otherSide =
                                !m_controllerState[0].followGaze ? xr::Side::Left : xr::Side::Right;
                            if (m_controllerState[otherSide].latestReportedPose) {
                                // TODO: This math is not correct. We want to apply the translation on the plan
                                // orthogonal to the controller forward pose.
                                const auto translation = Pose::MakePose(
                                    XrVector3f{(float)(m_controllerInputState.Thumbstick[m_dominantHand ^ 1].x *
                                                       m_joystickHorizontalSensitivity * deltaTime),
                                               (float)(m_controllerInputState.Thumbstick[m_dominantHand ^ 1].y *
                                                       m_joystickVerticalSensitivity * deltaTime),
                                               0.f},
                                    XrVector3f{0, 0, 0});
                                m_controllerState[otherSide].latestReportedPose =
                                    translation * m_controllerState[otherSide].latestReportedPose.value();
                            }
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

            const auto useTouchControllerButtons =
                cJSON_GetObjectItemCaseSensitive(top, "debug_use_touch_controller_buttons");
            m_useTouchControllerButtons = useTouchControllerButtons && useTouchControllerButtons->valueint;

            // TODO: It's be nice to properly detect and report certain parsing errors, instead of silently ignoring
            // values.
            const auto dominantHand = cJSON_GetObjectItemCaseSensitive(top, "dominant_hand");
            if (dominantHand) {
                m_dominantHand = std::min(1, dominantHand->valueint);
            }

            const auto leftGripAsAim = cJSON_GetObjectItemCaseSensitive(top, "left_grip_as_aim");
            if (leftGripAsAim) {
                m_controllerState[0].gripAsAim = leftGripAsAim->valueint;
            }
            const auto rightGripAsAim = cJSON_GetObjectItemCaseSensitive(top, "right_grip_as_aim");
            if (rightGripAsAim) {
                m_controllerState[1].gripAsAim = rightGripAsAim->valueint;
            }

            const auto joystickHorizontalSensitivity =
                cJSON_GetObjectItemCaseSensitive(top, "joystick_horizontal_sensitivity");
            if (joystickHorizontalSensitivity) {
                m_joystickHorizontalSensitivity = (float)joystickHorizontalSensitivity->valuedouble;
            }
            const auto joystickVerticalSensitivity =
                cJSON_GetObjectItemCaseSensitive(top, "joystick_vertical_sensitivity");
            if (joystickVerticalSensitivity) {
                m_joystickVerticalSensitivity = (float)joystickVerticalSensitivity->valuedouble;
            }

            const auto recordedAction = cJSON_GetObjectItemCaseSensitive(top, "recorded_action");
            if (recordedAction) {
                const auto name = cJSON_GetObjectItemCaseSensitive(recordedAction, "name");
                if (!name) {
                    throw std::runtime_error("Malformatted recorded action: no name");
                }

                const auto poses = cJSON_GetObjectItemCaseSensitive(recordedAction, "poses");
                if (!poses) {
                    throw std::runtime_error("Malformatted recorded action: no poses");
                }

                PosePlayback playback;

                const auto startFromGrip = cJSON_GetObjectItemCaseSensitive(recordedAction, "start_from_grip");
                if (startFromGrip) {
                    playback.startFromGrip = startFromGrip->valueint;
                }

                const auto numSamples = cJSON_GetArraySize(poses);
                playback.poses.reserve(numSamples);

                for (int i = 0; i < numSamples; i++) {
                    const auto item = cJSON_GetArrayItem(poses, i);
                    const auto timestamp = item ? cJSON_GetObjectItemCaseSensitive(item, "timestamp") : nullptr;
                    const auto x = item ? cJSON_GetObjectItemCaseSensitive(item, "x") : nullptr;
                    const auto y = item ? cJSON_GetObjectItemCaseSensitive(item, "y") : nullptr;
                    const auto z = item ? cJSON_GetObjectItemCaseSensitive(item, "z") : nullptr;
                    const auto rx = item ? cJSON_GetObjectItemCaseSensitive(item, "rx") : nullptr;
                    const auto ry = item ? cJSON_GetObjectItemCaseSensitive(item, "ry") : nullptr;
                    const auto rz = item ? cJSON_GetObjectItemCaseSensitive(item, "rz") : nullptr;
                    const auto rw = item ? cJSON_GetObjectItemCaseSensitive(item, "rw") : nullptr;

                    if (!(timestamp && x && y && z && rw && rx && ry && rz)) {
                        throw std::runtime_error("Malformatted recorded action: bad entry");
                    }

                    XrPosef pose =
                        Pose::MakePose(XrVector4f{(float)rx->valuedouble,
                                                  (float)ry->valuedouble,
                                                  (float)rz->valuedouble,
                                                  (float)rw->valuedouble},
                                       XrVector3f{(float)x->valuedouble, (float)y->valuedouble, (float)z->valuedouble});
                    playback.poses.push_back(std::make_pair(timestamp->valuedouble, pose));
                }

                m_playback.insert_or_assign(name->valuestring, std::move(playback));
            }
        }

        const ovrSession m_ovrSession;
        EmulatedControllerState m_controllerState[xr::Side::Count];
        std::thread m_inputThread;
        std::atomic<bool> m_isRunning;

        ovrInputState m_controllerInputState{};
        bool m_useTouchControllerButtons = false;
        xr::side_t m_dominantHand = xr::Side::Right;
        float m_joystickHorizontalSensitivity = 0.1f; // m/s at full joystick swing.
        float m_joystickVerticalSensitivity = 0.1f;   // m/s at full joystick swing.

        std::map<std::string, PosePlayback> m_playback;

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
