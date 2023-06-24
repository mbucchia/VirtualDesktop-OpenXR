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

// Implements the necessary support for the XR_EXT_hand_tracking extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;
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

        LOG_TELEMETRY_ONCE(logFeature("HandTracking"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if ((createInfo->hand != XR_HAND_LEFT_EXT && createInfo->hand != XR_HAND_RIGHT_EXT) ||
            createInfo->handJointSet != XR_HAND_JOINT_SET_DEFAULT_EXT) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        HandTracker& xrHandTracker = *new HandTracker;
        xrHandTracker.side = createInfo->hand == XR_HAND_LEFT_EXT ? 0 : 1;

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

        if (!m_handTrackers.count(handTracker) || !m_spaces.count(locateInfo->baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const XrHandJointsMotionRangeInfoEXT* motionRange =
            reinterpret_cast<const XrHandJointsMotionRangeInfoEXT*>(locateInfo->next);
        if (has_XR_EXT_hand_joints_motion_range) {
            while (motionRange) {
                if (motionRange->type == XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT) {
                    break;
                }
                motionRange = reinterpret_cast<const XrHandJointsMotionRangeInfoEXT*>(motionRange->next);
            }
        }

        XrHandJointVelocitiesEXT* velocities = reinterpret_cast<XrHandJointVelocitiesEXT*>(locations->next);
        while (velocities) {
            if (velocities->type == XR_TYPE_HAND_JOINT_VELOCITIES_EXT) {
                break;
            }
            velocities = reinterpret_cast<XrHandJointVelocitiesEXT*>(velocities->next);
        }

        if (locations->jointCount != XR_HAND_JOINT_COUNT_EXT ||
            (velocities && velocities->jointCount != XR_HAND_JOINT_COUNT_EXT)) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        HandTracker& xrHandTracker = *(HandTracker*)handTracker;

        pvrSkeletalMotionRange range = pvrSkeletalMotionRange_WithoutController;
        if (motionRange) {
            range = motionRange->handJointsMotionRange == XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT
                        ? pvrSkeletalMotionRange_WithoutController
                        : pvrSkeletalMotionRange_WithController;
        }

        Space& xrBaseSpace = *(Space*)locateInfo->baseSpace;

        XrPosef baseSpaceToVirtual = Pose::Identity();
        XrPosef basePose = Pose::Identity();
        const auto flags1 = locateSpaceToOrigin(xrBaseSpace, locateInfo->time, baseSpaceToVirtual, nullptr, nullptr);
        const auto flags2 = getControllerPose(xrHandTracker.side, locateInfo->time, basePose, nullptr);

        pvrSkeletalData skeletalData{};
        const auto result = pvr_getSkeletalData(m_pvrSession,
                                                xrHandTracker.side == 0 ? pvrTrackedDevice_LeftController
                                                                        : pvrTrackedDevice_RightController,
                                                range,
                                                &skeletalData);
        if (result == pvr_not_support || skeletalData.boneCount == 0) {
            TraceLoggingWrite(g_traceProvider,
                              "PVR_SkeletalData",
                              TLArg(xrHandTracker.side == 0 ? "Left" : "Right", "Side"),
                              TLArg(xr::ToString(result).c_str(), "Result"),
                              TLArg(skeletalData.boneCount, "Count"));

            // This is how we detect no hands presence.
            locations->isActive = XR_FALSE;

        } else {
            CHECK_PVRCMD(result);

            // We rely on PVR using the same definitions as SteamVR, which turn out to be share (almost) the same first
            // 26 joints with the OpenXr definitions. https://github.com/ValveSoftware/openvr/wiki/Hand-Skeleton
            TraceLoggingWrite(
                g_traceProvider,
                "PVR_SkeletalData",
                TLArg(xrHandTracker.side == 0 ? "Left" : "Right", "Side"),
                TLArg(skeletalData.boneCount, "Count"),
                TLArg(xr::ToString(skeletalData.boneTransforms[0]).c_str(), "Root"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_WRIST_EXT]).c_str(), "Wrist"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_THUMB_METACARPAL_EXT]).c_str(),
                      "ThumbMetacarpal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_THUMB_PROXIMAL_EXT]).c_str(),
                      "ThumbProximal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_THUMB_DISTAL_EXT]).c_str(), "ThumbDistal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_THUMB_TIP_EXT]).c_str(), "ThumbTip"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_INDEX_METACARPAL_EXT]).c_str(),
                      "IndexMetacarpal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_INDEX_PROXIMAL_EXT]).c_str(),
                      "IndexProximal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT]).c_str(),
                      "IndexIntermediate"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_INDEX_DISTAL_EXT]).c_str(), "IndexDistal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_INDEX_TIP_EXT]).c_str(), "IndexTip"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_MIDDLE_METACARPAL_EXT]).c_str(),
                      "MiddleMetacarpal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT]).c_str(),
                      "MiddleProximal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT]).c_str(),
                      "MiddleIntermediate"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_MIDDLE_DISTAL_EXT]).c_str(),
                      "MiddleDistal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_MIDDLE_TIP_EXT]).c_str(), "MiddleTip"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_RING_METACARPAL_EXT]).c_str(),
                      "RingMetacarpal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_RING_PROXIMAL_EXT]).c_str(),
                      "RingProximal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_RING_INTERMEDIATE_EXT]).c_str(),
                      "RingIntermediate"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_RING_DISTAL_EXT]).c_str(), "RingDistal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_RING_TIP_EXT]).c_str(), "RingTip"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_LITTLE_METACARPAL_EXT]).c_str(),
                      "LittleMetacarpal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_LITTLE_PROXIMAL_EXT]).c_str(),
                      "LittleProximal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT]).c_str(),
                      "LittleIntermediate"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_LITTLE_DISTAL_EXT]).c_str(),
                      "LittleDistal"),
                TLArg(xr::ToString(skeletalData.boneTransforms[XR_HAND_JOINT_LITTLE_TIP_EXT]).c_str(), "LittleTip"));

            locations->isActive = XR_TRUE;
        }

        // If base space pose is not valid, we cannot locate.
        if (locations->isActive != XR_TRUE || !Pose::IsPoseValid(flags1) || !Pose::IsPoseValid(flags2)) {
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

        // We must apply the transforms in order of the bone structure:
        // https://github.com/ValveSoftware/openvr/wiki/Hand-Skeleton#bone-structure
        XrVector3f barycenter{};
        XrPosef accumulatedPose = basePose;
        XrPosef wristPose;
        for (uint32_t i = 0; i < locations->jointCount; i++) {
            accumulatedPose = Pose::Multiply(pvrPoseToXrPose(skeletalData.boneTransforms[i]), accumulatedPose);

            // Palm is estimated after this loop.
            if (i != XR_HAND_JOINT_PALM_EXT) {
                locations->jointLocations[i].radius = 0.005f;

                // We need extra rotations to convert from what SteamVR expects to what OpenXR expects.
                XrPosef correctedPose;
                if (i != XR_HAND_JOINT_WRIST_EXT) {
                    correctedPose =
                        Pose::Multiply(Pose::MakePose(Quaternion::RotationRollPitchYaw(
                                                          {PVR::DegreeToRad(!xrHandTracker.side ? 0.f : 180.f),
                                                           PVR::DegreeToRad(-90.f),
                                                           PVR::DegreeToRad(180.f)}),
                                                      XrVector3f{0, 0, 0}),
                                       accumulatedPose);
                } else {
                    correctedPose =
                        Pose::Multiply(Pose::MakePose(Quaternion::RotationRollPitchYaw(
                                                          {PVR::DegreeToRad(180.f),
                                                           PVR::DegreeToRad(0.f),
                                                           PVR::DegreeToRad(!xrHandTracker.side ? -90.f : 90.f)}),
                                                      XrVector3f{0, 0, 0}),
                                       accumulatedPose);
                }
                locations->jointLocations[i].pose = Pose::Multiply(correctedPose, Pose::Invert(baseSpaceToVirtual));
            }
            locations->jointLocations[i].locationFlags =
                (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) | flags2;

            switch (i) {
            case XR_HAND_JOINT_WRIST_EXT:
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
                barycenter = barycenter + accumulatedPose.position;
                break;

            // Reset to the wrist base pose once we reach the tip.
            case XR_HAND_JOINT_THUMB_TIP_EXT:
            case XR_HAND_JOINT_INDEX_TIP_EXT:
            case XR_HAND_JOINT_MIDDLE_TIP_EXT:
            case XR_HAND_JOINT_RING_TIP_EXT:
            case XR_HAND_JOINT_LITTLE_TIP_EXT:
                accumulatedPose = wristPose;
                break;
            }

            if (velocities) {
                velocities->jointVelocities[i].angularVelocity = {};
                velocities->jointVelocities[i].linearVelocity = {};
                velocities->jointVelocities[i].velocityFlags = 0;
            }
        }

        // SteamVR doesn't have palm, we compute the barycenter of the metacarpal and proximal for
        // index/middle/ring/little fingers.
        barycenter = barycenter / 8.0f;
        locations->jointLocations[XR_HAND_JOINT_PALM_EXT].radius = 0.04f;
        locations->jointLocations[XR_HAND_JOINT_PALM_EXT].pose = Pose::Multiply(
            Pose::MakePose(locations->jointLocations[XR_HAND_JOINT_MIDDLE_METACARPAL_EXT].pose.orientation, barycenter),
            Pose::Invert(baseSpaceToVirtual));

        return XR_SUCCESS;
    }

} // namespace pimax_openxr