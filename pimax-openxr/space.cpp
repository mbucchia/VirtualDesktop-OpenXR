// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
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

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateReferenceSpaces
    XrResult OpenXrRuntime::xrEnumerateReferenceSpaces(XrSession session,
                                                       uint32_t spaceCapacityInput,
                                                       uint32_t* spaceCountOutput,
                                                       XrReferenceSpaceType* spaces) {
        static const XrReferenceSpaceType referenceSpaces[] = {
            XR_REFERENCE_SPACE_TYPE_VIEW, XR_REFERENCE_SPACE_TYPE_LOCAL, XR_REFERENCE_SPACE_TYPE_STAGE};

        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateReferenceSpaces",
                          TLXArg(session, "Session"),
                          TLArg(spaceCapacityInput, "SpaceCapacityInput"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (spaceCapacityInput && spaceCapacityInput < ARRAYSIZE(referenceSpaces)) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *spaceCountOutput = ARRAYSIZE(referenceSpaces);
        TraceLoggingWrite(g_traceProvider, "xrEnumerateReferenceSpaces", TLArg(*spaceCountOutput, "SpaceCountOutput"));

        if (spaces) {
            for (uint32_t i = 0; i < *spaceCountOutput; i++) {
                spaces[i] = referenceSpaces[i];
                TraceLoggingWrite(
                    g_traceProvider, "xrEnumerateReferenceSpaces", TLArg(xr::ToCString(spaces[i]), "Space"));
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateReferenceSpace
    XrResult OpenXrRuntime::xrCreateReferenceSpace(XrSession session,
                                                   const XrReferenceSpaceCreateInfo* createInfo,
                                                   XrSpace* space) {
        if (createInfo->type != XR_TYPE_REFERENCE_SPACE_CREATE_INFO) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateReferenceSpace",
                          TLXArg(session, "Session"),
                          TLArg(xr::ToCString(createInfo->referenceSpaceType), "ReferenceSpaceType"),
                          TLArg(xr::ToString(createInfo->poseInReferenceSpace).c_str(), "PoseInReferenceSpace"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_VIEW &&
            createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_LOCAL &&
            createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_STAGE) {
            return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED;
        }

        // Create the internal struct.
        Space* xrSpace = new Space;
        xrSpace->referenceType = createInfo->referenceSpaceType;
        xrSpace->poseInSpace = createInfo->poseInReferenceSpace;

        *space = (XrSpace)xrSpace;

        // Maintain a list of known spaces for validation and cleanup.
        m_spaces.insert(*space);

        TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace", TLXArg(*space, "Space"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetReferenceSpaceBoundsRect
    XrResult OpenXrRuntime::xrGetReferenceSpaceBoundsRect(XrSession session,
                                                          XrReferenceSpaceType referenceSpaceType,
                                                          XrExtent2Df* bounds) {
        TraceLoggingWrite(g_traceProvider,
                          "xrGetReferenceSpaceBoundsRect",
                          TLXArg(session, "Session"),
                          TLArg(xr::ToCString(referenceSpaceType), "ReferenceSpaceType"));

        bounds->width = bounds->height = 0.f;

        return XR_SPACE_BOUNDS_UNAVAILABLE;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrLocateSpace
    XrResult OpenXrRuntime::xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) {
        if (location->type != XR_TYPE_SPACE_LOCATION) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrLocateSpace",
                          TLXArg(space, "Space"),
                          TLXArg(baseSpace, "BaseSpace"),
                          TLArg(time, "Time"));

        location->locationFlags = 0;

        // TODO: Do nothing for action spaces.
        if (space == (XrSpace)1 || baseSpace == (XrSpace)1) {
            return XR_SUCCESS;
        }

        if (!m_spaces.count(space) || !m_spaces.count(baseSpace)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Space* xrSpace = (Space*)space;
        Space* xrBaseSpace = (Space*)baseSpace;

        // Locate the HMD for view poses, otherwise use the origin.
        XrPosef pose = Pose::Identity();
        if ((xrSpace->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW ||
             xrBaseSpace->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW) &&
            xrSpace->referenceType != xrBaseSpace->referenceType) {
            pvrPoseStatef state{};
            CHECK_PVRCMD(
                pvr_getTrackedDevicePoseState(m_pvrSession, pvrTrackedDevice_HMD, xrTimeToPvrTime(time), &state));
            TraceLoggingWrite(g_traceProvider,
                              "PVR_HmdPoseState",
                              TLArg(state.StatusFlags, "StatusFlags"),
                              TLArg(xr::ToString(state.ThePose).c_str(), "Pose"));

            pose = pvrPoseToXrPose(state.ThePose);
            if (state.StatusFlags & pvrStatus_OrientationTracked) {
                location->locationFlags |=
                    (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT);
            }
            // For 9-axis setups, we propagate the Orientation bit to Position.
            if (state.StatusFlags & pvrStatus_PositionTracked || state.StatusFlags & pvrStatus_OrientationTracked) {
                location->locationFlags |=
                    (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT);
            }

            // If the space is stage and not local, add the height.
            if ((xrSpace->referenceType == XR_REFERENCE_SPACE_TYPE_STAGE ||
                 xrBaseSpace->referenceType == XR_REFERENCE_SPACE_TYPE_STAGE)) {
                pose.position.y += m_floorHeight;
            }

            // If the view is the reference, then we need the inverted pose.
            if (xrBaseSpace->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW) {
                StoreXrPose(&location->pose, LoadInvertedXrPose(location->pose));
            }
        } else {
            location->locationFlags =
                (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT |
                 XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT);

            // If the space is stage and not local, add the height.
            if ((xrSpace->referenceType == XR_REFERENCE_SPACE_TYPE_STAGE ||
                 xrBaseSpace->referenceType == XR_REFERENCE_SPACE_TYPE_STAGE)) {
                pose.position.y -= m_floorHeight;
            }
        }

        // Apply the offset transforms.
        StoreXrPose(&location->pose,
                    XMMatrixMultiply(LoadXrPose(xrSpace->poseInSpace),
                                     XMMatrixMultiply(LoadXrPose(pose), LoadInvertedXrPose(xrBaseSpace->poseInSpace))));

        TraceLoggingWrite(g_traceProvider,
                          "xrLocateSpace",
                          TLArg(location->locationFlags, "LocationFlags"),
                          TLArg(xr::ToString(location->pose).c_str(), "Pose"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrLocateViews
    XrResult OpenXrRuntime::xrLocateViews(XrSession session,
                                          const XrViewLocateInfo* viewLocateInfo,
                                          XrViewState* viewState,
                                          uint32_t viewCapacityInput,
                                          uint32_t* viewCountOutput,
                                          XrView* views) {
        if (viewLocateInfo->type != XR_TYPE_VIEW_LOCATE_INFO || viewState->type != XR_TYPE_VIEW_STATE) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrLocateViews",
                          TLXArg(session, "Session"),
                          TLArg(xr::ToCString(viewLocateInfo->viewConfigurationType), "ViewConfigurationType"),
                          TLArg(viewLocateInfo->displayTime, "DisplayTime"),
                          TLXArg(viewLocateInfo->space, "Space"),
                          TLArg(viewCapacityInput, "ViewCapacityInput"));

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (viewLocateInfo->viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
            return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
        }

        if (viewCapacityInput && viewCapacityInput < xr::StereoView::Count) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *viewCountOutput = xr::StereoView::Count;
        TraceLoggingWrite(g_traceProvider, "xrLocateViews", TLArg(*viewCountOutput, "ViewCountOutput"));

        if (views) {
            // Get the HMD pose in the base space.
            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
            CHECK_XRCMD(xrLocateSpace(m_viewSpace, viewLocateInfo->space, viewLocateInfo->displayTime, &location));
            viewState->viewStateFlags = location.locationFlags;

            if (viewState->viewStateFlags & (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT)) {
                // Calculate poses for each eye.
                pvrPosef hmdToEyePose[xr::StereoView::Count];
                hmdToEyePose[0] = m_cachedEyeInfo[0].HmdToEyePose;
                hmdToEyePose[1] = m_cachedEyeInfo[1].HmdToEyePose;
                if (m_useParallelProjection) {
                    // Eliminate canting.
                    hmdToEyePose[0].Orientation = PVR::Quatf::Identity();
                    hmdToEyePose[1].Orientation = PVR::Quatf::Identity();
                }

                pvrPosef eyePoses[xr::StereoView::Count]{{}, {}};
                pvr_calcEyePoses(m_pvr, xrPoseToPvrPose(location.pose), hmdToEyePose, eyePoses);

                for (uint32_t i = 0; i < *viewCountOutput; i++) {
                    if (views[i].type != XR_TYPE_VIEW) {
                        return XR_ERROR_VALIDATION_FAILURE;
                    }

                    views[i].pose = pvrPoseToXrPose(eyePoses[i]);
                    views[i].fov.angleDown = -atan(m_cachedEyeInfo[i].Fov.DownTan);
                    views[i].fov.angleUp = atan(m_cachedEyeInfo[i].Fov.UpTan);
                    views[i].fov.angleLeft = -atan(m_cachedEyeInfo[i].Fov.LeftTan);
                    views[i].fov.angleRight = atan(m_cachedEyeInfo[i].Fov.RightTan);

                    if (m_useParallelProjection) {
                        // Shift FOV by 10 degree. All Pimax headsets have a 10 degree canting.
                        const float angle = i == 0 ? -PVR::DegreeToRad(10.f) : PVR::DegreeToRad(10.f);
                        views[i].fov.angleLeft += angle;
                        views[i].fov.angleRight += angle;
                    }

                    TraceLoggingWrite(
                        g_traceProvider, "xrLocateViews", TLArg(viewState->viewStateFlags, "ViewStateFlags"));
                    TraceLoggingWrite(g_traceProvider,
                                      "xrLocateViews",
                                      TLArg(xr::ToString(views[i].pose).c_str(), "Pose"),
                                      TLArg(xr::ToString(views[i].fov).c_str(), "Fov"));
                }
            } else {
                // All or nothing.
                viewState->viewStateFlags = 0;
                TraceLoggingWrite(g_traceProvider, "xrLocateViews", TLArg(viewState->viewStateFlags, "ViewStateFlags"));
            }
        }

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroySpace
    XrResult OpenXrRuntime::xrDestroySpace(XrSpace space) {
        TraceLoggingWrite(g_traceProvider, "xrDestroySpace", TLXArg(space, "Space"));

        // TODO: Do nothing for action spaces.
        if (space == (XrSpace)1) {
            return XR_SUCCESS;
        }

        if (!m_spaces.count(space)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        Space* xrSpace = (Space*)space;
        delete xrSpace;

        m_spaces.erase(space);

        return XR_SUCCESS;
    }

} // namespace pimax_openxr
