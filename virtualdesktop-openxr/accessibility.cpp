// MIT License
//
// Copyright(c) 2025 Microsoft Corp.
// Initial implementation by Matthieu Bucchianeri, Jonas Holderman and Heather Kemp.
// Copyright(c) 2025 Matthieu Bucchianeri
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

// GameInput is only available as a 64-bit package.
#ifdef _WIN64

namespace {

    using namespace virtualdesktop_openxr;
    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    using namespace std::chrono_literals;

#if GAMEINPUT_API_VERSION == 1
    using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
    using namespace GameInput::v2;
#endif

    using Microsoft::WRL::ComPtr;

    // The interval we will poll for inputs from GameInput.
    static constexpr auto k_PollingInterval = 2ms;

    // Default controller position relative to head (in meters)
    // Left 15cm, below 10cm, in front 35cm.
    // Will be mirrored for right side.
    static constexpr XrVector3f k_DefaultPositionRelativeToHead = {-0.15f, -0.1f, -0.35f};
    static constexpr XrVector3f k_DefaultRotationRelativeToHead = {0.f, 0.f, 0.f};

    struct PosePlayback {
        // Whether to reset to grip pose prior to starting the animation. Useful when a controller is in gripAsAim mode.
        bool startFromGrip = false;

        // Playback speed for the animation.
        double playbackSpeed = 1.0;

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
        std::optional<double> animationStartTime;

        // The current base frame for the playback.
        std::optional<size_t> animationFrame;

        // The pose to use when placing the controller in from of the user and following gaze.
        XrPosef initialPoseRelativeToHead =
            Pose::MakePose(Quaternion::RotationRollPitchYaw({OVR::DegreeToRad(k_DefaultPositionRelativeToHead.x),
                                                             OVR::DegreeToRad(k_DefaultPositionRelativeToHead.y),
                                                             OVR::DegreeToRad(k_DefaultPositionRelativeToHead.z)}),
                           k_DefaultRotationRelativeToHead);

        // An offset to apply to the running animation.
        XrPosef poseAnimationOffset = Pose::Identity();
    };

