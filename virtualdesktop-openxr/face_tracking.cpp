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

// Implements the necessary support for the XR_FB_face_tracking extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_face_tracking

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace xr::math;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateFaceTrackerFB
    XrResult OpenXrRuntime::xrCreateFaceTrackerFB(XrSession session,
                                                  const XrFaceTrackerCreateInfoFB* createInfo,
                                                  XrFaceTrackerFB* faceTracker) {
        if (createInfo->type != XR_TYPE_FACE_TRACKER_CREATE_INFO_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider, "xrCreateFaceTrackerFB", TLXArg(session, "Session"));

        if (!has_XR_FB_face_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (createInfo->faceExpressionSet != XR_FACE_EXPRESSION_SET_DEFAULT_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        std::unique_lock lock(m_faceAndEyeTrackersMutex);

        FaceTracker& xrFaceTracker = *new FaceTracker;

        *faceTracker = (XrFaceTrackerFB)&xrFaceTracker;

        // Maintain a list of known trackers for validation.
        m_faceTrackers.insert(*faceTracker);

        TraceLoggingWrite(g_traceProvider, "xrCreateFaceTrackerFB", TLXArg(*faceTracker, "FaceTracker"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyFaceTrackerFB
    XrResult OpenXrRuntime::xrDestroyFaceTrackerFB(XrFaceTrackerFB faceTracker) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyFaceTrackerFB", TLXArg(faceTracker, "FaceTracker"));

        if (!has_XR_FB_face_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_faceAndEyeTrackersMutex);

        if (!m_faceTrackers.count(faceTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        FaceTracker* xrFaceTracker = (FaceTracker*)faceTracker;

        delete xrFaceTracker;
        m_faceTrackers.erase(faceTracker);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetFaceExpressionWeightsFB
    XrResult OpenXrRuntime::xrGetFaceExpressionWeightsFB(XrFaceTrackerFB faceTracker,
                                                         const XrFaceExpressionInfoFB* expressionInfo,
                                                         XrFaceExpressionWeightsFB* expressionWeights) {
        if (expressionInfo->type != XR_TYPE_FACE_EXPRESSION_INFO_FB ||
            expressionWeights->type != XR_TYPE_FACE_EXPRESSION_WEIGHTS_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetFaceExpressionWeightsFB",
                          TLXArg(faceTracker, "FaceTracker"),
                          TLArg(expressionInfo->time));

        if (!has_XR_FB_face_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_faceAndEyeTrackersMutex);

        if (!m_faceTrackers.count(faceTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (expressionWeights->weightCount != XR_FACE_EXPRESSION_COUNT_FB ||
            expressionWeights->confidenceCount != XR_FACE_CONFIDENCE_COUNT_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // Forward the state from the memory mapped file.
        if (m_faceState) {
            for (uint32_t i = 0; i < XR_FACE_EXPRESSION_COUNT_FB; i++) {
                expressionWeights->weights[i] = m_faceState->ExpressionWeights[i];
            }
            for (uint32_t i = 0; i < XR_FACE_CONFIDENCE_COUNT_FB; i++) {
                expressionWeights->confidences[i] = m_faceState->ExpressionConfidences[i];
            }
            expressionWeights->status.isValid = m_faceState->FaceIsValid ? XR_TRUE : XR_FALSE;
            expressionWeights->status.isEyeFollowingBlendshapesValid =
                m_faceState->IsEyeFollowingBlendshapesValid ? XR_TRUE : XR_FALSE;
        } else {
            for (uint32_t i = 0; i < XR_FACE_EXPRESSION_COUNT_FB; i++) {
                expressionWeights->weights[i] = 0.f;
            }
            for (uint32_t i = 0; i < XR_FACE_CONFIDENCE_COUNT_FB; i++) {
                expressionWeights->confidences[i] = 0.f;
            }
            expressionWeights->status.isValid = expressionWeights->status.isEyeFollowingBlendshapesValid = XR_FALSE;
        }

        // We do not do any extrapolation.
        expressionWeights->time = expressionInfo->time;

        TraceLoggingWrite(
            g_traceProvider,
            "xrGetEyeGazesFB",
            TLArg(!!expressionWeights->status.isValid, "Valid"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION_EYES_CLOSED_L_FB], "LeftEyeClosed"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION_EYES_CLOSED_R_FB], "RightEyeClosed"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION_MOUTH_LEFT_FB], "MouthToLeft"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION_MOUTH_RIGHT_FB], "MouthToRight"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION_JAW_DROP_FB], "JawDrop"),
            TLArg(expressionWeights->confidences[XR_FACE_CONFIDENCE_LOWER_FACE_FB], "ConfidenceLowerFace"),
            TLArg(expressionWeights->confidences[XR_FACE_CONFIDENCE_UPPER_FACE_FB], "ConfidenceUpperFace"),
            TLArg(!!expressionWeights->status.isEyeFollowingBlendshapesValid, "EyeFollowingBlendshapesValid"),
            TLArg(expressionWeights->time, "Time"));

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr