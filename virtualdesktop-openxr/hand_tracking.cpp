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

namespace {

    using namespace virtualdesktop_openxr;
    using namespace xr::math;

    void convertSteamVRBonesToFingerJoints(uint32_t side,
                                           const XrPosef& basePose,
                                           BodyTracking::FingerJointState* joints,
                                           vr::VRBoneTransform_t* bones) {
        const auto vrPoseToXrPose = [](const vr::VRBoneTransform_t& vrPose) {
            XrPosef xrPose;
            xrPose.position.x = vrPose.position.v[0];
            xrPose.position.y = vrPose.position.v[1];
            xrPose.position.z = vrPose.position.v[2];
            xrPose.orientation.x = vrPose.orientation.x;
            xrPose.orientation.y = vrPose.orientation.y;
            xrPose.orientation.z = vrPose.orientation.z;
            xrPose.orientation.w = vrPose.orientation.w;

            return xrPose;
        };

        const auto xrPoseToBodyTrackingPose = [](const XrPosef& xrPose) {
            BodyTracking::Pose bodyTrackingPose;
            bodyTrackingPose.position.x = xrPose.position.x;
            bodyTrackingPose.position.y = xrPose.position.y;
            bodyTrackingPose.position.z = xrPose.position.z;
            bodyTrackingPose.orientation.x = xrPose.orientation.x;
            bodyTrackingPose.orientation.y = xrPose.orientation.y;
            bodyTrackingPose.orientation.z = xrPose.orientation.z;
            bodyTrackingPose.orientation.w = xrPose.orientation.w;
            return bodyTrackingPose;
        };

        // We must apply the transforms in order of the bone structure:
        // https://github.com/ValveSoftware/openvr/wiki/Hand-Skeleton#bone-structure
        XrVector3f barycenter{};
        XrPosef accumulatedPose = basePose;
        XrPosef wristPose;
        for (uint32_t i = 0; i <= eBone_PinkyFinger4; i++) {
            accumulatedPose = Pose::Multiply(vrPoseToXrPose(bones[i]), accumulatedPose);

            // Palm is estimated after this loop.
            if (i != XR_HAND_JOINT_PALM_EXT) {
                // We need extra rotations to convert from what SteamVR expects to what OpenXR expects.
                XrPosef correctedPose;
                if (i != XR_HAND_JOINT_WRIST_EXT) {
                    correctedPose = Pose::Multiply(
                        Pose::Orientation({(side == xr::Side::Left) ? 0.f : (float)M_PI, (float)-M_PI_2, (float)M_PI}),
                        accumulatedPose);
                } else {
                    correctedPose = Pose::Multiply(
                        Pose::Orientation(
                            {(float)M_PI, 0.f, (side == xr::Side::Left) ? (float)-M_PI_2 : (float)M_PI_2}),
                        accumulatedPose);
                }
                joints[i].Pose = xrPoseToBodyTrackingPose(correctedPose);
            }

            switch (i) {
            case XR_HAND_JOINT_WRIST_EXT:
                joints[i].Radius = 0.016f;
                wristPose = accumulatedPose;
                break;

            case XR_HAND_JOINT_INDEX_METACARPAL_EXT:
            case XR_HAND_JOINT_INDEX_PROXIMAL_EXT:
            case XR_HAND_JOINT_MIDDLE_METACARPAL_EXT:
            case XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT:
            case XR_HAND_JOINT_RING_METACARPAL_EXT:
            case XR_HAND_JOINT_RING_PROXIMAL_EXT:
            case XR_HAND_JOINT_LITTLE_METACARPAL_EXT:
            case XR_HAND_JOINT_LITTLE_PROXIMAL_EXT:
                joints[i].Radius = 0.016f;
                barycenter = barycenter + accumulatedPose.position;
                break;

            // Reset to the wrist base pose once we reach the tip.
            case XR_HAND_JOINT_THUMB_TIP_EXT:
            case XR_HAND_JOINT_INDEX_TIP_EXT:
            case XR_HAND_JOINT_MIDDLE_TIP_EXT:
            case XR_HAND_JOINT_RING_TIP_EXT:
            case XR_HAND_JOINT_LITTLE_TIP_EXT:
                joints[i].Radius = 0.008f;
                accumulatedPose = wristPose;
                break;

            default:
                joints[i].Radius = 0.008f;
                break;
            }
        }

        // SteamVR doesn't have palm, we compute the barycenter of the metacarpal and proximal for
        // index/middle/ring/little fingers.
        barycenter = barycenter / 8.0f;
        joints[XR_HAND_JOINT_PALM_EXT].Radius = 0.016f;
        joints[XR_HAND_JOINT_PALM_EXT].Pose = xrPoseToBodyTrackingPose(
            Pose::MakePose(joints[XR_HAND_JOINT_MIDDLE_METACARPAL_EXT].Pose.orientation, barycenter));
    }

} // namespace

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

        const XrHandTrackingDataSourceInfoEXT* dataSource = nullptr;

        const XrBaseInStructure* entry = reinterpret_cast<const XrBaseInStructure*>(createInfo->next);
        while (entry) {
            switch (entry->type) {
            case XR_TYPE_HAND_TRACKING_DATA_SOURCE_INFO_EXT:
                dataSource = reinterpret_cast<const XrHandTrackingDataSourceInfoEXT*>(entry);
                break;
            }

            entry = reinterpret_cast<const XrBaseInStructure*>(entry->next);
        }

        std::unique_lock lock(m_handTrackersMutex);

        HandTracker& xrHandTracker = *new HandTracker;
        xrHandTracker.side = createInfo->hand == XR_HAND_LEFT_EXT ? xr::Side::Left : xr::Side::Right;

        // By default, we always want optical hand tracking and we want data simulated from the motion controller iff
        // Index Controller emulation is enabled. However, the OculusXR plugin does not use the data correctly, so we
        // also exclude it.
        xrHandTracker.useOpticalTracking = true;
        xrHandTracker.useHandJointsSimulation = m_emulateIndexControllers && !m_isOculusXrPlugin;

        if (has_XR_EXT_hand_tracking_data_source && dataSource) {
            xrHandTracker.useOpticalTracking = xrHandTracker.useHandJointsSimulation = false;
            for (uint32_t i = 0; i < dataSource->requestedDataSourceCount; i++) {
                switch (dataSource->requestedDataSources[i]) {
                case XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT:
                    xrHandTracker.useOpticalTracking = true;
                    break;
                case XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT:
                    xrHandTracker.useHandJointsSimulation = true;
                    break;
                }
            }
        }

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

        if (locateInfo->time <= 0) {
            return XR_ERROR_TIME_INVALID;
        }

        std::shared_lock lock(m_handTrackersMutex);
        std::shared_lock lock2(m_actionsAndSpacesMutex);

        if (!m_handTrackers.count(handTracker) || !m_spaces.count(locateInfo->baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        XrHandJointVelocitiesEXT* velocities = nullptr;
        XrHandTrackingAimStateFB* aimState = nullptr;
        XrHandTrackingDataSourceStateEXT* dataSourceState = nullptr;

        XrBaseOutStructure* entry = reinterpret_cast<XrBaseOutStructure*>(locations->next);
        while (entry) {
            switch (entry->type) {
            case XR_TYPE_HAND_JOINT_VELOCITIES_EXT:
                velocities = reinterpret_cast<XrHandJointVelocitiesEXT*>(entry);
                break;
            case XR_TYPE_HAND_TRACKING_AIM_STATE_FB:
                aimState = reinterpret_cast<XrHandTrackingAimStateFB*>(entry);
                break;
            case XR_TYPE_HAND_TRACKING_DATA_SOURCE_STATE_EXT:
                dataSourceState = reinterpret_cast<XrHandTrackingDataSourceStateEXT*>(entry);
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

        BodyTracking::FingerJointState simulationJointStates[XR_HAND_JOINT_COUNT_EXT];
        BodyTracking::FingerJointState* joints = nullptr;

        {
            std::shared_lock lock(m_bodyStateMutex);

            locations->isActive = XR_FALSE;

            if (has_XR_EXT_hand_tracking_data_source && dataSourceState) {
                dataSourceState->isActive = XR_FALSE;
            }

            // Check the hand state.
            bool needHeightAdjustment = true;
            if (m_bodyState && xrHandTracker.useOpticalTracking &&
                ((xrHandTracker.side == xr::Side::Left && m_cachedBodyState.LeftHandActive) ||
                 (xrHandTracker.side == xr::Side::Right && m_cachedBodyState.RightHandActive))) {
                joints = xrHandTracker.side == xr::Side::Left ? m_cachedBodyState.LeftHandJointStates
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

                if (has_XR_EXT_hand_tracking_data_source && dataSourceState) {
                    dataSourceState->isActive = XR_TRUE;
                    dataSourceState->dataSource = XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT;
                }

            } else if (xrHandTracker.useHandJointsSimulation) {
                XrPosef basePose = Pose::Identity();
                const auto flags2 = getControllerPose(xrHandTracker.side, locateInfo->time, basePose, nullptr);

                TraceLoggingWrite(g_traceProvider,
                                  "xrLocateHandJointsEXT",
                                  TLArg(xrHandTracker.side == xr::Side::Left ? "Left" : "Right", "Side"),
                                  TLArg(!!m_cachedBodyState.LeftHandActive, "LeftHandActive"),
                                  TLArg(!!m_cachedBodyState.RightHandActive, "RightHandActive"),
                                  TLArg(flags2, "ControllerLocationFlags"));

                if (Pose::IsPoseValid(flags2)) {
                    // Use hand simulation.
                    const uint32_t side = xrHandTracker.side == xr::Side::Left ? 0 : 1;
                    vr::VRBoneTransform_t bones[eBone_Count];
                    MyFingerCurls curls{};
                    const bool touchA = xrHandTracker.side == xr::Side::Left
                                            ? (m_cachedInputState.Touches & ovrButton_X)
                                            : (m_cachedInputState.Touches & ovrButton_A);
                    const bool touchB = xrHandTracker.side == xr::Side::Left
                                            ? (m_cachedInputState.Touches & ovrButton_Y)
                                            : (m_cachedInputState.Touches & ovrButton_B);
                    curls.thumb = touchB ? 1.f : touchA ? 0.5f : 0.f;
                    curls.index = m_cachedInputState.IndexTrigger[side];
                    curls.middle = curls.ring = curls.pinky = m_cachedInputState.HandTrigger[side];
                    m_handSimulation[side].ComputeSkeletonTransforms(xrHandTracker.side == xr::Side::Left
                                                                         ? vr::TrackedControllerRole_LeftHand
                                                                         : vr::TrackedControllerRole_RightHand,
                                                                     curls,
                                                                     {},
                                                                     bones);
                    convertSteamVRBonesToFingerJoints(xrHandTracker.side,
                                                      Pose::Multiply(m_controllerHandPose[side], basePose),
                                                      simulationJointStates,
                                                      bones);
                    joints = simulationJointStates;
                    needHeightAdjustment = false;

                    if (has_XR_EXT_hand_tracking_data_source && dataSourceState) {
                        dataSourceState->isActive = XR_TRUE;
                        dataSourceState->dataSource = XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT;
                    }

                    locations->isActive = XR_TRUE;
                }
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

            XrPosef jointsToVirtual = Pose::Identity();
            if (needHeightAdjustment) {
                // Virtual Desktop queries the joints in local or stage space depending on whether Stage Tracking is
                // enabled. We need to offset to the virtual space.
                assert(ovr_GetTrackingOriginType(m_ovrSession) == ovrTrackingOrigin_FloorLevel);
                const float floorHeight = ovr_GetFloat(m_ovrSession, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);
                TraceLoggingWrite(g_traceProvider, "OVR_GetConfig", TLArg(floorHeight, "EyeHeight"));
                jointsToVirtual =
                    (std::abs(floorHeight) >= FLT_EPSILON) ? Pose::Translation({0, floorHeight, 0}) : Pose::Identity();
            }
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
