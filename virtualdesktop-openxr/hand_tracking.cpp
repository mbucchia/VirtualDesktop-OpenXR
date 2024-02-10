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

// Implements the necessary support for the XR_EXT_hand_tracking extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateHandTrackerEXT
    XrResult OpenXrRuntime::xrCreateHandTrackerEXT(XrSession session,
                                                   const XrHandTrackerCreateInfoEXT* createInfo,
                                                   XrHandTrackerEXT* handTracker) {
        if (createInfo->type != XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateHandTrackerEXT",
                          TLXArg(session, "Session"),
                          TLArg((uint32_t)createInfo->hand, "Hand"),
                          TLArg((uint32_t)createInfo->handJointSet, "HandJointSet"));

        if (!has_XR_EXT_hand_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_supportsHandTracking) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if ((createInfo->hand != XR_HAND_LEFT_EXT && createInfo->hand != XR_HAND_RIGHT_EXT) ||
            createInfo->handJointSet != XR_HAND_JOINT_SET_DEFAULT_EXT) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        std::unique_lock lock(m_handTrackersMutex);

        HandTracker& xrHandTracker = *new HandTracker;
        xrHandTracker.side = createInfo->hand == XR_HAND_LEFT_EXT ? xr::Side::Left : xr::Side::Right;

        *handTracker = (XrHandTrackerEXT)&xrHandTracker;

        // Maintain a list of known trackers for validation.
        m_handTrackers.insert(*handTracker);

        TraceLoggingWrite(g_traceProvider, "xrCreateHandTrackerEXT", TLXArg(*handTracker, "HandTracker"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyHandTrackerEXT
    XrResult OpenXrRuntime::xrDestroyHandTrackerEXT(XrHandTrackerEXT handTracker) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyHandTrackerEXT", TLXArg(handTracker, "HandTracker"));

        if (!has_XR_EXT_hand_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_handTrackersMutex);

        if (!m_handTrackers.count(handTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        HandTracker* xrHandTracker = (HandTracker*)handTracker;

        delete xrHandTracker;
        m_handTrackers.erase(handTracker);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrLocateHandJointsEXT
    XrResult OpenXrRuntime::xrLocateHandJointsEXT(XrHandTrackerEXT handTracker,
                                                  const XrHandJointsLocateInfoEXT* locateInfo,
                                                  XrHandJointLocationsEXT* locations) {
        if (locateInfo->type != XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT ||
            locations->type != XR_TYPE_HAND_JOINT_LOCATIONS_EXT) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrLocateHandJointsEXT",
                          TLXArg(handTracker, "HandTracker"),
                          TLArg(locateInfo->time),
                          TLXArg(locateInfo->baseSpace));

        if (!has_XR_EXT_hand_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::shared_lock lock(m_handTrackersMutex);
        std::shared_lock lock2(m_actionsAndSpacesMutex);

        if (!m_handTrackers.count(handTracker) || !m_spaces.count(locateInfo->baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        XrHandJointVelocitiesEXT* velocities = nullptr;
        XrHandTrackingAimStateFB* aimState = nullptr;

        XrBaseOutStructure* entry = reinterpret_cast<XrBaseOutStructure*>(locations->next);
        while (entry) {
            switch (entry->type) {
            case XR_TYPE_HAND_JOINT_VELOCITIES_EXT:
                velocities = reinterpret_cast<XrHandJointVelocitiesEXT*>(entry);
                break;
            case XR_TYPE_HAND_TRACKING_AIM_STATE_FB:
                aimState = reinterpret_cast<XrHandTrackingAimStateFB*>(entry);
                break;
            }

            entry = reinterpret_cast<XrBaseOutStructure*>(entry->next);
        }

        if (locations->jointCount != XR_HAND_JOINT_COUNT_EXT ||
            (velocities && velocities->jointCount != XR_HAND_JOINT_COUNT_EXT)) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        const HandTracker& xrHandTracker = *(HandTracker*)handTracker;

        Space& xrBaseSpace = *(Space*)locateInfo->baseSpace;

        XrPosef baseSpaceToVirtual = Pose::Identity();
        const auto flags = locateSpaceToOrigin(xrBaseSpace, locateInfo->time, baseSpaceToVirtual, nullptr, nullptr);

        {
            std::shared_lock lock(m_bodyStateMutex);

            // Check the hand state.
            if (m_bodyState && ((xrHandTracker.side == xr::Side::Left && m_cachedBodyState.LeftHandActive) ||
                                m_cachedBodyState.RightHandActive)) {
                const BodyTracking::FingerJointState* const joints = xrHandTracker.side == xr::Side::Left
                                                                         ? m_cachedBodyState.LeftHandJointStates
                                                                         : m_cachedBodyState.RightHandJointStates;

                TraceLoggingWrite(g_traceProvider,
                                  "xrLocateHandJointsEXT",
                                  TLArg(xrHandTracker.side == xr::Side::Left ? "Left" : "Right", "Side"),
                                  TLArg(xrHandTracker.side == xr::Side::Left ? !!m_cachedBodyState.LeftHandActive
                                                                             : !!m_cachedBodyState.RightHandActive,
                                        "HandActive"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_PALM_EXT].Pose).c_str(), "Palm"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_WRIST_EXT].Pose).c_str(), "Wrist"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_THUMB_TIP_EXT].Pose).c_str(), "ThumbTip"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_INDEX_TIP_EXT].Pose).c_str(), "IndexTip"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_MIDDLE_TIP_EXT].Pose).c_str(), "MiddleTip"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_RING_TIP_EXT].Pose).c_str(), "RingTip"),
                                  TLArg(xr::ToString(joints[XR_HAND_JOINT_LITTLE_TIP_EXT].Pose).c_str(), "LittleTip"));

                locations->isActive = XR_TRUE;

            } else {
                TraceLoggingWrite(g_traceProvider,
                                  "xrLocateHandJointsEXT",
                                  TLArg(xrHandTracker.side == xr::Side::Left ? "Left" : "Right", "Side"),
                                  TLArg(!!m_cachedBodyState.LeftHandActive, "LeftHandActive"),
                                  TLArg(!!m_cachedBodyState.RightHandActive, "RightHandActive"));

                locations->isActive = XR_FALSE;
            }

            // If base space pose is not valid, we cannot locate.
            if (locations->isActive != XR_TRUE || !Pose::IsPoseValid(flags)) {
                TraceLoggingWrite(g_traceProvider, "xrLocateHandJointsEXT", TLArg(0, "LocationFlags"));
                for (uint32_t i = 0; i < locations->jointCount; i++) {
                    locations->jointLocations[i].radius = 0.0f;
                    locations->jointLocations[i].pose = Pose::Identity();
                    locations->jointLocations[i].locationFlags = 0;

                    if (velocities) {
                        velocities->jointVelocities[i].angularVelocity = {};
                        velocities->jointVelocities[i].linearVelocity = {};
                        velocities->jointVelocities[i].velocityFlags = 0;
                    }
                }
                return XR_SUCCESS;
            }

            const BodyTracking::FingerJointState* const joints = xrHandTracker.side == xr::Side::Left
                                                                     ? m_cachedBodyState.LeftHandJointStates
                                                                     : m_cachedBodyState.RightHandJointStates;

            // Virtual Desktop queries the joints in local or stage space depending on whether Stage Tracking is
            // enabled. We need to offset to the virtual space.
            assert(ovr_GetTrackingOriginType(m_ovrSession) == ovrTrackingOrigin_FloorLevel);
            const float floorHeight = ovr_GetFloat(m_ovrSession, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);
            TraceLoggingWrite(g_traceProvider, "OVR_GetConfig", TLArg(floorHeight, "EyeHeight"));
            const XrPosef jointsToVirtual =
                (std::abs(floorHeight) >= FLT_EPSILON) ? Pose::Translation({0, floorHeight, 0}) : Pose::Identity();
            const XrPosef basePose = Pose::Multiply(jointsToVirtual, Pose::Invert(baseSpaceToVirtual));

            for (uint32_t i = 0; i < locations->jointCount; i++) {
                const XrPosef poseOfJoint = Pose::MakePose(
                    XrQuaternionf{joints[i].Pose.orientation.x,
                                  joints[i].Pose.orientation.y,
                                  joints[i].Pose.orientation.z,
                                  joints[i].Pose.orientation.w},
                    XrVector3f{joints[i].Pose.position.x, joints[i].Pose.position.y, joints[i].Pose.position.z});

                locations->jointLocations[i].pose = Pose::Multiply(poseOfJoint, basePose);
                locations->jointLocations[i].locationFlags =
                    (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT |
                     XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT);

                // Forward the rest of the data as-is from the memory mapped file.
                locations->jointLocations[i].radius = joints[i].Radius;

                if (velocities) {
                    velocities->jointVelocities[i].angularVelocity = {
                        joints[i].AngularVelocity.x, joints[i].AngularVelocity.y, joints[i].AngularVelocity.z};
                    velocities->jointVelocities[i].linearVelocity = {
                        joints[i].LinearVelocity.x, joints[i].LinearVelocity.y, joints[i].LinearVelocity.z};
                    velocities->jointVelocities[i].velocityFlags =
                        XR_SPACE_VELOCITY_ANGULAR_VALID_BIT | XR_SPACE_VELOCITY_LINEAR_VALID_BIT;

                    TraceLoggingWrite(
                        g_traceProvider,
                        "xrLocateHandJointsEXT",
                        TLArg(i, "JointIndex"),
                        TLArg(locations->jointLocations[i].locationFlags, "LocationFlags"),
                        TLArg(xr::ToString(locations->jointLocations[i].pose).c_str(), "Pose"),
                        TLArg(locations->jointLocations[i].radius, "Radius"),
                        TLArg(velocities->jointVelocities[i].velocityFlags, "VelocityFlags"),
                        TLArg(xr::ToString(velocities->jointVelocities[i].angularVelocity).c_str(), "AngularVelocity"),
                        TLArg(xr::ToString(velocities->jointVelocities[i].linearVelocity).c_str(), "LinearVelocity"));
                } else {
                    TraceLoggingWrite(g_traceProvider,
                                      "xrLocateHandJointsEXT",
                                      TLArg(i, "JointIndex"),
                                      TLArg(locations->jointLocations[i].locationFlags, "LocationFlags"),
                                      TLArg(xr::ToString(locations->jointLocations[i].pose).c_str(), "Pose"),
                                      TLArg(locations->jointLocations[i].radius, "Radius"));
                }
            }

            if (has_XR_FB_hand_tracking_aim && aimState) {
                const BodyTracking::HandTrackingAimState& aim = xrHandTracker.side == xr::Side::Left
                                                                    ? m_cachedBodyState.LeftAimState
                                                                    : m_cachedBodyState.RightAimState;

                aimState->status = aim.AimStatus;
                aimState->aimPose = Pose::Multiply(
                    Pose::MakePose(XrQuaternionf{aim.AimPose.orientation.x,
                                                 aim.AimPose.orientation.y,
                                                 aim.AimPose.orientation.z,
                                                 aim.AimPose.orientation.w},
                                   XrVector3f{aim.AimPose.position.x, aim.AimPose.position.y, aim.AimPose.position.z}),
                    basePose);
                aimState->pinchStrengthIndex = aim.PinchStrengthIndex;
                aimState->pinchStrengthMiddle = aim.PinchStrengthMiddle;
                aimState->pinchStrengthRing = aim.PinchStrengthRing;
                aimState->pinchStrengthLittle = aim.PinchStrengthLittle;

                TraceLoggingWrite(g_traceProvider,
                                  "xrLocateHandJointsEXT",
                                  TLArg(xrHandTracker.side == xr::Side::Left ? "Left" : "Right", "Side"),
                                  TLArg(aimState->status, "Status"),
                                  TLArg(xr::ToString(aimState->aimPose).c_str(), "AimPose"),
                                  TLArg(aimState->pinchStrengthIndex, "PinchStrengthIndex"),
                                  TLArg(aimState->pinchStrengthMiddle, "PinchStrengthMiddle"),
                                  TLArg(aimState->pinchStrengthRing, "PinchStrengthRing"),
                                  TLArg(aimState->pinchStrengthLittle, "PinchStrengthLittle"));
            }
        }

        return XR_SUCCESS;
    }

    // Detect hand gestures and convert them into controller inputs.
    void OpenXrRuntime::processHandGestures(uint32_t side) {
        std::shared_lock lock(m_bodyStateMutex);

        if (m_bodyState &&
            ((side == xr::Side::Left && m_cachedBodyState.LeftHandActive) || m_cachedBodyState.RightHandActive)) {
            const BodyTracking::FingerJointState* joints =
                side == xr::Side::Left ? m_cachedBodyState.LeftHandJointStates : m_cachedBodyState.RightHandJointStates;
            const bool otherJointsValid =
                side == xr::Side::Left ? m_cachedBodyState.LeftHandActive : m_cachedBodyState.RightHandActive;
            const BodyTracking::FingerJointState* otherJoints =
                side == xr::Side::Left ? m_cachedBodyState.RightHandJointStates : m_cachedBodyState.LeftHandJointStates;
            const BodyTracking::HandTrackingAimState& aimState =
                side == xr::Side::Left ? m_cachedBodyState.LeftAimState : m_cachedBodyState.RightAimState;

            TraceLoggingWrite(
                g_traceProvider,
                "HandGestures",
                TLArg(side == xr::Side::Left ? "Left" : "Right", "Side"),
                TLArg(otherJointsValid, "OtherHandActive"),
                TLArg(aimState.PinchStrengthIndex, "PinchStrengthIndex"),
                TLArg(xr::ToString(joints[XR_HAND_JOINT_PALM_EXT].Pose).c_str(), "Palm"),
                TLArg(xr::ToString(otherJoints[XR_HAND_JOINT_INDEX_TIP_EXT].Pose).c_str(), "OtherHandIndexTip"));

            const auto jointActionValue = [](const BodyTracking::FingerJointState& joint1,
                                             const BodyTracking::FingerJointState& joint2) {
                static constexpr float NearDistance = 0.01f;
                static constexpr float FarDistance = 0.03f;

                // Compute the distance between the two joints, and subtract the radii.
                const float distance = std::max(
                    Length(XrVector3f{joint1.Pose.position.x, joint1.Pose.position.y, joint1.Pose.position.z} -
                           XrVector3f{joint2.Pose.position.x, joint2.Pose.position.y, joint2.Pose.position.z}) -
                        joint1.Radius - joint2.Radius,
                    0.f);

                return 1.f -
                       (std::clamp(distance, NearDistance, FarDistance) - NearDistance) / (FarDistance - NearDistance);
            };
            static constexpr float Threshold = 0.9f;

            // Pinch.
            m_cachedInputState.IndexTrigger[side] = aimState.PinchStrengthIndex;

            if (otherJointsValid) {
                // Y.
                if (side == xr::Side::Left) {
                    if (jointActionValue(joints[XR_HAND_JOINT_PALM_EXT], otherJoints[XR_HAND_JOINT_INDEX_TIP_EXT]) >
                        Threshold) {
                        m_cachedInputState.Buttons |= ovrButton_Y;
                    }
                }

                // B.
                if (side == xr::Side::Right) {
                    if (jointActionValue(joints[XR_HAND_JOINT_PALM_EXT], otherJoints[XR_HAND_JOINT_INDEX_TIP_EXT]) >
                        Threshold) {
                        m_cachedInputState.Buttons |= ovrButton_B;
                    }
                }
            }

            TraceLoggingWrite(
                g_traceProvider,
                "HandGestures",
                TLArg(side == 0 ? "Left" : "Right", "Side"),
                TLArg(m_cachedInputState.Buttons & (side == 0 ? ovrButton_LMask : ovrButton_RMask), "Buttons"),
                TLArg(m_cachedInputState.IndexTrigger[side], "IndexTrigger"),
                TLArg(m_cachedInputState.HandTrigger[side], "HandTrigger"),
                TLArg(
                    fmt::format("{}, {}", m_cachedInputState.Thumbstick[side].x, m_cachedInputState.Thumbstick[side].y)
                        .c_str(),
                    "Joystick"));
        } else {
            TraceLoggingWrite(g_traceProvider,
                              "HandGestures",
                              TLArg(side == xr::Side::Left ? "Left" : "Right", "Side"),
                              TLArg(!!m_cachedBodyState.LeftHandActive, "LeftHandActive"),
                              TLArg(!!m_cachedBodyState.RightHandActive, "RightHandActive"));
        }
    }

    // Get the pinch pose (replacing aim pose).
    bool OpenXrRuntime::getPinchPose(int side, const XrPosef& controllerPose, XrPosef& pose) const {
        std::shared_lock lock(m_bodyStateMutex);

        if (m_bodyState &&
            ((side == xr::Side::Left && m_cachedBodyState.LeftHandActive) || m_cachedBodyState.RightHandActive)) {
            const BodyTracking::HandTrackingAimState& aimState =
                side == xr::Side::Left ? m_cachedBodyState.LeftAimState : m_cachedBodyState.RightAimState;
            const bool isAimValid = aimState.AimStatus & XR_HAND_TRACKING_AIM_VALID_BIT_FB;

            TraceLoggingWrite(g_traceProvider,
                              "HandGestures",
                              TLArg(side == xr::Side::Left ? "Left" : "Right", "Side"),
                              TLArg(isAimValid, "IsAimValid"));
            if (!isAimValid) {
                return false;
            }

            // Virtual Desktop queries the joints in local or stage space depending on whether Stage Tracking is
            // enabled. We need to offset to the virtual space.
            assert(ovr_GetTrackingOriginType(m_ovrSession) == ovrTrackingOrigin_FloorLevel);
            const float floorHeight = ovr_GetFloat(m_ovrSession, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);
            TraceLoggingWrite(g_traceProvider, "OVR_GetConfig", TLArg(floorHeight, "EyeHeight"));
            const XrPosef baseToVirtual =
                (std::abs(floorHeight) >= FLT_EPSILON) ? Pose::Translation({0, floorHeight, 0}) : Pose::Identity();

            pose = Pose::Multiply(
                Pose::MakePose(
                    XrQuaternionf{aimState.AimPose.orientation.x,
                                  aimState.AimPose.orientation.y,
                                  aimState.AimPose.orientation.z,
                                  aimState.AimPose.orientation.w},
                    XrVector3f{aimState.AimPose.position.x, aimState.AimPose.position.y, aimState.AimPose.position.z}),
                baseToVirtual);

            return true;
        } else {
            TraceLoggingWrite(g_traceProvider,
                              "PinchPose",
                              TLArg(side == xr::Side::Left ? "Left" : "Right", "Side"),
                              TLArg(!!m_cachedBodyState.LeftHandActive, "LeftHandActive"),
                              TLArg(!!m_cachedBodyState.RightHandActive, "RightHandActive"));
            return false;
        }
    }

} // namespace virtualdesktop_openxr
