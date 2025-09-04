#ifndef META_BODY_TRACKING_CALIBRATION_H_
#define META_BODY_TRACKING_CALIBRATION_H_ 1

/**********************
This file is @generated from the OpenXR XML API registry.
Language    :   C99
Copyright   :   (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
***********************/

#include <openxr/openxr.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XR_META_body_tracking_calibration

#define XR_META_body_tracking_calibration 1
#define XR_META_body_tracking_calibration_SPEC_VERSION 1
#define XR_META_BODY_TRACKING_CALIBRATION_EXTENSION_NAME "XR_META_body_tracking_calibration"
static const XrStructureType XR_TYPE_BODY_TRACKING_CALIBRATION_INFO_META = (XrStructureType) 1000283002;
static const XrStructureType XR_TYPE_BODY_TRACKING_CALIBRATION_STATUS_META = (XrStructureType) 1000283003;
static const XrStructureType XR_TYPE_SYSTEM_PROPERTIES_BODY_TRACKING_CALIBRATION_META = (XrStructureType) 1000283004;

typedef enum XrBodyTrackingCalibrationStateMETA {
    // Valid calibration, pose is safe to use
    XR_BODY_TRACKING_CALIBRATION_STATE_VALID_META = 1,
    // Calibration is still running, pose may be incorrect
    XR_BODY_TRACKING_CALIBRATION_STATE_CALIBRATING_META = 2,
    // Calibration is invalid, pose is not safe to use
    XR_BODY_TRACKING_CALIBRATION_STATE_INVALID_META = 3,
    XR_BODY_TRACKING_CALIBRATION_STATE_MAX_ENUM_META = 0x7FFFFFFF
} XrBodyTrackingCalibrationStateMETA;

// XrBodyTrackingCalibrationStatusMETA extends XrBodyJointLocationsFB
typedef struct XrBodyTrackingCalibrationStatusMETA {
    XrStructureType                       type;
    void* XR_MAY_ALIAS                    next;
    XrBodyTrackingCalibrationStateMETA    status;
} XrBodyTrackingCalibrationStatusMETA;

typedef struct XrBodyTrackingCalibrationInfoMETA {
    XrStructureType             type;
    const void* XR_MAY_ALIAS    next;
    float                       bodyHeight;
} XrBodyTrackingCalibrationInfoMETA;

typedef struct XrSystemPropertiesBodyTrackingCalibrationMETA {
    XrStructureType       type;
    void* XR_MAY_ALIAS    next;
    XrBool32              supportsHeightOverride;
} XrSystemPropertiesBodyTrackingCalibrationMETA;

typedef XrResult (XRAPI_PTR *PFN_xrSuggestBodyTrackingCalibrationOverrideMETA)(XrBodyTrackerFB bodyTracker, const XrBodyTrackingCalibrationInfoMETA* calibrationInfo);
typedef XrResult (XRAPI_PTR *PFN_xrResetBodyTrackingCalibrationMETA)(XrBodyTrackerFB bodyTracker);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES
XRAPI_ATTR XrResult XRAPI_CALL xrSuggestBodyTrackingCalibrationOverrideMETA(
    XrBodyTrackerFB                             bodyTracker,
    const XrBodyTrackingCalibrationInfoMETA*    calibrationInfo);

XRAPI_ATTR XrResult XRAPI_CALL xrResetBodyTrackingCalibrationMETA(
    XrBodyTrackerFB                             bodyTracker);
#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */
#endif /* XR_META_body_tracking_calibration */

#ifdef __cplusplus
}
#endif

#endif
