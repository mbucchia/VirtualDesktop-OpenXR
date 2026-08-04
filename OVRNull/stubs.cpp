// MIT License
//
// Copyright(c) 2024-2025 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
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

using namespace ovrnull::log;

// OVR API that we stub out and not implement.

OVR_PUBLIC_FUNCTION(ovrResult) ovr_IdentifyClient(const char* identity) {
    // Nothing to do here.
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrHmdColorDesc) ovr_GetHmdColorDesc(ovrSession session) {
    ovrHmdColorDesc hmdColorDesc{};
    hmdColorDesc.ColorSpace = ovrColorSpace_Rec_709;
    return hmdColorDesc;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetClientColorDesc(ovrSession session, const ovrHmdColorDesc* colorDesc) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetClientColorDesc_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(unsigned int) ovr_GetTrackerCount(ovrSession session) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetTrackerCount_Unsupported");
    return 0;
}

OVR_PUBLIC_FUNCTION(ovrTrackerDesc) ovr_GetTrackerDesc(ovrSession session, unsigned int trackerDescIndex) {
    ovrTrackerDesc trackerDesc{};
    return trackerDesc;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_IsExtensionSupported(ovrSession session, ovrExtensions extension, ovrBool* outExtensionSupported) {
    if (outExtensionSupported) {
        *outExtensionSupported = false;
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_EnableExtension(ovrSession session, ovrExtensions extension) {
    TraceLoggingWrite(g_traceProvider, "OVR_EnableExtension_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_RecenterTrackingOrigin(ovrSession session) {
    TraceLoggingWrite(g_traceProvider, "OVR_RecenterTrackingOrigin_Unsupported");
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SpecifyTrackingOrigin(ovrSession session, ovrPosef originPose) {
    TraceLoggingWrite(g_traceProvider, "OVR_SpecifyTrackingOrigin_Unsupported");
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(void) ovr_ClearShouldRecenterFlag(ovrSession session) {
    TraceLoggingWrite(g_traceProvider, "OVR_ClearShouldRecenterFlag_Unsupported");
}

OVR_PUBLIC_FUNCTION(ovrTrackerPose) ovr_GetTrackerPose(ovrSession session, unsigned int trackerPoseIndex) {
    ovrTrackerPose nullTrackerPose{};
    return nullTrackerPose;
}

OVR_PUBLIC_FUNCTION(ovrTouchHapticsDesc)
ovr_GetTouchHapticsDesc(ovrSession session, ovrControllerType controllerType) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetTouchHapticsDesc_Unsupported");
    ovrTouchHapticsDesc nullDesc{};
    return nullDesc;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SubmitControllerVibration(ovrSession session, ovrControllerType controllerType, const ovrHapticsBuffer* buffer) {
    TraceLoggingWrite(g_traceProvider, "OVR_SubmitControllerVibration_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetControllerVibrationState(ovrSession session,
                                ovrControllerType controllerType,
                                ovrHapticsPlaybackState* outState) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetControllerVibrationState_Unsupported");
    if (outState) {
        *outState = {};
    }
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_TestBoundary(ovrSession session,
                 ovrTrackedDeviceType deviceBitmask,
                 ovrBoundaryType boundaryType,
                 ovrBoundaryTestResult* outTestResult) {
    TraceLoggingWrite(g_traceProvider, "OVR_TestBoundary_Unsupported");
    if (outTestResult) {
        *outTestResult = {};
    }
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_TestBoundaryPoint(ovrSession session,
                      const ovrVector3f* point,
                      ovrBoundaryType singleBoundaryType,
                      ovrBoundaryTestResult* outTestResult) {
    TraceLoggingWrite(g_traceProvider, "OVR_TestBoundaryPoint_Unsupported");
    if (outTestResult) {
        *outTestResult = {};
    }
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SetBoundaryLookAndFeel(ovrSession session, const ovrBoundaryLookAndFeel* lookAndFeel) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetBoundaryLookAndFeel_Unsupported");
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_ResetBoundaryLookAndFeel(ovrSession session) {
    TraceLoggingWrite(g_traceProvider, "OVR_ResetBoundaryLookAndFeel_Unsupported");
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetBoundaryGeometry(ovrSession session,
                        ovrBoundaryType boundaryType,
                        ovrVector3f* outFloorPoints,
                        int* outFloorPointsCount) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetBoundaryGeometry_Unsupported");
    if (outFloorPointsCount) {
        *outFloorPointsCount = 0;
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetBoundaryDimensions(ovrSession session, ovrBoundaryType boundaryType, ovrVector3f* outDimensions) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetBoundaryDimensions_Unsupported");
    if (outDimensions) {
        *outDimensions = {};
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetBoundaryVisible(ovrSession session, ovrBool* outIsVisible) {
    if (outIsVisible) {
        *outIsVisible = false;
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_RequestBoundaryVisible(ovrSession session, ovrBool visible) {
    TraceLoggingWrite(g_traceProvider, "OVR_RequestBoundaryVisible_Unsupported");
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetExternalCameras(ovrSession session, ovrExternalCamera* cameras, unsigned int* inoutCameraCount) {
    if (inoutCameraCount) {
        *inoutCameraCount = 0;
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SetExternalCameraProperties(ovrSession session,
                                const char* name,
                                const ovrCameraIntrinsics* const intrinsics,
                                const ovrCameraExtrinsics* const extrinsics) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetExternalCameraProperties_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrEyeRenderDesc) ovr_GetRenderDesc2(ovrSession session, ovrEyeType eyeType, ovrFovPort fov) {
    // redirected
    return ovr_GetRenderDesc(session, eyeType, fov);
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_SubmitFrame2(ovrSession session,
                 long long frameIndex,
                 const ovrViewScaleDesc* viewScaleDesc,
                 ovrLayerHeader const* const* layerPtrList,
                 unsigned int layerCount) {
    // redirected
    return ovr_SubmitFrame(session, frameIndex, viewScaleDesc, layerPtrList, layerCount);
}

OVR_PUBLIC_FUNCTION(int) ovr_GetInt(ovrSession session, const char* propertyName, int defaultVal) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetInt_Unsupported", TLArg(propertyName, "Property"));
    return defaultVal;
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetInt(ovrSession session, const char* propertyName, int value) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetInt_Unsupported", TLArg(propertyName, "Property"));
    return ovrFalse;
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetFloat(ovrSession session, const char* propertyName, float value) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetFloat_Unsupported", TLArg(propertyName, "Property"));
    return ovrFalse;
}

OVR_PUBLIC_FUNCTION(unsigned int)
ovr_GetFloatArray(ovrSession session, const char* propertyName, float values[], unsigned int valuesCapacity) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetFloatArray_Unsupported", TLArg(propertyName, "Property"));
    return 0;
}

OVR_PUBLIC_FUNCTION(ovrBool)
ovr_SetFloatArray(ovrSession session, const char* propertyName, const float values[], unsigned int valuesSize) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetFloatArray_Unsupported", TLArg(propertyName, "Property"));
    return ovrFalse;
}

OVR_PUBLIC_FUNCTION(const char*)
ovr_GetString(ovrSession session, const char* propertyName, const char* defaultVal) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetString_Unsupported", TLArg(propertyName, "Property"));
    return defaultVal;
}

OVR_PUBLIC_FUNCTION(ovrBool) ovr_SetString(ovrSession session, const char* propertyName, const char* value) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetString_Unsupported", TLArg(propertyName, "Property"));
    return ovrFalse;
}

struct ovrViewportStencilDesc_;
typedef struct ovrViewportStencilDesc_ ovrViewportStencilDesc;
OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetViewportStencil(ovrSession session,
                       const ovrViewportStencilDesc* viewportStencilDesc,
                       ovrFovStencilMeshBuffer* meshBuffer) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetViewportStencil_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceOutWaveId(UINT* deviceOutId) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetAudioDeviceOutWaveId_Unsupported");
    if (deviceOutId) {
        *deviceOutId = 0;
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceInWaveId(UINT* deviceInId) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetAudioDeviceInWaveId_Unsupported");
    if (deviceInId) {
        *deviceInId = 0;
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetAudioDeviceOutGuidStr(WCHAR deviceOutStrBuffer[OVR_AUDIO_MAX_DEVICE_STR_SIZE]) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetAudioDeviceOutGuidStr_Unsupported");
    ZeroMemory(deviceOutStrBuffer, sizeof(deviceOutStrBuffer));
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceOutGuid(GUID* deviceOutGuid) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetAudioDeviceOutGuid_Unsupported");
    if (deviceOutGuid) {
        *deviceOutGuid = {};
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceInGuidStr(WCHAR deviceInStrBuffer[OVR_AUDIO_MAX_DEVICE_STR_SIZE]) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetAudioDeviceInGuidStr_Unsupported");
    ZeroMemory(deviceInStrBuffer, sizeof(deviceInStrBuffer));
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetAudioDeviceInGuid(GUID* deviceInGuid) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetAudioDeviceInGuid_Unsupported");
    if (deviceInGuid) {
        *deviceInGuid = {};
    }
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CreateMirrorTextureDX(ovrSession session,
                          IUnknown* d3dPtr,
                          const ovrMirrorTextureDesc* desc,
                          ovrMirrorTexture* out_MirrorTexture) {
    // redirected
    return ovr_CreateMirrorTextureWithOptionsDX(session, d3dPtr, desc, out_MirrorTexture);
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CreateTextureSwapChainGL(ovrSession session,
                             const ovrTextureSwapChainDesc* desc,
                             ovrTextureSwapChain* out_TextureSwapChain) {
    TraceLoggingWrite(g_traceProvider, "OVR_CreateTextureSwapChainGL_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetTextureSwapChainBufferGL(ovrSession session, ovrTextureSwapChain chain, int index, unsigned int* out_TexId) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetTextureSwapChainBufferGL_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CreateMirrorTextureWithOptionsGL(ovrSession session,
                                     const ovrMirrorTextureDesc* desc,
                                     ovrMirrorTexture* out_MirrorTexture) {
    TraceLoggingWrite(g_traceProvider, "OVR_CreateMirrorTextureWithOptionsGL_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CreateMirrorTextureGL(ovrSession session, const ovrMirrorTextureDesc* desc, ovrMirrorTexture* out_MirrorTexture) {
    TraceLoggingWrite(g_traceProvider, "OVR_CreateMirrorTextureGL_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetMirrorTextureBufferGL(ovrSession session, ovrMirrorTexture mirrorTexture, unsigned int* out_TexId) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetMirrorTextureBufferGL_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetInstanceExtensionsVk(ovrGraphicsLuid luid, char* extensionNames, uint32_t* inoutExtensionNamesSize) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetInstanceExtensionsVk_Unsupported");
    if (extensionNames) {
        extensionNames[0] = 0;
    }
    *inoutExtensionNamesSize = 0;
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetDeviceExtensionsVk(ovrGraphicsLuid luid, char* extensionNames, uint32_t* inoutExtensionNamesSize) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetDeviceExtensionsVk_Unsupported");
    if (extensionNames) {
        extensionNames[0] = 0;
    }
    *inoutExtensionNamesSize = 0;
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetSessionPhysicalDeviceVk(ovrSession session,
                               ovrGraphicsLuid luid,
                               VkInstance instance,
                               VkPhysicalDevice* out_physicalDevice) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetSessionPhysicalDeviceVk_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetSynchronizationQueueVk(ovrSession session, VkQueue queue) {
    TraceLoggingWrite(g_traceProvider, "OVR_SetSynchronizationQueueVk_Unsupported");
    return ovrError_Unsupported;
}

#undef ovr_SetSynchonizationQueueVk
OVR_PUBLIC_FUNCTION(ovrResult) ovr_SetSynchonizationQueueVk(ovrSession session, VkQueue queue) {
    // deprecated (typo), redirected to the correct function
    return ovr_SetSynchronizationQueueVk(session, queue);
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CreateTextureSwapChainVk(ovrSession session,
                             VkDevice device,
                             const ovrTextureSwapChainDesc* desc,
                             ovrTextureSwapChain* out_TextureSwapChain) {
    TraceLoggingWrite(g_traceProvider, "OVR_CreateTextureSwapChainVk_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetTextureSwapChainBufferVk(ovrSession session, ovrTextureSwapChain chain, int index, VkImage* out_Image) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetTextureSwapChainBufferVk_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_CreateMirrorTextureWithOptionsVk(ovrSession session,
                                     VkDevice device,
                                     const ovrMirrorTextureDesc* desc,
                                     ovrMirrorTexture* out_MirrorTexture) {
    TraceLoggingWrite(g_traceProvider, "OVR_CreateMirrorTextureWithOptionsVk_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_GetMirrorTextureBufferVk(ovrSession session, ovrMirrorTexture mirrorTexture, VkImage* out_Image) {
    TraceLoggingWrite(g_traceProvider, "OVR_GetMirrorTextureBufferVk_Unsupported");
    return ovrError_Unsupported;
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_ResetPerfStats(ovrSession session) {
    return ovrSuccess;
}

OVR_PUBLIC_FUNCTION(void)
ovr_ReportClientInfo(unsigned int compilerVersion,
                     int productVersion,
                     int majorVersion,
                     int minorVersion,
                     int patchVersion,
                     int buildNumber) {
    // Nothing to do here.
}

OVR_PUBLIC_FUNCTION(ovrResult)
ovr_Lookup(const char* name, void** data) {
    TraceLoggingWrite(g_traceProvider, "OVR_Lookup_Unsupported");
    return ovrError_Unsupported;
}
