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

// Implements the necessary support for the XR_FB_body_tracking and XR_META_body_tracking_full_body extensions:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_body_tracking
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_META_body_tracking_full_body

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateBodyTrackerFB
    XrResult OpenXrRuntime::xrCreateBodyTrackerFB(XrSession session,
                                                  const XrBodyTrackerCreateInfoFB* createInfo,
                                                  XrBodyTrackerFB* bodyTracker) {
        if (createInfo->type != XR_TYPE_BODY_TRACKER_CREATE_INFO_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateBodyTrackerFB",
                          TLXArg(session, "Session"),
                          TLArg((uint32_t)createInfo->bodyJointSet, "BodyJointSet"));

        if (!has_XR_FB_body_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const bool useFullBody = createInfo->bodyJointSet == XR_BODY_JOINT_SET_FULL_BODY_META;
        if (!m_supportsBodyTracking || (useFullBody && !m_supportsFullBodyTracking)) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        if (createInfo->bodyJointSet != XR_BODY_JOINT_SET_DEFAULT_FB &&
            (!has_XR_META_body_tracking_full_body || !useFullBody)) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        BodyTracker& xrBodyTracker = *new BodyTracker;

        *bodyTracker = (XrBodyTrackerFB)&xrBodyTracker;
        xrBodyTracker.useFullBody = useFullBody;

        // Maintain a list of known trackers for validation.
        m_bodyTrackers.insert(*bodyTracker);

        TraceLoggingWrite(g_traceProvider, "xrCreateBodyTrackerFB", TLXArg(*bodyTracker, "BodyTracker"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyBodyTrackerFB
    XrResult OpenXrRuntime::xrDestroyBodyTrackerFB(XrBodyTrackerFB bodyTracker) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyBodyTrackerFB", TLXArg(bodyTracker, "BodyTracker"));

        if (!has_XR_FB_body_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_bodyTrackers.count(bodyTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        BodyTracker* xrBodyTracker = (BodyTracker*)bodyTracker;

        delete xrBodyTracker;
        m_bodyTrackers.erase(bodyTracker);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrLocateBodyJointsFB
    XrResult OpenXrRuntime::xrLocateBodyJointsFB(XrBodyTrackerFB bodyTracker,
                                                 const XrBodyJointsLocateInfoFB* locateInfo,
                                                 XrBodyJointLocationsFB* locations) {
        if (locateInfo->type != XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB ||
            locations->type != XR_TYPE_BODY_JOINT_LOCATIONS_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrLocateBodyJointsFB",
                          TLXArg(bodyTracker, "BodyTracker"),
                          TLArg(locateInfo->time),
                          TLXArg(locateInfo->baseSpace));

        if (!has_XR_FB_body_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_bodyTrackers.count(bodyTracker) || !m_spaces.count(locateInfo->baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const BodyTracker& xrBodyTracker = *(BodyTracker*)bodyTracker;

        if ((!xrBodyTracker.useFullBody && locations->jointCount != XR_BODY_JOINT_COUNT_FB) ||
            (xrBodyTracker.useFullBody && locations->jointCount != XR_FULL_BODY_JOINT_COUNT_META)) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        Space& xrBaseSpace = *(Space*)locateInfo->baseSpace;

        XrPosef baseSpaceToVirtual = Pose::Identity();
        const auto flags = locateSpaceToOrigin(xrBaseSpace, locateInfo->time, baseSpaceToVirtual, nullptr, nullptr);

        // Check the hand state.
        if (m_bodyState && m_bodyState->BodyTrackingConfidence > 0.f) {
            const BodyTracking::BodyJointLocation* const joints = m_bodyState->BodyJoints;

            TraceLoggingWrite(
                g_traceProvider,
                "xrLocateBodyJointsFB",
                TLArg(m_bodyState->BodyTrackingConfidence, "BodyTrackingConfidence"),
                TLArg(joints[XR_FULL_BODY_JOINT_ROOT_META].LocationFlags, "RootLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_ROOT_META].Pose).c_str(), "Root"),
                TLArg(joints[XR_FULL_BODY_JOINT_HIPS_META].LocationFlags, "HipsLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_HIPS_META].Pose).c_str(), "Hips"),
                TLArg(joints[XR_FULL_BODY_JOINT_HEAD_META].LocationFlags, "HeadLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_HEAD_META].Pose).c_str(), "Head"),
                TLArg(joints[XR_FULL_BODY_JOINT_LEFT_HAND_PALM_META].LocationFlags, "LeftPalmLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_LEFT_HAND_PALM_META].Pose).c_str(), "LeftPalm"),
                TLArg(joints[XR_FULL_BODY_JOINT_RIGHT_HAND_PALM_META].LocationFlags, "RightPalmLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_RIGHT_HAND_PALM_META].Pose).c_str(), "RightPalm"),
                TLArg(joints[XR_FULL_BODY_JOINT_LEFT_FOOT_BALL_META].LocationFlags, "LeftFootLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_LEFT_FOOT_BALL_META].Pose).c_str(), "LeftFoot"),
                TLArg(joints[XR_FULL_BODY_JOINT_RIGHT_FOOT_BALL_META].LocationFlags, "RightFootLocationFlags"),
                TLArg(xr::ToString(joints[XR_FULL_BODY_JOINT_RIGHT_FOOT_BALL_META].Pose).c_str(), "RightFoot"));

            locations->isActive = XR_TRUE;

        } else {
            TraceLoggingWrite(g_traceProvider,
                              "xrLocateBodyJointsFB",
                              TLArg(m_bodyState->BodyTrackingConfidence, "BodyTrackingConfidence"));

            locations->isActive = XR_FALSE;
        }

        // If base space pose is not valid, we cannot locate.
        if (locations->isActive != XR_TRUE || !Pose::IsPoseValid(flags)) {
            TraceLoggingWrite(g_traceProvider, "xrLocateBodyJointsFB", TLArg(0, "LocationFlags"));
            locations->confidence = 0.f;
            for (uint32_t i = 0; i < locations->jointCount; i++) {
                locations->jointLocations[i].pose = Pose::Identity();
                locations->jointLocations[i].locationFlags = 0;
            }
            locations->skeletonChangedCount = 0;
            return XR_SUCCESS;
        }

        // Virtual Desktop queries the joints in local or stage space depending on whether Stage Tracking is
        // enabled. We need to offset to the virtual space.
        assert(ovr_GetTrackingOriginType(m_ovrSession) == ovrTrackingOrigin_FloorLevel);
        const float floorHeight = ovr_GetFloat(m_ovrSession, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);
        TraceLoggingWrite(g_traceProvider, "OVR_GetConfig", TLArg(floorHeight, "EyeHeight"));
        const XrPosef jointsToVirtual =
            (std::abs(floorHeight) >= FLT_EPSILON) ? Pose::Translation({0, floorHeight, 0}) : Pose::Identity();
        const XrPosef basePose = Pose::Multiply(jointsToVirtual, Pose::Invert(baseSpaceToVirtual));

        locations->confidence = m_bodyState->BodyTrackingConfidence;
        for (uint32_t i = 0; i < locations->jointCount; i++) {
            locations->jointLocations[i].locationFlags = m_bodyState->BodyJoints[i].LocationFlags;
            if (Pose::IsPoseValid(locations->jointLocations[i].locationFlags)) {
                const XrPosef poseOfBodyJoint = Pose::Multiply(
                    xr::math::Pose::MakePose(XrQuaternionf{m_bodyState->BodyJoints[i].Pose.orientation.x,
                                                           m_bodyState->BodyJoints[i].Pose.orientation.y,
                                                           m_bodyState->BodyJoints[i].Pose.orientation.z,
                                                           m_bodyState->BodyJoints[i].Pose.orientation.w},
                                             XrVector3f{m_bodyState->BodyJoints[i].Pose.position.x,
                                                        m_bodyState->BodyJoints[i].Pose.position.y,
                                                        m_bodyState->BodyJoints[i].Pose.position.z}),
                    basePose);

                locations->jointLocations[i].pose = Pose::Multiply(poseOfBodyJoint, Pose::Invert(baseSpaceToVirtual));
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrLocateBodyJointsFB",
                              TLArg(i, "JointIndex"),
                              TLArg(locations->jointLocations[i].locationFlags, "LocationFlags"),
                              TLArg(xr::ToString(locations->jointLocations[i].pose).c_str(), "Pose"));
        }

        locations->skeletonChangedCount = m_bodyState->SkeletonChangedCount;

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetBodySkeletonFB
    XrResult OpenXrRuntime::xrGetBodySkeletonFB(XrBodyTrackerFB bodyTracker, XrBodySkeletonFB* skeleton) {
        if (skeleton->type != XR_TYPE_BODY_SKELETON_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetBodySkeletonFB",
                          TLXArg(bodyTracker, "BodyTracker"),
                          TLArg(skeleton->jointCount, "JointsCount"));

        if (!has_XR_FB_body_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_bodyTrackers.count(bodyTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        const BodyTracker& xrBodyTracker = *(BodyTracker*)bodyTracker;

        if ((!xrBodyTracker.useFullBody && skeleton->jointCount != XR_BODY_JOINT_COUNT_FB) ||
            (xrBodyTracker.useFullBody && skeleton->jointCount != XR_FULL_BODY_JOINT_COUNT_META)) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // Forward the state from the memory mapped file.
        if (m_bodyState) {
            for (uint32_t i = 0; i < skeleton->jointCount; i++) {
                skeleton->joints[i].joint = m_bodyState->SkeletonJoints[i].Joint;
                skeleton->joints[i].parentJoint = m_bodyState->SkeletonJoints[i].ParentJoint;
                skeleton->joints[i].pose =
                    xr::math::Pose::MakePose(XrQuaternionf{m_bodyState->SkeletonJoints[i].Pose.orientation.x,
                                                           m_bodyState->SkeletonJoints[i].Pose.orientation.y,
                                                           m_bodyState->SkeletonJoints[i].Pose.orientation.z,
                                                           m_bodyState->SkeletonJoints[i].Pose.orientation.w},
                                             XrVector3f{m_bodyState->SkeletonJoints[i].Pose.position.x,
                                                        m_bodyState->SkeletonJoints[i].Pose.position.y,
                                                        m_bodyState->SkeletonJoints[i].Pose.position.z});
            }
        } else {
            for (uint32_t i = 0; i < skeleton->jointCount; i++) {
                skeleton->joints[i].joint = i;
                skeleton->joints[i].parentJoint = 0;
                skeleton->joints[i].pose = {};
            }
        }

        TraceLoggingWrite(
            g_traceProvider,
            "xrGetBodySkeletonFB",
            TLArg(m_bodyState->BodyTrackingConfidence, "BodyTrackingConfidence"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_ROOT_META].joint).c_str(), "Root"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_HIPS_META].joint).c_str(), "Hips"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_HEAD_META].joint).c_str(), "Head"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_LEFT_HAND_PALM_META].joint).c_str(), "LeftPalm"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_RIGHT_HAND_PALM_META].joint).c_str(), "RightPalm"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_LEFT_FOOT_BALL_META].joint).c_str(), "LeftFoot"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_RIGHT_FOOT_BALL_META].joint).c_str(), "RightFoot"));

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr
