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

// Implements the necessary support for the XR_FB_face_tracking and XR_FB_face_tracking2 extensions:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_face_tracking
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_face_tracking2

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

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateFaceTrackerFB",
                          TLXArg(session, "Session"),
                          TLArg((uint32_t)createInfo->faceExpressionSet, "FaceExpressionSet"));

        if (!has_XR_FB_face_tracking) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_supportsFaceTracking) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        if (createInfo->faceExpressionSet != XR_FACE_EXPRESSION_SET_DEFAULT_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

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

        std::unique_lock lock(m_bodyTrackersMutex);

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

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_faceTrackers.count(faceTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (expressionWeights->weightCount != XR_FACE_EXPRESSION_COUNT_FB ||
            expressionWeights->confidenceCount != XR_FACE_CONFIDENCE_COUNT_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        // Forward the state from the memory mapped file.
        if (m_bodyState) {
            std::unique_lock lock(m_bodyStateMutex);

            for (uint32_t i = 0; i < XR_FACE_EXPRESSION_COUNT_FB; i++) {
                expressionWeights->weights[i] = m_cachedBodyState.ExpressionWeights[i];
            }
            for (uint32_t i = 0; i < XR_FACE_CONFIDENCE_COUNT_FB; i++) {
                expressionWeights->confidences[i] = m_cachedBodyState.ExpressionConfidences[i];
            }
            expressionWeights->status.isValid = m_cachedBodyState.FaceIsValid ? XR_TRUE : XR_FALSE;
            expressionWeights->status.isEyeFollowingBlendshapesValid =
                m_cachedBodyState.IsEyeFollowingBlendshapesValid ? XR_TRUE : XR_FALSE;
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
            "xrGetFaceExpressionWeightsFB",
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

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateFaceTracker2FB
    XrResult OpenXrRuntime::xrCreateFaceTracker2FB(XrSession session,
                                                   const XrFaceTrackerCreateInfo2FB* createInfo,
                                                   XrFaceTracker2FB* faceTracker) {
        if (createInfo->type != XR_TYPE_FACE_TRACKER_CREATE_INFO2_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrCreateFaceTracker2FB",
                          TLXArg(session, "Session"),
                          TLArg((uint32_t)createInfo->faceExpressionSet, "FaceExpressionSet"));

        if (!has_XR_FB_face_tracking2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!m_supportsFaceTracking) {
            return XR_ERROR_FEATURE_UNSUPPORTED;
        }

        if (createInfo->faceExpressionSet != XR_FACE_EXPRESSION_SET2_DEFAULT_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        FaceTracker& xrFaceTracker = *new FaceTracker;
        const std::vector<XrFaceTrackingDataSource2FB> sources(
            createInfo->requestedDataSources, createInfo->requestedDataSources + createInfo->requestedDataSourceCount);
        xrFaceTracker.canUseVisualSource =
            std::find_if(sources.cbegin(), sources.cend(), [](const XrFaceTrackingDataSource2FB& source) {
                return source == XR_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB;
            }) != sources.cend();

        *faceTracker = (XrFaceTracker2FB)&xrFaceTracker;

        // Maintain a list of known trackers for validation.
        m_faceTrackers2.insert(*faceTracker);

        TraceLoggingWrite(g_traceProvider, "xrCreateFaceTracker2FB", TLXArg(*faceTracker, "FaceTracker"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroyFaceTracker2FB
    XrResult OpenXrRuntime::xrDestroyFaceTracker2FB(XrFaceTracker2FB faceTracker) {
        TraceLoggingWrite(g_traceProvider, "xrDestroyFaceTrackerFB", TLXArg(faceTracker, "FaceTracker"));

        if (!has_XR_FB_face_tracking2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_faceTrackers2.count(faceTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        FaceTracker* xrFaceTracker = (FaceTracker*)faceTracker;

        delete xrFaceTracker;
        m_faceTrackers2.erase(faceTracker);

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetFaceExpressionWeights2FB
    XrResult OpenXrRuntime::xrGetFaceExpressionWeights2FB(XrFaceTracker2FB faceTracker,
                                                          const XrFaceExpressionInfo2FB* expressionInfo,
                                                          XrFaceExpressionWeights2FB* expressionWeights) {
        if (expressionInfo->type != XR_TYPE_FACE_EXPRESSION_INFO2_FB ||
            expressionWeights->type != XR_TYPE_FACE_EXPRESSION_WEIGHTS2_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrGetFaceExpressionWeights2FB",
                          TLXArg(faceTracker, "FaceTracker"),
                          TLArg(expressionInfo->time));

        if (!has_XR_FB_face_tracking2) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        std::unique_lock lock(m_bodyTrackersMutex);

        if (!m_faceTrackers2.count(faceTracker)) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (expressionWeights->weightCount != XR_FACE_EXPRESSION2_COUNT_FB ||
            expressionWeights->confidenceCount != XR_FACE_CONFIDENCE2_COUNT_FB) {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        const FaceTracker& xrFaceTracker = *(FaceTracker*)faceTracker;

        // Forward the state from the memory mapped file.
        if (m_bodyState) {
            std::unique_lock lock(m_bodyStateMutex);

            for (uint32_t i = 0; i < XR_FACE_EXPRESSION2_COUNT_FB; i++) {
                expressionWeights->weights[i] = m_cachedBodyState.ExpressionWeights[i];
            }
            for (uint32_t i = 0; i < XR_FACE_CONFIDENCE2_COUNT_FB; i++) {
                expressionWeights->confidences[i] = m_cachedBodyState.ExpressionConfidences[i];
            }
            expressionWeights->isValid = m_cachedBodyState.FaceIsValid ? XR_TRUE : XR_FALSE;
            expressionWeights->isEyeFollowingBlendshapesValid =
                m_cachedBodyState.IsEyeFollowingBlendshapesValid ? XR_TRUE : XR_FALSE;
        } else {
            for (uint32_t i = 0; i < XR_FACE_EXPRESSION2_COUNT_FB; i++) {
                expressionWeights->weights[i] = 0.f;
            }
            for (uint32_t i = 0; i < XR_FACE_CONFIDENCE2_COUNT_FB; i++) {
                expressionWeights->confidences[i] = 0.f;
            }
            expressionWeights->isValid = expressionWeights->isEyeFollowingBlendshapesValid = XR_FALSE;
        }
        expressionWeights->dataSource = xrFaceTracker.canUseVisualSource ? XR_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB
                                                                         : XR_FACE_TRACKING_DATA_SOURCE2_AUDIO_FB;

        // We do not do any extrapolation.
        expressionWeights->time = expressionInfo->time;

        TraceLoggingWrite(
            g_traceProvider,
            "xrGetFaceExpressionWeights2FB",
            TLArg(!!expressionWeights->isValid, "Valid"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION2_EYES_CLOSED_L_FB], "LeftEyeClosed"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION2_EYES_CLOSED_R_FB], "RightEyeClosed"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION2_MOUTH_LEFT_FB], "MouthToLeft"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION2_MOUTH_RIGHT_FB], "MouthToRight"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION2_JAW_DROP_FB], "JawDrop"),
            TLArg(expressionWeights->weights[XR_FACE_EXPRESSION2_TONGUE_OUT_FB], "TongueOut"),
            TLArg(expressionWeights->confidences[XR_FACE_CONFIDENCE2_LOWER_FACE_FB], "ConfidenceLowerFace"),
            TLArg(expressionWeights->confidences[XR_FACE_CONFIDENCE2_UPPER_FACE_FB], "ConfidenceUpperFace"),
            TLArg(!!expressionWeights->isEyeFollowingBlendshapesValid, "EyeFollowingBlendshapesValid"),
            TLArg(expressionWeights->time, "Time"));

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr