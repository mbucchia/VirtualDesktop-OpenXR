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

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if ((createInfo->hand != XR_HAND_LEFT_EXT && createInfo->hand != XR_HAND_RIGHT_EXT) ||
            createInfo->handJointSet != XR_HAND_JOINT_SET_DEFAULT_EXT) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        std::unique_lock lock(m_handTrackersMutex);

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

        std::unique_lock lock(m_handTrackersMutex);

        if (!m_handTrackers.count(handTracker) || !m_spaces.count(locateInfo->baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
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

        Space& xrBaseSpace = *(Space*)locateInfo->baseSpace;

        XrPosef baseSpaceToVirtual = Pose::Identity();
        XrPosef basePose = Pose::Identity();
        const auto flags1 = locateSpaceToOrigin(xrBaseSpace, locateInfo->time, baseSpaceToVirtual, nullptr, nullptr);
        const auto flags2 = getControllerPose(xrHandTracker.side, locateInfo->time, basePose, nullptr);

        // TODO: Implement proper hand tracking.
        locations->isActive = XR_FALSE;

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

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr
