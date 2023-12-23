#ifndef FB_FACE_TRACKING2_H_
#define FB_FACE_TRACKING2_H_ 1

/**********************
This file is @generated from the OpenXR XML API registry.
Language    :   C99
Copyright   :   (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
***********************/

#include <openxr/openxr.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XR_FB_face_tracking2

#define XR_FB_face_tracking2 1
XR_DEFINE_HANDLE(XrFaceTracker2FB)
#define XR_FB_face_tracking2_SPEC_VERSION 1
#define XR_FB_FACE_TRACKING2_EXTENSION_NAME "XR_FB_face_tracking2"
// XrFaceTracker2FB
static const XrObjectType XR_OBJECT_TYPE_FACE_TRACKER2_FB = (XrObjectType) 1000287012;
static const XrStructureType XR_TYPE_SYSTEM_FACE_TRACKING_PROPERTIES2_FB = (XrStructureType) 1000287013;
static const XrStructureType XR_TYPE_FACE_TRACKER_CREATE_INFO2_FB = (XrStructureType) 1000287014;
static const XrStructureType XR_TYPE_FACE_EXPRESSION_INFO2_FB = (XrStructureType) 1000287015;
static const XrStructureType XR_TYPE_FACE_EXPRESSION_WEIGHTS2_FB = (XrStructureType) 1000287016;