    class AccessibilityHelperImpl : public AccessibilityHelper {
      public:
        AccessibilityHelperImpl(ovrSession ovrSession,
                                const std::wstring& configPath,
                                const std::string& applicationName,
                                const std::string& exeName)
            : m_ovrSession(ovrSession) {
            // Adjust default config for right controller.
            FlipHandedness(m_controllerState[1].initialPoseRelativeToHead);

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

                    // Always load defaults first.
                    const cJSON* defaults = cJSON_GetObjectItemCaseSensitive(json, "default");
                    if (!defaults) {
                        throw std::runtime_error("Failed to get default configuration item");
                    }
                    ParseConfiguration(defaults);

                    // Next, try to amend with game-specific.
                    // Some engines (Unity with OVRPlugin) will not properly populate the OpenXR applicationName, so we
                    // try using the .exe name as well.
                    cJSON* appByName = cJSON_GetObjectItemCaseSensitive(json, applicationName.c_str());
                    cJSON* appByExe = cJSON_GetObjectItemCaseSensitive(json, exeName.c_str());
                    if (appByExe) {
                        ParseConfiguration(appByExe);
                    } else {
                        ParseConfiguration(appByName);
                    }
                } catch (std::runtime_error& exc) {
                    Log("Error parsing configuration file %ws: %s", configPath.c_str(), exc.what());
                    throw;
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

                const auto processAnimation = [&](XrPosef& transform) {
                    if (m_controllerState[side].animation) {
                        const auto currentPlaybackTime =
                            absTime - m_controllerState[side].animationStartTime.value_or(absTime);

                        while (m_controllerState[side].animationFrame &&
                               m_controllerState[side].animationFrame.value() + 1 <
                                   m_controllerState[side].animation->poses.size() &&
                               currentPlaybackTime >
                                   m_controllerState[side]
                                           .animation->poses[m_controllerState[side].animationFrame.value() + 1]
                                           .first /
                                       m_controllerState[side].animation->playbackSpeed) {
                            m_controllerState[side].animationFrame = m_controllerState[side].animationFrame.value() + 1;
                        }

                        const auto currentFrameIndex = m_controllerState[side].animationFrame.value_or(0);
                        const auto currentFrame = m_controllerState[side].animation->poses[currentFrameIndex];
                        const auto currentTimeStamp =
                            currentFrame.first / m_controllerState[side].animation->playbackSpeed;
                        const auto currentPose = currentFrame.second;

                        const auto nextFrameIndex = m_controllerState[side].animationFrame.value_or(-1) + 1;
                        if (nextFrameIndex >= m_controllerState[side].animation->poses.size()) {
                            transform = currentPose * m_controllerState[side].poseAnimationOffset * transform;

                            m_controllerState[side].animation = nullptr;
                            m_controllerState[side].animationStartTime.reset();
                            m_controllerState[side].animationFrame.reset();
                        } else {
                            // Interpolate between this frame and the next
                            const auto nextFrame = m_controllerState[side].animation->poses[nextFrameIndex];
                            const auto nextFrameTimeStamp =
                                nextFrame.first / m_controllerState[side].animation->playbackSpeed;
                            const auto nextFramePose = nextFrame.second;

                            const double deltaTime = nextFrameTimeStamp - currentTimeStamp;
                            const float alpha =
                                deltaTime ? (float)((currentPlaybackTime - currentTimeStamp) / deltaTime) : 0.f;
                            transform = xr::math::Pose::Slerp(currentPose, nextFramePose, alpha) *
                                        m_controllerState[side].poseAnimationOffset * transform;
                        }
                    }
                };

                if (!m_controllerState[side].followGaze && !shouldSpawn) {
                    if (!m_controllerState[side].latestReportedPose) {
                        return false;
                    } else {
                        finalPose = m_controllerState[side].latestReportedPose.value();

                        // Even if we stopped tracking the controller, we should still finish any queued up animation.
                        processAnimation(finalPose);

                        outDevicePose->ThePose = xrPoseToOvrPose(finalPose);
                        return true;
                    }
                }

                ZeroMemory(outDevicePose, sizeof(ovrPoseStatef));

                // Move the emulated controllers in front of the user.
                // Get the head pose.
                ovrPoseStatef headPoseState{};
                ovrTrackedDeviceType hmd = ovrTrackedDevice_HMD;
                const auto result = ovr_GetDevicePoses(m_ovrSession, &hmd, 1, absTime, &headPoseState);

                const auto headPose = ovrPoseToXrPose(headPoseState.ThePose);

                // Either leave as grip, or apply transform into aim.
                finalPose = m_controllerState[side].initialPoseRelativeToHead * headPose;
                if (m_controllerState[side].gripAsAim) {
                    finalPose = Pose::Invert(m_toGripPose[side]) * finalPose;
                }

                // Replay a pre-recorded sequence (animation).
                processAnimation(finalPose);
            }

            outDevicePose->ThePose = xrPoseToOvrPose(finalPose);

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

            // Passthrough the trigger (so we can click in menus).
            outInputState->IndexTrigger[side] = outInputState->IndexTriggerNoDeadzone[side] =
                outInputState->IndexTriggerRaw[side] = m_controllerInputState.IndexTrigger[m_dominantHand];

            // Passthrough the menu button (so we can open menus).
            outInputState->Buttons |= (m_controllerInputState.Buttons & ovrButton_Enter);

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
        enum class CardinalDirection { Center, North, South, East, West, Northeast, Northwest, Southeast, Southwest };

        void InputThread() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

            // GameInput types
            ComPtr<IGameInput> gameInput;

