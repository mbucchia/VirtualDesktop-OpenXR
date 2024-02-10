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
#include "trackers.h"
#include "utils.h"

// Implements the necessary support for the XR_FB_body_tracking and XR_META_body_tracking_full_body extensions:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_body_tracking
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_META_body_tracking_full_body

// Implement emulation for XR_HTCX_vive_tracker_interaction using the body tracking data.
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_HTCX_vive_tracker_interaction

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

        std::shared_lock lock(m_bodyTrackersMutex);
        std::shared_lock lock2(m_actionsAndSpacesMutex);

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

        {
            std::shared_lock lock(m_bodyStateMutex);

            // Check the hand state.
            if (m_bodyState && m_cachedBodyState.BodyTrackingConfidence > 0.f) {
                const BodyTracking::BodyJointLocation* const joints = m_cachedBodyState.BodyJoints;

                TraceLoggingWrite(
                    g_traceProvider,
                    "xrLocateBodyJointsFB",
                    TLArg(m_cachedBodyState.BodyTrackingConfidence, "BodyTrackingConfidence"),
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
                                  TLArg(m_cachedBodyState.BodyTrackingConfidence, "BodyTrackingConfidence"));

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

            locations->confidence = m_cachedBodyState.BodyTrackingConfidence;
            for (uint32_t i = 0; i < locations->jointCount; i++) {
                locations->jointLocations[i].locationFlags = m_cachedBodyState.BodyJoints[i].LocationFlags;
                if (Pose::IsPoseValid(locations->jointLocations[i].locationFlags)) {
                    const XrPosef poseOfBodyJoint = Pose::Multiply(
                        xr::math::Pose::MakePose(XrQuaternionf{m_cachedBodyState.BodyJoints[i].Pose.orientation.x,
                                                               m_cachedBodyState.BodyJoints[i].Pose.orientation.y,
                                                               m_cachedBodyState.BodyJoints[i].Pose.orientation.z,
                                                               m_cachedBodyState.BodyJoints[i].Pose.orientation.w},
                                                 XrVector3f{m_cachedBodyState.BodyJoints[i].Pose.position.x,
                                                            m_cachedBodyState.BodyJoints[i].Pose.position.y,
                                                            m_cachedBodyState.BodyJoints[i].Pose.position.z}),
                        basePose);

                    locations->jointLocations[i].pose =
                        Pose::Multiply(poseOfBodyJoint, Pose::Invert(baseSpaceToVirtual));
                }

                TraceLoggingWrite(g_traceProvider,
                                  "xrLocateBodyJointsFB",
                                  TLArg(i, "JointIndex"),
                                  TLArg(locations->jointLocations[i].locationFlags, "LocationFlags"),
                                  TLArg(xr::ToString(locations->jointLocations[i].pose).c_str(), "Pose"));
            }

            locations->skeletonChangedCount = m_cachedBodyState.SkeletonChangedCount;
        }

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

        std::shared_lock lock(m_bodyTrackersMutex);

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
            std::shared_lock lock(m_bodyStateMutex);

            for (uint32_t i = 0; i < skeleton->jointCount; i++) {
                skeleton->joints[i].joint = m_cachedBodyState.SkeletonJoints[i].Joint;
                skeleton->joints[i].parentJoint = m_cachedBodyState.SkeletonJoints[i].ParentJoint;
                skeleton->joints[i].pose =
                    xr::math::Pose::MakePose(XrQuaternionf{m_cachedBodyState.SkeletonJoints[i].Pose.orientation.x,
                                                           m_cachedBodyState.SkeletonJoints[i].Pose.orientation.y,
                                                           m_cachedBodyState.SkeletonJoints[i].Pose.orientation.z,
                                                           m_cachedBodyState.SkeletonJoints[i].Pose.orientation.w},
                                             XrVector3f{m_cachedBodyState.SkeletonJoints[i].Pose.position.x,
                                                        m_cachedBodyState.SkeletonJoints[i].Pose.position.y,
                                                        m_cachedBodyState.SkeletonJoints[i].Pose.position.z});
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
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_ROOT_META].joint).c_str(), "Root"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_HIPS_META].joint).c_str(), "Hips"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_HEAD_META].joint).c_str(), "Head"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_LEFT_HAND_PALM_META].joint).c_str(), "LeftPalm"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_RIGHT_HAND_PALM_META].joint).c_str(), "RightPalm"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_LEFT_FOOT_BALL_META].joint).c_str(), "LeftFoot"),
            TLArg(xr::ToString(skeleton->joints[XR_FULL_BODY_JOINT_RIGHT_FOOT_BALL_META].joint).c_str(), "RightFoot"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateViveTrackerPathsHTCX
    XrResult OpenXrRuntime::xrEnumerateViveTrackerPathsHTCX(XrInstance instance,
                                                            uint32_t pathCapacityInput,
                                                            uint32_t* pathCountOutput,
                                                            XrViveTrackerPathsHTCX* paths) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateViveTrackerPathsHTCX",
                          TLXArg(instance, "Instance"),
                          TLArg(pathCapacityInput, "PathCapacityInput"));

        uint32_t trackersCount = 0;
        if (m_supportsBodyTracking) {
            for (const auto& role : TrackerRoles) {
                // Ignore lower body joints when not supported.
                if (!m_supportsFullBodyTracking && role.joint >= XR_BODY_JOINT_COUNT_FB) {
                    continue;
                }

                trackersCount++;
            }
        }

        if (pathCapacityInput && pathCapacityInput < trackersCount) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *pathCountOutput = trackersCount;
        TraceLoggingWrite(
            g_traceProvider, "xrEnumerateViveTrackerPathsHTCX", TLArg(*pathCountOutput, "PathCountOutput"));

        if (pathCapacityInput && paths) {
            uint32_t i = 0;
            for (const auto& role : TrackerRoles) {
                // Ignore lower body joints when not supported.
                if (!m_supportsFullBodyTracking && role.joint >= XR_BODY_JOINT_COUNT_FB) {
                    continue;
                }
                if (paths[i].type != XR_TYPE_VIVE_TRACKER_PATHS_HTCX) {
                    return XR_ERROR_VALIDATION_FAILURE;
                }

                const auto trackerSerial = role.role;
                const std::string persistentPath = "/user/vive_tracker_htcx/serial/" + trackerSerial;
                const std::string rolePath = "/user/vive_tracker_htcx/role/" + trackerSerial;
                CHECK_XRCMD(xrStringToPath(XR_NULL_HANDLE, persistentPath.c_str(), &paths[i].persistentPath));
                CHECK_XRCMD(xrStringToPath(XR_NULL_HANDLE, rolePath.c_str(), &paths[i].rolePath));
                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateViveTrackerPathsHTCX",
                                  TLArg(paths[i].persistentPath, "PersistentPath"),
                                  TLArg(paths[i].rolePath, "RolePath"),
                                  TLArg(persistentPath.c_str(), "PersistentPath"),
                                  TLArg(rolePath.c_str(), "RolePath"));
                i++;
            }
        }

        return XR_SUCCESS;
    }

    int OpenXrRuntime::getTrackerIndex(const std::string& path) const {
        std::string role;
        if (startsWith(path, "/user/vive_tracker_htcx/serial/")) {
            role = path.substr(31);
        } else if (startsWith(path, "/user/vive_tracker_htcx/role/")) {
            role = path.substr(29);
        }

        // Trim any component path.
        const size_t offs = role.find('/');
        if (offs != std::string::npos) {
            role = role.substr(0, offs);
        }

        if (!role.empty()) {
            for (uint32_t i = 0; i < std::size(TrackerRoles); i++) {
                if (TrackerRoles[i].role == role) {
                    return i;
                }
            }
        }

        return -1;
    }

    XrSpaceLocationFlags OpenXrRuntime::getBodyJointPose(XrFullBodyJointMETA joint, XrTime time, XrPosef& pose) const {
        std::shared_lock lock(m_bodyStateMutex);

        TraceLoggingWrite(g_traceProvider,
                          "VirtualDesktopBodyTracker",
                          TLArg(m_cachedBodyState.BodyTrackingConfidence, "BodyTrackingConfidence"));
        if (!m_cachedBodyState.BodyTrackingConfidence) {
            return 0;
        }

        const BodyTracking::BodyJointLocation& location = m_cachedBodyState.BodyJoints[joint];
        TraceLoggingWrite(g_traceProvider,
                          "VirtualDesktopBodyTracker",
                          TLArg((int)joint, "JointIndex"),
                          TLArg(location.LocationFlags, "LocationFlags"));
        if (!Pose::IsPoseValid(location.LocationFlags)) {
            return 0;
        }

        // Virtual Desktop queries the joints in local or stage space depending on whether Stage Tracking is
        // enabled. We need to offset to the virtual space.
        assert(ovr_GetTrackingOriginType(m_ovrSession) == ovrTrackingOrigin_FloorLevel);
        const float floorHeight = ovr_GetFloat(m_ovrSession, OVR_KEY_EYE_HEIGHT, OVR_DEFAULT_EYE_HEIGHT);
        TraceLoggingWrite(g_traceProvider, "OVR_GetConfig", TLArg(floorHeight, "EyeHeight"));
        const XrPosef jointsToVirtual =
            (std::abs(floorHeight) >= FLT_EPSILON) ? Pose::Translation({0, floorHeight, 0}) : Pose::Identity();

        pose = Pose::Multiply(
            xr::math::Pose::MakePose(
                XrQuaternionf{location.Pose.orientation.x,
                              location.Pose.orientation.y,
                              location.Pose.orientation.z,
                              location.Pose.orientation.w},
                XrVector3f{location.Pose.position.x, location.Pose.position.y, location.Pose.position.z}),
            jointsToVirtual);

        TraceLoggingWrite(g_traceProvider,
                          "VirtualDesktopBodyTracker",
                          TLArg((int)joint, "JointIndex"),
                          TLArg(xr::ToString(pose).c_str(), "Pose"));

        return location.LocationFlags;
    }

} // namespace virtualdesktop_openxr