typedef enum XrFaceExpression2FB {
    XR_FACE_EXPRESSION2_BROW_LOWERER_L_FB = 0,
    XR_FACE_EXPRESSION2_BROW_LOWERER_R_FB = 1,
    XR_FACE_EXPRESSION2_CHEEK_PUFF_L_FB = 2,
    XR_FACE_EXPRESSION2_CHEEK_PUFF_R_FB = 3,
    XR_FACE_EXPRESSION2_CHEEK_RAISER_L_FB = 4,
    XR_FACE_EXPRESSION2_CHEEK_RAISER_R_FB = 5,
    XR_FACE_EXPRESSION2_CHEEK_SUCK_L_FB = 6,
    XR_FACE_EXPRESSION2_CHEEK_SUCK_R_FB = 7,
    XR_FACE_EXPRESSION2_CHIN_RAISER_B_FB = 8,
    XR_FACE_EXPRESSION2_CHIN_RAISER_T_FB = 9,
    XR_FACE_EXPRESSION2_DIMPLER_L_FB = 10,
    XR_FACE_EXPRESSION2_DIMPLER_R_FB = 11,
    XR_FACE_EXPRESSION2_EYES_CLOSED_L_FB = 12,
    XR_FACE_EXPRESSION2_EYES_CLOSED_R_FB = 13,
    XR_FACE_EXPRESSION2_EYES_LOOK_DOWN_L_FB = 14,
    XR_FACE_EXPRESSION2_EYES_LOOK_DOWN_R_FB = 15,
    XR_FACE_EXPRESSION2_EYES_LOOK_LEFT_L_FB = 16,
    XR_FACE_EXPRESSION2_EYES_LOOK_LEFT_R_FB = 17,
    XR_FACE_EXPRESSION2_EYES_LOOK_RIGHT_L_FB = 18,
    XR_FACE_EXPRESSION2_EYES_LOOK_RIGHT_R_FB = 19,
    XR_FACE_EXPRESSION2_EYES_LOOK_UP_L_FB = 20,
    XR_FACE_EXPRESSION2_EYES_LOOK_UP_R_FB = 21,
    XR_FACE_EXPRESSION2_INNER_BROW_RAISER_L_FB = 22,
    XR_FACE_EXPRESSION2_INNER_BROW_RAISER_R_FB = 23,
    XR_FACE_EXPRESSION2_JAW_DROP_FB = 24,
    XR_FACE_EXPRESSION2_JAW_SIDEWAYS_LEFT_FB = 25,
    XR_FACE_EXPRESSION2_JAW_SIDEWAYS_RIGHT_FB = 26,
    XR_FACE_EXPRESSION2_JAW_THRUST_FB = 27,
    XR_FACE_EXPRESSION2_LID_TIGHTENER_L_FB = 28,
    XR_FACE_EXPRESSION2_LID_TIGHTENER_R_FB = 29,
    XR_FACE_EXPRESSION2_LIP_CORNER_DEPRESSOR_L_FB = 30,
    XR_FACE_EXPRESSION2_LIP_CORNER_DEPRESSOR_R_FB = 31,
    XR_FACE_EXPRESSION2_LIP_CORNER_PULLER_L_FB = 32,
    XR_FACE_EXPRESSION2_LIP_CORNER_PULLER_R_FB = 33,
    XR_FACE_EXPRESSION2_LIP_FUNNELER_LB_FB = 34,
    XR_FACE_EXPRESSION2_LIP_FUNNELER_LT_FB = 35,
    XR_FACE_EXPRESSION2_LIP_FUNNELER_RB_FB = 36,
    XR_FACE_EXPRESSION2_LIP_FUNNELER_RT_FB = 37,
    XR_FACE_EXPRESSION2_LIP_PRESSOR_L_FB = 38,
    XR_FACE_EXPRESSION2_LIP_PRESSOR_R_FB = 39,
    XR_FACE_EXPRESSION2_LIP_PUCKER_L_FB = 40,
    XR_FACE_EXPRESSION2_LIP_PUCKER_R_FB = 41,
    XR_FACE_EXPRESSION2_LIP_STRETCHER_L_FB = 42,
    XR_FACE_EXPRESSION2_LIP_STRETCHER_R_FB = 43,
    XR_FACE_EXPRESSION2_LIP_SUCK_LB_FB = 44,
    XR_FACE_EXPRESSION2_LIP_SUCK_LT_FB = 45,
    XR_FACE_EXPRESSION2_LIP_SUCK_RB_FB = 46,
    XR_FACE_EXPRESSION2_LIP_SUCK_RT_FB = 47,
    XR_FACE_EXPRESSION2_LIP_TIGHTENER_L_FB = 48,
    XR_FACE_EXPRESSION2_LIP_TIGHTENER_R_FB = 49,
    XR_FACE_EXPRESSION2_LIPS_TOWARD_FB = 50,
    XR_FACE_EXPRESSION2_LOWER_LIP_DEPRESSOR_L_FB = 51,
    XR_FACE_EXPRESSION2_LOWER_LIP_DEPRESSOR_R_FB = 52,
    XR_FACE_EXPRESSION2_MOUTH_LEFT_FB = 53,
    XR_FACE_EXPRESSION2_MOUTH_RIGHT_FB = 54,
    XR_FACE_EXPRESSION2_NOSE_WRINKLER_L_FB = 55,
    XR_FACE_EXPRESSION2_NOSE_WRINKLER_R_FB = 56,
    XR_FACE_EXPRESSION2_OUTER_BROW_RAISER_L_FB = 57,
    XR_FACE_EXPRESSION2_OUTER_BROW_RAISER_R_FB = 58,
    XR_FACE_EXPRESSION2_UPPER_LID_RAISER_L_FB = 59,
    XR_FACE_EXPRESSION2_UPPER_LID_RAISER_R_FB = 60,
    XR_FACE_EXPRESSION2_UPPER_LIP_RAISER_L_FB = 61,
    XR_FACE_EXPRESSION2_UPPER_LIP_RAISER_R_FB = 62,
    XR_FACE_EXPRESSION2_TONGUE_TIP_INTERDENTAL_FB = 63,
    XR_FACE_EXPRESSION2_TONGUE_TIP_ALVEOLAR_FB = 64,
    XR_FACE_EXPRESSION2_TONGUE_FRONT_DORSAL_PALATE_FB = 65,
    XR_FACE_EXPRESSION2_TONGUE_MID_DORSAL_PALATE_FB = 66,
    XR_FACE_EXPRESSION2_TONGUE_BACK_DORSAL_VELAR_FB = 67,
    XR_FACE_EXPRESSION2_TONGUE_OUT_FB = 68,
    XR_FACE_EXPRESSION2_TONGUE_RETREAT_FB = 69,
    XR_FACE_EXPRESSION2_COUNT_FB = 70,
    XR_FACE_EXPRESSION_2FB_MAX_ENUM_FB = 0x7FFFFFFF
} XrFaceExpression2FB;

typedef enum XrFaceExpressionSet2FB {
    // indicates that the created slink:XrFaceTracker2FB tracks the set of blend shapes described by elink:XrFaceExpression2FB enum, i.e. the flink:xrGetFaceExpressionWeights2FB function returns an array of blend shapes with the count of ename:XR_FACE_EXPRESSION2_COUNT_FB and can: be indexed using elink:XrFaceExpression2FB.
    XR_FACE_EXPRESSION_SET2_DEFAULT_FB = 0,
    XR_FACE_EXPRESSION_SET_2FB_MAX_ENUM_FB = 0x7FFFFFFF
} XrFaceExpressionSet2FB;

typedef enum XrFaceTrackingDataSource2FB {
    // face tracking uses visual data to estimate expressions. Face tracking may use audio to further improve the quality of face tracking.
    XR_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB = 0,
    // face tracking uses audio data to estimate expressions.
    XR_FACE_TRACKING_DATA_SOURCE2_AUDIO_FB = 1,
    XR_FACE_TRACKING_DATA_SOURCE_2FB_MAX_ENUM_FB = 0x7FFFFFFF
} XrFaceTrackingDataSource2FB;

typedef enum XrFaceConfidence2FB {
    XR_FACE_CONFIDENCE2_LOWER_FACE_FB = 0,
    XR_FACE_CONFIDENCE2_UPPER_FACE_FB = 1,
    XR_FACE_CONFIDENCE2_COUNT_FB = 2,
    XR_FACE_CONFIDENCE_2FB_MAX_ENUM_FB = 0x7FFFFFFF
} XrFaceConfidence2FB;
typedef struct XrSystemFaceTrackingProperties2FB {
    XrStructureType       type;
    void* XR_MAY_ALIAS    next;
    XrBool32              supportsVisualFaceTracking;
    XrBool32              supportsAudioFaceTracking;
} XrSystemFaceTrackingProperties2FB;

typedef struct XrFaceTrackerCreateInfo2FB {
    XrStructureType                 type;
    const void* XR_MAY_ALIAS        next;
    XrFaceExpressionSet2FB          faceExpressionSet;
    uint32_t                        requestedDataSourceCount;
    XrFaceTrackingDataSource2FB*    requestedDataSources;
} XrFaceTrackerCreateInfo2FB;

typedef struct XrFaceExpressionInfo2FB {
    XrStructureType             type;
    const void* XR_MAY_ALIAS    next;
    XrTime                      time;
} XrFaceExpressionInfo2FB;

typedef struct XrFaceExpressionWeights2FB {
    XrStructureType                type;
    void* XR_MAY_ALIAS             next;
    uint32_t                       weightCount;
    float*                         weights;
    uint32_t                       confidenceCount;
    float*                         confidences;
    XrBool32                       isValid;
    XrBool32                       isEyeFollowingBlendshapesValid;
    XrFaceTrackingDataSource2FB    dataSource;
    XrTime                         time;
} XrFaceExpressionWeights2FB;

typedef XrResult (XRAPI_PTR *PFN_xrCreateFaceTracker2FB)(XrSession session, const XrFaceTrackerCreateInfo2FB* createInfo, XrFaceTracker2FB* faceTracker);
typedef XrResult (XRAPI_PTR *PFN_xrDestroyFaceTracker2FB)(XrFaceTracker2FB faceTracker);
typedef XrResult (XRAPI_PTR *PFN_xrGetFaceExpressionWeights2FB)(XrFaceTracker2FB faceTracker, const XrFaceExpressionInfo2FB* expressionInfo, XrFaceExpressionWeights2FB* expressionWeights);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES
XRAPI_ATTR XrResult XRAPI_CALL xrCreateFaceTracker2FB(
    XrSession                                   session,
    const XrFaceTrackerCreateInfo2FB*           createInfo,
    XrFaceTracker2FB*                           faceTracker);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyFaceTracker2FB(
    XrFaceTracker2FB                            faceTracker);

XRAPI_ATTR XrResult XRAPI_CALL xrGetFaceExpressionWeights2FB(
    XrFaceTracker2FB                            faceTracker,
    const XrFaceExpressionInfo2FB*              expressionInfo,
    XrFaceExpressionWeights2FB*                 expressionWeights);
#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */
#endif /* XR_FB_face_tracking2 */

#ifdef __cplusplus
}
#endif

#endif