            if (!m_useTouchControllerButtons) {
                // Initialize GameInput.
                if (!SUCCEEDED(GameInputCreate(gameInput.GetAddressOf()))) {
                    throw std::runtime_error("Failed to initialize GameInput API");
                }
            }

            double lastOvrTime = ovr_GetTimeInSeconds();
            while (m_isRunning.load()) {
                const auto nextInterval = std::chrono::high_resolution_clock::now() + k_PollingInterval;

                if (m_useTouchControllerButtons) {
                    ovr_GetInputState(m_ovrSession, ovrControllerType_Touch, &m_controllerInputState);
                } else {
                    GetCurrentGamepadState(gameInput, &m_controllerInputState);
                }

                // We will use this time to latch the start time of an animation, so we can replay data timely. This is
                // the same clock that is passed to GetEmulatedDevicePose()'s absTime.
                const double ovrNow = ovr_GetTimeInSeconds();
                const double deltaTime = ovrNow - lastOvrTime;

                {
                    std::unique_lock lock1(m_controllerState[0].mutex), lock2(m_controllerState[1].mutex);

                    // Use the A/B button to switch between left/right (or both) being followGaze.
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

                    // If we _were_ following the gaze and now we're not, update the rotation of that last reported pose
                    for (xr::side_t side = 0; side < xr::Side::Count; side++) {
                        if (!m_controllerState[side].followGaze && wasFollowingGaze[side]) {
                            m_controllerState[side].poseAnimationOffset = Pose::Identity();
                        }
                    }

                    // Use D-pad to cycle through pre-recorded sequences.
                    if (!m_useTouchControllerButtons ? (m_controllerInputState.Buttons & ovrButton_Up) : false) {
                        m_playbackIndex = 0;
                    }

                    if (!m_useTouchControllerButtons ? (m_controllerInputState.Buttons & ovrButton_Right) : false) {
                        m_playbackIndex = 1 % m_playback.size();
                    }

                    if (!m_useTouchControllerButtons ? (m_controllerInputState.Buttons & ovrButton_Down) : false) {
                        m_playbackIndex = 2 % m_playback.size();
                    }

                    if (!m_useTouchControllerButtons ? (m_controllerInputState.Buttons & ovrButton_Left) : false) {
                        m_playbackIndex = 3 % m_playback.size();
                    }

                    // Use the shoulder button to start replay.
                    if (!m_playback.empty() && !m_useTouchControllerButtons
                            ? (m_controllerInputState.Buttons &
                               ((m_dominantHand == 0) ? ovrButton_LShoulder : ovrButton_RShoulder))
                            : m_controllerInputState.HandTrigger[m_dominantHand] > 0.25f) {
                        // Sample the joystick on the dominant hand to apply an additional transform to the replay.
                        const auto filtered =
                            HandleJoystickDeadzone({m_controllerInputState.Thumbstick[m_dominantHand].x,
                                                    m_controllerInputState.Thumbstick[m_dominantHand].y});

                        // Snap the joystick to the cardinal directions if the user has enabled that option.
                        const auto direction =
                            (m_useJoystickCardinalSnap) ? SnapJoystickToCardinal(filtered) : filtered;

                        // Normalize the direction. If the joystick is untouched, assume direction is Down.
                        const auto lengthDirection = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                        const auto normalizedDirection =
                            (lengthDirection > FLT_EPSILON) ? direction / lengthDirection : XrVector2f{0.f, -1.f};
                        const auto normalizedDirectionWithDeadzone = (normalizedDirection);

                        for (xr::side_t side = 0; side < xr::Side::Count; side++) {
                            if (!m_controllerState[side].followGaze) {
                                continue;
                            }

                            auto cit = m_playback.cbegin();
                            std::advance(cit, m_playbackIndex);
                            m_controllerState[side].animation = &cit->second;

                            m_controllerState[side].poseAnimationOffset = Pose::MakePose(
                                Quaternion::Identity(),
                                XrVector3f{0.f,
                                           0.f,
                                           (float)M_PI_2 + std::atan2(normalizedDirectionWithDeadzone.y,
                                                                      normalizedDirectionWithDeadzone.x)});

                            if (m_controllerState[side].animation->startFromGrip && m_controllerState[side].gripAsAim) {
                                m_controllerState[side].poseAnimationOffset =
                                    m_controllerState[side].poseAnimationOffset * m_toGripPose[side];
                            }
                        }
                    } else {
                        for (xr::side_t side = 0; side < xr::Side::Count; side++) {
                            if (m_controllerState[side].animation && !m_controllerState[side].animationStartTime) {
                                m_controllerState[side].animationStartTime = ovrNow;
                                m_controllerState[side].animationFrame = 0;
                            }
                        }
                    }

                    // Use the joystick input on non-dominant hand to "move" the other controller (the
                    // one not following gaze).
                    if (!(m_controllerState[0].followGaze && m_controllerState[1].followGaze)) {
                        const xr::side_t otherSide =
                            !m_controllerState[0].followGaze ? xr::Side::Left : xr::Side::Right;
                        if (m_controllerState[otherSide].latestReportedPose) {
                            // TODO: This math is not correct. We want to apply the translation on the plan
                            // orthogonal to the controller forward pose.
                            const auto translation = Pose::MakePose(
                                Quaternion::Identity(),
                                XrVector3f{(float)(m_controllerInputState.Thumbstick[m_dominantHand ^ 1].x *
                                                   m_joystickHorizontalSensitivity * deltaTime),
                                           (float)(m_controllerInputState.Thumbstick[m_dominantHand ^ 1].y *
                                                   m_joystickVerticalSensitivity * deltaTime),
                                           0.f});
                            m_controllerState[otherSide].latestReportedPose =
                                translation * m_controllerState[otherSide].latestReportedPose.value();
                        }
                    }
                }

                // Record the last time the inputs were polled, so we can scale inputs with time.
                lastOvrTime = ovrNow;

                std::this_thread::sleep_until(nextInterval);
            }
        }

        void ParseConfiguration(const cJSON* top) {
            auto parsePoseSimple = [](const cJSON* parent, const char* key, XrPosef* outPose) {
                if (parent && outPose) {
                    const cJSON* poseObj = key ? cJSON_GetObjectItemCaseSensitive(parent, key) : parent;
                    if (poseObj) {
                        const auto x = cJSON_GetObjectItemCaseSensitive(poseObj, "x");
                        const auto y = cJSON_GetObjectItemCaseSensitive(poseObj, "y");
                        const auto z = cJSON_GetObjectItemCaseSensitive(poseObj, "z");
                        const auto yaw = cJSON_GetObjectItemCaseSensitive(poseObj, "yaw");
                        const auto pitch = cJSON_GetObjectItemCaseSensitive(poseObj, "pitch");
                        const auto roll = cJSON_GetObjectItemCaseSensitive(poseObj, "roll");
                        // Make position mandatory, orientation optional.
                        if (x && y && z) {
                            const auto toRadians = [](double degrees) { return (float)(degrees * M_PI / 180); };

                            *outPose = Pose::MakePose(
                                Quaternion::RotationRollPitchYaw(
                                    {pitch ? (float)OVR::DegreeToRad(pitch->valuedouble) : 0.f,
                                     yaw ? (float)OVR::DegreeToRad(yaw->valuedouble) : 0.f,
                                     roll ? (float)OVR::DegreeToRad(roll->valuedouble) : 0.f}),
                                XrVector3f{(float)x->valuedouble, (float)y->valuedouble, (float)z->valuedouble});
                            return true;
                        }
                    }
                }
                return false;
            };

            auto parsePose = [](const cJSON* parent, const char* key, XrPosef* outPose) {
                if (parent && outPose) {
                    const cJSON* poseObj = key ? cJSON_GetObjectItemCaseSensitive(parent, key) : parent;
                    if (poseObj && outPose) {
                        const auto x = cJSON_GetObjectItemCaseSensitive(poseObj, "x");
                        const auto y = cJSON_GetObjectItemCaseSensitive(poseObj, "y");
                        const auto z = cJSON_GetObjectItemCaseSensitive(poseObj, "z");
                        if (x && y && z) {
                            const auto rx = cJSON_GetObjectItemCaseSensitive(poseObj, "rx");
                            const auto ry = cJSON_GetObjectItemCaseSensitive(poseObj, "ry");
                            const auto rz = cJSON_GetObjectItemCaseSensitive(poseObj, "rz");
                            const auto rw = cJSON_GetObjectItemCaseSensitive(poseObj, "rw");
                            if (rx && ry && rz && rw) {
                                *outPose = Pose::MakePose(
                                    XrVector4f{(float)rx->valuedouble,
                                               (float)ry->valuedouble,
                                               (float)rz->valuedouble,
                                               (float)rw->valuedouble},
                                    XrVector3f{(float)x->valuedouble, (float)y->valuedouble, (float)z->valuedouble});
                                return true;
                            }

                            const auto yaw = cJSON_GetObjectItemCaseSensitive(poseObj, "yaw");
                            const auto pitch = cJSON_GetObjectItemCaseSensitive(poseObj, "pitch");
                            const auto roll = cJSON_GetObjectItemCaseSensitive(poseObj, "roll");
                            if (yaw && pitch && roll) {
                                *outPose = Pose::MakePose(
                                    Quaternion::RotationRollPitchYaw(
                                        {pitch ? (float)OVR::DegreeToRad(pitch->valuedouble) : 0.f,
                                         yaw ? (float)OVR::DegreeToRad(yaw->valuedouble) : 0.f,
                                         roll ? (float)OVR::DegreeToRad(roll->valuedouble) : 0.f}),
                                    XrVector3f{(float)x->valuedouble, (float)y->valuedouble, (float)z->valuedouble});
                                return true;
                            }
                        }
                    }
                }
                return false;
            };

            if (const auto emulateLeft = cJSON_GetObjectItemCaseSensitive(top, "emulate_left")) {
                m_controllerState[0].enabled = emulateLeft->valueint;
            }
            if (const auto emulateRight = cJSON_GetObjectItemCaseSensitive(top, "emulate_right")) {
                m_controllerState[1].enabled = emulateRight->valueint;
            }

            if (parsePoseSimple(top, "pose_relative_to_head", &m_controllerState[0].initialPoseRelativeToHead)) {
                // Replicate to right side and flip.
                m_controllerState[1].initialPoseRelativeToHead = m_controllerState[0].initialPoseRelativeToHead;
                FlipHandedness(m_controllerState[1].initialPoseRelativeToHead);
            } else {
                parsePoseSimple(top, "left_pose_relative_to_head", &m_controllerState[0].initialPoseRelativeToHead);
                parsePoseSimple(top, "right_pose_relative_to_head", &m_controllerState[1].initialPoseRelativeToHead);
            }

            if (const auto useTouchControllerButtons =
                    cJSON_GetObjectItemCaseSensitive(top, "debug_use_touch_controller_buttons")) {
                m_useTouchControllerButtons = useTouchControllerButtons->valueint;
            }

            // TODO: It'd be nice to properly detect and report certain parsing errors, instead of silently ignoring
            // values.
            if (const auto dominantHand = cJSON_GetObjectItemCaseSensitive(top, "dominant_hand")) {
                m_dominantHand = std::min(1, dominantHand->valueint);
            }

            if (const auto leftGripAsAim = cJSON_GetObjectItemCaseSensitive(top, "left_grip_as_aim")) {
                m_controllerState[0].gripAsAim = leftGripAsAim->valueint;
            }
            if (const auto rightGripAsAim = cJSON_GetObjectItemCaseSensitive(top, "right_grip_as_aim")) {
                m_controllerState[1].gripAsAim = rightGripAsAim->valueint;
            }

            if (const auto joystickHorizontalSensitivity =
                    cJSON_GetObjectItemCaseSensitive(top, "joystick_horizontal_sensitivity")) {
                m_joystickHorizontalSensitivity = (float)joystickHorizontalSensitivity->valuedouble;
            }
            if (const auto joystickVerticalSensitivity =
                    cJSON_GetObjectItemCaseSensitive(top, "joystick_vertical_sensitivity")) {
                m_joystickVerticalSensitivity = (float)joystickVerticalSensitivity->valuedouble;
            }
            if (const auto joystickDeadzone = cJSON_GetObjectItemCaseSensitive(top, "joystick_deadzone")) {
                m_joystickDeadzone = (float)joystickDeadzone->valuedouble;
            }
            if (const auto joystickCardinalSnap = cJSON_GetObjectItemCaseSensitive(top, "joystick_cardinal_snap")) {
                m_useJoystickCardinalSnap = joystickCardinalSnap->valueint;
            }

            if (const auto recordedActions = cJSON_GetObjectItemCaseSensitive(top, "recorded_actions")) {
                m_playback.clear();
                const auto numActions = cJSON_GetArraySize(recordedActions);
                for (int j = 0; j < numActions; j++) {
                    const auto recordedAction = cJSON_GetArrayItem(recordedActions, j);
                    const auto name = cJSON_GetObjectItemCaseSensitive(recordedAction, "name");
                    if (!name) {
                        throw std::runtime_error("Malformatted recorded action: no name");
                    }

                    const auto poses = cJSON_GetObjectItemCaseSensitive(recordedAction, "poses");
                    if (!poses) {
                        throw std::runtime_error("Malformatted recorded action: no poses");
                    }

                    PosePlayback playback;

                    if (const auto startFromGrip =
                            cJSON_GetObjectItemCaseSensitive(recordedAction, "start_from_grip")) {
                        playback.startFromGrip = startFromGrip->valueint;
                    }

                    if (const auto playbackSpeedJson =
                            cJSON_GetObjectItemCaseSensitive(recordedAction, "playbackSpeed")) {
                        playback.playbackSpeed = playbackSpeedJson->valuedouble;
                    }

                    const auto numSamples = cJSON_GetArraySize(poses);
                    playback.poses.reserve(numSamples);

                    for (int i = 0; i < numSamples; i++) {
                        const auto item = cJSON_GetArrayItem(poses, i);
                        if (!item) {
                            throw std::runtime_error("Malformatted recorded action: missing array item");
                        }

                        const auto timestamp = cJSON_GetObjectItemCaseSensitive(item, "timestamp");
                        if (!timestamp) {
                            throw std::runtime_error("Malformatted recorded action: missing timestamp");
                        }

                        XrPosef pose{};
                        if (!parsePose(item, nullptr, &pose)) {
                            throw std::runtime_error("Malformatted recorded action: bad pose entry");
                        }

                        playback.poses.emplace_back(timestamp->valuedouble, pose);
                    }

                    m_playback.insert_or_assign(name->valuestring, std::move(playback));
                }
            }
        }

        // Helper function to convert GameInputGamepadState to ovrInputState using the mapping logic from the input
        // thread.
        void ConvertGamepadStateToOvrInputState(const GameInputGamepadState& gamepadState, ovrInputState* ovrState) {
            ZeroMemory(ovrState, sizeof(ovrInputState));

            ovrState->ControllerType = ovrControllerType_XBox;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadA)
                ovrState->Buttons |= ovrButton_A;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadB)
                ovrState->Buttons |= ovrButton_B;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadX)
                ovrState->Buttons |= ovrButton_X;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadY)
                ovrState->Buttons |= ovrButton_Y;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadMenu)
                ovrState->Buttons |= ovrButton_Enter;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadView)
                ovrState->Buttons |= ovrButton_Back;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadUp)
                ovrState->Buttons |= ovrButton_Up;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadDown)
                ovrState->Buttons |= ovrButton_Down;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadLeft)
                ovrState->Buttons |= ovrButton_Left;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadDPadRight)
                ovrState->Buttons |= ovrButton_Right;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadRightShoulder)
                ovrState->Buttons |= ovrButton_RShoulder;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadLeftShoulder)
                ovrState->Buttons |= ovrButton_LShoulder;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadRightThumbstick)
                ovrState->Buttons |= ovrButton_RThumb;

            if (gamepadState.buttons & GameInputGamepadButtons::GameInputGamepadLeftThumbstick)
                ovrState->Buttons |= ovrButton_LThumb;

            // Map thumbsticks
            ovrState->Thumbstick[ovrHand_Left].x = gamepadState.leftThumbstickX;
            ovrState->Thumbstick[ovrHand_Left].y = gamepadState.leftThumbstickY;
            ovrState->Thumbstick[ovrHand_Right].x = gamepadState.rightThumbstickX;
            ovrState->Thumbstick[ovrHand_Right].y = gamepadState.rightThumbstickY;

            // Raw values (same as above if no filtering)
            ovrState->ThumbstickRaw[ovrHand_Left] = ovrState->Thumbstick[ovrHand_Left];
            ovrState->ThumbstickRaw[ovrHand_Right] = ovrState->Thumbstick[ovrHand_Right];

            // Map triggers to IndexTrigger (finger trigger)
            ovrState->IndexTrigger[ovrHand_Left] = gamepadState.leftTrigger;
            ovrState->IndexTrigger[ovrHand_Right] = gamepadState.rightTrigger;

            // Raw triggers
            ovrState->IndexTriggerRaw[ovrHand_Left] = gamepadState.leftTrigger;
            ovrState->IndexTriggerRaw[ovrHand_Right] = gamepadState.rightTrigger;

            // NoDeadzone versions
            ovrState->ThumbstickNoDeadzone[ovrHand_Left] = ovrState->Thumbstick[ovrHand_Left];
            ovrState->ThumbstickNoDeadzone[ovrHand_Right] = ovrState->Thumbstick[ovrHand_Right];
            ovrState->IndexTriggerNoDeadzone[ovrHand_Left] = gamepadState.leftTrigger;
            ovrState->IndexTriggerNoDeadzone[ovrHand_Right] = gamepadState.rightTrigger;
        }

        // Helper function to get the current gamepad state using GameInput API.
        bool GetCurrentGamepadState(const ComPtr<IGameInput>& gameInput, ovrInputState* outState) {
            ComPtr<IGameInputReading> gamepadReading;
            if (SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, gamepadReading.GetAddressOf()))) {
                GameInputGamepadState gamepadState = {};
                if (gamepadReading->GetGamepadState(&gamepadState)) {
                    ConvertGamepadStateToOvrInputState(gamepadState, outState);
                    return true;
                }
            }
            return false;
        }

        XrVector2f HandleJoystickDeadzone(const XrVector2f& rawInput) const {
            const float length = std::sqrt(rawInput.x * rawInput.x + rawInput.y * rawInput.y);
            if (length < m_joystickDeadzone) {
                return {0, 0};
            }
            XrVector2f normalizedInput{rawInput.x / length, rawInput.y / length};
            const float scaling = (length - m_joystickDeadzone) / (1 - m_joystickDeadzone);
            return {normalizedInput.x * scaling, normalizedInput.y * scaling};
        }

        void FlipHandedness(XrPosef& pose) {
            // Mirror pose along the X axis.
            // https://stackoverflow.com/a/33999726/15056285
            pose.position.x = -pose.position.x;
            pose.orientation.y = -pose.orientation.y;
            pose.orientation.z = -pose.orientation.z;
        };

        CardinalDirection GetJoystickCardinalDirection(const XrVector2f& rawInput) {
            static constexpr float k_Threshold = 0.3f; // Threshold for snap to center

            if (std::fabs(rawInput.x) < k_Threshold && std::fabs(rawInput.y) < k_Threshold) {
                return CardinalDirection::Center;
            }

            if (rawInput.y >= k_Threshold) {
                if (rawInput.x >= k_Threshold) {
                    return CardinalDirection::Northeast;
                }
                if (rawInput.x <= -k_Threshold) {
                    return CardinalDirection::Northwest;
                }
                return CardinalDirection::North;
            } else if (rawInput.y <= -k_Threshold) {
                if (rawInput.x >= k_Threshold) {
                    return CardinalDirection::Southeast;
                }
                if (rawInput.x <= -k_Threshold) {
                    return CardinalDirection::Southwest;
                }
                return CardinalDirection::South;
            } else {
                if (rawInput.x >= k_Threshold) {
                    return CardinalDirection::East;
                }
                if (rawInput.x <= -k_Threshold) {
                    return CardinalDirection::West;
                }
            }

            return CardinalDirection::Center;
        }

        XrVector2f SnapJoystickToCardinal(const XrVector2f& rawInput) {
            static constexpr float k_InvSqrt2 = 0.70710678f;

            const CardinalDirection dir = GetJoystickCardinalDirection(rawInput);
            const float magnitude = std::sqrt(rawInput.x * rawInput.x + rawInput.y * rawInput.y);

            switch (dir) {
            case CardinalDirection::North:
                return {0.f, magnitude};
            case CardinalDirection::South:
                return {0.f, -magnitude};
            case CardinalDirection::East:
                return {magnitude, 0.f};
            case CardinalDirection::West:
                return {-magnitude, 0.f};
            case CardinalDirection::Northeast:
                return {magnitude * k_InvSqrt2, magnitude * k_InvSqrt2};
            case CardinalDirection::Northwest:
                return {-magnitude * k_InvSqrt2, magnitude * k_InvSqrt2};
            case CardinalDirection::Southeast:
                return {magnitude * k_InvSqrt2, -magnitude * k_InvSqrt2};
            case CardinalDirection::Southwest:
                return {-magnitude * k_InvSqrt2, -magnitude * k_InvSqrt2};
            case CardinalDirection::Center:
            default:
                return {0.f, 0.f};
            }
        }

        const ovrSession m_ovrSession;
        EmulatedControllerState m_controllerState[xr::Side::Count];
        std::thread m_inputThread;
        std::atomic<bool> m_isRunning;

        ovrInputState m_controllerInputState{};
        bool m_useTouchControllerButtons = false;
        bool m_useJoystickCardinalSnap = false;
        xr::side_t m_dominantHand = xr::Side::Right;
        float m_joystickHorizontalSensitivity = 0.1f; // m/s at full joystick swing.
        float m_joystickVerticalSensitivity = 0.1f;   // m/s at full joystick swing.
        float m_joystickDeadzone = 0.2f;

        size_t m_playbackIndex = 0;
        std::map<std::string, PosePlayback> m_playback;

        // OVR to OpenXR poses. Useful if we want to emulate a pose relative to the standard grip or aim pose.
        XrPosef m_toGripPose[xr::Side::Count];
        XrPosef m_toAimPose[xr::Side::Count];
    };

} // namespace

#endif

namespace virtualdesktop_openxr {
    std::unique_ptr<AccessibilityHelper> CreateAccessibilityHelper(ovrSession ovrSession,
                                                                   const std::wstring& configPath,
                                                                   const std::string& applicationName,
                                                                   const std::string& exeName) {
        try {
#ifdef _WIN64
            return std::make_unique<AccessibilityHelperImpl>(ovrSession, configPath, applicationName, exeName);
#endif
        } catch (...) {
        }
        return {};
    }

} // namespace virtualdesktop_openxr
