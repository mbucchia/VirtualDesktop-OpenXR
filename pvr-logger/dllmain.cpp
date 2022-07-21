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

// {cbf3adcd-42b1-4c38-830b-91980af201f6}
TRACELOGGING_DEFINE_PROVIDER(g_traceProvider,
                             "PimaxOpenXR",
                             (0xcbf3adcd, 0x42b1, 0x4c38, 0x83, 0x0b, 0x91, 0x98, 0x0a, 0xf2, 0x01, 0xf6));

TraceLoggingActivity<g_traceProvider> g_traceActivity;

#define TraceLocalActivity(activity) TraceLoggingActivity<g_traceProvider> activity;

#define TLArg(var, ...) TraceLoggingValue(var, ##__VA_ARGS__)
#define TLPArg(var, ...) TraceLoggingPointer(var, ##__VA_ARGS__)

namespace util {
    static inline std::string ToString(const pvrPosef& pose) {
        return fmt::format("p: ({:.3f}, {:.3f}, {:.3f}), o:({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                           pose.Position.x,
                           pose.Position.y,
                           pose.Position.z,
                           pose.Orientation.x,
                           pose.Orientation.y,
                           pose.Orientation.z,
                           pose.Orientation.w);
    }

    static inline std::string ToString(const pvrVector2f& vec) {
        return fmt::format("({:.3f}, {:.3f})", vec.x, vec.y);
    }

    static inline std::string ToString(const pvrViewPort& viewport) {
        return fmt::format("p: ({}, {}), s: ({}, {})", viewport.x, viewport.y, viewport.width, viewport.height);
    }

    static inline std::string ToString(const pvrFovPort& fov) {
        return fmt::format(
            "u: {:.3f}, d: {:.3f}, l: {:.3f}, r: {:.3f}", fov.UpTan, fov.DownTan, fov.LeftTan, fov.RightTan);
    }

    static inline std::string ToString(const pvrDepthProjectionDesc& depthProj) {
        return fmt::format("22: {:.3f}, 23: {:.3f}, 32: {:.3f}",
                           depthProj.Projection22,
                           depthProj.Projection23,
                           depthProj.Projection32);
    }

    static inline std::string ToString(pvrTrackedDeviceType type) {
        switch (type) {
        case pvrTrackedDevice_HMD:
            return "HMD";
        case pvrTrackedDevice_LeftController:
            return "LeftController";
        case pvrTrackedDevice_RightController:
            return "RightController";
        default:
            return fmt::format("pvrTrackedDeviceType_{}", type);
        }
    }

    static inline std::string ToString(pvrResult result) {
        switch (result) {
        case pvr_success:
            return "Success";
        case pvr_failed:
            return "Failed";
        case pvr_dll_failed:
            return "DLL Failed";
        case pvr_dll_wrong:
            return "DLL Wrong";
        case pvr_interface_not_found:
            return "Interface not found";
        case pvr_invalid_param:
            return "Invalid Parameter";
        case pvr_rpc_failed:
            return "RPC Failed";
        case pvr_share_mem_failed:
            return "Share Memory Failed";
        case pvr_unsupport_render_name:
            return "Unsupported Render Name";
        case pvr_no_display:
            return "No Display";
        case pvr_no_render_device:
            return "No Render Device";
        case pvr_app_not_visible:
            return "App Not Visible";
        case pvr_srv_not_ready:
            return "Service Not Ready";
        case pvr_dll_srv_mismatch:
            return "DLL Mismatch";
        case pvr_app_adapter_mismatch:
            return "App Adapter Mismatch";
        case pvr_not_support:
            return " Not Supported";

        default:
            return fmt::format("pvrResult_{}", result);
        }
    }
} // namespace util

namespace {
    using namespace util;

    std::mutex g_globalLock;
    wil::unique_hmodule g_realPvrLibrary;

    pvrInterface g_realPvrInterface{};
    bool g_realPvrInterfaceValid = false;
    pvrD3DInterface g_realPvrInterfaceD3D{};
    bool g_realPvrInterfaceD3DValid = false;

    pvrResult wrapper_initialise() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_initialize");
        const auto& result = g_realPvrInterface.initialise();
        TraceLoggingWriteStop(local, "PVR_initialize", TLArg(ToString(result).c_str(), "result"));

        return result;
    }

    void wrapper_shutdown() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_shutdown");
        g_realPvrInterface.shutdown();
        TraceLoggingWriteStop(local, "PVR_shutdown");
    }

    const char* wrapper_getVersionString() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getVersionString");
        const auto& result = g_realPvrInterface.getVersionString();
        TraceLoggingWriteStop(local, "PVR_getVersionString", TLArg(result));

        return result;
    }

    double wrapper_getTimeSeconds() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getTimeSeconds");
        const auto& result = g_realPvrInterface.getTimeSeconds();
        TraceLoggingWriteStop(local, "PVR_getTimeSeconds", TLArg(result));

        return result;
    }

    pvrResult wrapper_getTrackingState(pvrHmdHandle hmdh, double absTime, pvrTrackingState* state) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getTrackingState", TLArg(absTime));
        const auto& result = g_realPvrInterface.getTrackingState(hmdh, absTime, state);
        TraceLoggingWriteStop(local,
                              "PVR_getTrackingState",
                              TLArg(ToString(result).c_str(), "result"),
                              TLArg(state->HeadPose.StatusFlags, "HeadPose.StatusFlags"),
                              TLArg(ToString(state->HeadPose.ThePose).c_str(), "HeadPose.Pose"),
                              TLArg(state->HeadPose.TimeInSeconds, "HeadPose.Time"));

        return result;
    }

    pvrResult wrapper_getTrackedDevicePoseState(pvrHmdHandle hmdh,
                                                pvrTrackedDeviceType device,
                                                double absTime,
                                                pvrPoseStatef* state) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(
            local, "PVR_getTrackedDevicePoseState", TLArg(ToString(device).c_str(), "device"), TLArg(absTime));
        const auto& result = g_realPvrInterface.getTrackedDevicePoseState(hmdh, device, absTime, state);
        TraceLoggingWriteStop(local,
                              "PVR_getTrackedDevicePoseState",
                              TLArg(ToString(result).c_str(), "result"),
                              TLArg(state->StatusFlags, "Pose.StatusFlags"),
                              TLArg(ToString(state->ThePose).c_str(), "Pose.Pose"),
                              TLArg(state->TimeInSeconds, "Pose.Time"));

        return result;
    }

    pvrResult wrapper_createTextureSwapChainDX(pvrHmdHandle hmdh,
                                               IUnknown* d3dPtr,
                                               const pvrTextureSwapChainDesc* desc,
                                               pvrTextureSwapChain* out_TextureSwapChain) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local,
                               "PVR_createTextureSwapChainDX",
                               TLPArg(d3dPtr),
                               TLArg((int)desc->Type, "Desc.Type"),
                               TLArg((int)desc->Format, "Desc.Format"),
                               TLArg(desc->ArraySize, "Desc.ArraySize"),
                               TLArg(desc->Width, "Desc.Width"),
                               TLArg(desc->Height, "Desc.Height"),
                               TLArg(desc->MipLevels, "Desc.MipLevels"),
                               TLArg(desc->SampleCount, "Desc.SampleCount"),
                               TLArg(!!desc->StaticImage, "Desc.StaticImage"),
                               TLArg(desc->MiscFlags, "Desc.MiscFlags"),
                               TLArg(desc->BindFlags, "Desc.BindFlags"));
        const auto& result = g_realPvrInterfaceD3D.createTextureSwapChainDX(hmdh, d3dPtr, desc, out_TextureSwapChain);
        TraceLoggingWriteStop(local,
                              "PVR_createTextureSwapChainDX",
                              TLArg(ToString(result).c_str(), "result"),
                              TLPArg(*out_TextureSwapChain, "textureSwapChain"));

        return result;
    }

    void wrapper_destroyTextureSwapChain(pvrHmdHandle hmdh, pvrTextureSwapChain chain) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_destroyTextureSwapChain", TLPArg(chain));
        g_realPvrInterface.destroyTextureSwapChain(hmdh, chain);
        TraceLoggingWriteStop(local, "PVR_destroyTextureSwapChain");
    }

    pvrResult wrapper_getTextureSwapChainCurrentIndex(pvrHmdHandle hmdh, pvrTextureSwapChain chain, int* out_Index) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getTextureSwapChainCurrentIndex", TLPArg(chain));
        const auto& result = g_realPvrInterface.getTextureSwapChainCurrentIndex(hmdh, chain, out_Index);
        TraceLoggingWriteStop(local,
                              "PVR_getTextureSwapChainCurrentIndex",
                              TLArg(ToString(result).c_str(), "result"),
                              TLArg(*out_Index, "index"));

        return result;
    }

    pvrResult wrapper_commitTextureSwapChain(pvrHmdHandle hmdh, pvrTextureSwapChain chain) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_commitTextureSwapChain", TLPArg(chain));
        const auto& result = g_realPvrInterface.commitTextureSwapChain(hmdh, chain);
        TraceLoggingWriteStop(local, "PVR_commitTextureSwapChain", TLArg(ToString(result).c_str(), "result"));

        return result;
    }

    double wrapper_getPredictedDisplayTime(pvrHmdHandle hmdh, long long frameIndex) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getPredictedDisplayTime", TLArg(frameIndex));
        const auto& result = g_realPvrInterface.getPredictedDisplayTime(hmdh, frameIndex);
        TraceLoggingWriteStop(local, "PVR_getPredictedDisplayTime", TLArg(result));

        return result;
    }

    pvrResult wrapper_beginFrame(pvrHmdHandle hmdh, long long frameIndex) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_beginFrame", TLArg(frameIndex));
        const auto& result = g_realPvrInterface.beginFrame(hmdh, frameIndex);
        TraceLoggingWriteStop(local, "PVR_beginFrame", TLArg(ToString(result).c_str(), "result"));

        return result;
    }

    pvrResult wrapper_endFrame(pvrHmdHandle hmdh,
                               long long frameIndex,
                               pvrLayerHeader const* const* layerPtrList,
                               unsigned int layerCount) {
        TraceLocalActivity(local);

        // Frame rate counter for convenience.
        static std::deque<double> frameTimes;
        const auto now = g_realPvrInterface.getTimeSeconds();
        frameTimes.push_back(now);
        while (now - frameTimes.front() >= 1.0) {
            frameTimes.pop_front();
        }

        TraceLoggingWriteStart(
            local, "PVR_endFrame", TLArg(frameIndex), TLArg(layerCount), TLArg(frameTimes.size(), "Fps"));
        for (uint32_t i = 0; i < layerCount; i++) {
            const auto* const eyeFov = (pvrLayerEyeFov*)layerPtrList[i];
            const auto* const quad = (pvrLayerQuad*)layerPtrList[i];
            const auto* const eyeFovDepth = (pvrLayerEyeFovDepth*)layerPtrList[i];

            switch (layerPtrList[i]->Type) {
            case pvrLayerType_EyeFov:
            case pvrLayerType_EyeFovDepth:
                for (uint32_t eye = 0; eye < pvrEye_Count; eye++) {
                    TraceLoggingWriteTagged(
                        local,
                        "PVR_endFrame_Layer",
                        TLArg(layerPtrList[i]->Type == pvrLayerType_EyeFovDepth ? "EyeFovDepth" : "EyeFov", "Type"),
                        TLArg((int)eyeFov->Header.Flags, "Flags"));
                    if (layerPtrList[i]->Type == pvrLayerType_EyeFovDepth) {
                        TraceLoggingWriteTagged(
                            local,
                            "PVR_endFrame_LayerView",
                            TLArg(eye, "Eye"),
                            TLPArg(eyeFovDepth->ColorTexture[eye], "ColorTexture"),
                            TLArg(ToString(eyeFovDepth->RenderPose[eye]).c_str(), "RenderPose"),
                            TLArg(ToString(eyeFovDepth->Fov[eye]).c_str(), "Fov"),
                            TLPArg(eyeFovDepth->DepthTexture[eye], "DepthTexture"),
                            TLArg(ToString(eyeFovDepth->DepthProjectionDesc).c_str(), "DepthProjectionDesc"),
                            TLArg(ToString(eyeFovDepth->Viewport[eye]).c_str(), "Viewport"),
                            TLArg(eyeFovDepth->SensorSampleTime, "SensorSampleTime"));
                    } else {
                        TraceLoggingWriteTagged(local,
                                                "PVR_endFrame_LayerView",
                                                TLArg(eye, "Eye"),
                                                TLPArg(eyeFov->ColorTexture[eye], "ColorTexture"),
                                                TLArg(ToString(eyeFov->RenderPose[eye]).c_str(), "RenderPose"),
                                                TLArg(ToString(eyeFov->Fov[eye]).c_str(), "Fov"),
                                                TLArg(ToString(eyeFov->Viewport[eye]).c_str(), "Viewport"),
                                                TLArg(eyeFov->SensorSampleTime, "SensorSampleTime"));
                    }
                }
                break;

            case pvrLayerType_Quad:
                TraceLoggingWriteTagged(local,
                                        "PVR_endFrame_Layer",
                                        TLArg("Quad", "Type"),
                                        TLArg((int)quad->Header.Flags, "Flags"),
                                        TLPArg(quad->ColorTexture, "ColorTexture"),
                                        TLArg(ToString(quad->QuadPoseCenter).c_str(), "PoseCenter"),
                                        TLArg(ToString(quad->QuadSize).c_str(), "Size"),
                                        TLArg(ToString(quad->Viewport).c_str(), "Viewport"));
                break;

            default:
                TraceLoggingWriteTagged(local,
                                        "PVR_endFrame_Layer",
                                        TLArg(fmt::format("Unknown_{}", (int)layerPtrList[i]->Type).c_str(), "Type"));
                break;
            }
        }
        const auto& result = g_realPvrInterface.endFrame(hmdh, frameIndex, layerPtrList, layerCount);
        TraceLoggingWriteStop(local, "PVR_endFrame", TLArg(ToString(result).c_str(), "result"));

        return result;
    }

    void wrapper_logMessage(pvrLogLevel level, const char* message) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_logMessage", TLArg((int)level), TLArg(message));
        g_realPvrInterface.logMessage(level, message);
        TraceLoggingWriteStop(local, "PVR_logMessage");
    }

    // Entry point for patching the dispatch table for graphics calls.
    void* wrapper_getDxGlInterface(const char* api) {
        TraceLocalActivity(local);

        std::unique_lock lock(g_globalLock);

        void* result = nullptr;

        TraceLoggingWriteStart(local, "PVR_getDxGlInterface", TLArg(api));
        result = g_realPvrInterface.getDxGlInterface(api);
        if (result && std::string_view(api) == "dx") {
            pvrD3DInterface* asD3DInterface = (pvrD3DInterface*)result;

            // We can't return our own copy without breaking possible future versions. So we patch the pointers
            // in place.
            if (!g_realPvrInterfaceD3DValid) {
                g_realPvrInterfaceD3D = *asD3DInterface;
                g_realPvrInterfaceD3DValid = true;
            }

            asD3DInterface->createTextureSwapChainDX = wrapper_createTextureSwapChainDX;
        }
        TraceLoggingWriteStop(local, "PVR_getDxGlInterface", TLPArg(result));

        return result;
    }

    // Entry point for patching the dispatch table.
    pvrInterface* wrapper_getPvrInterface(uint32_t major_ver, uint32_t minor_ver) {
        TraceLocalActivity(local);

        std::unique_lock lock(g_globalLock);

        pvrInterface* result = nullptr;

        char modulePath[_MAX_PATH]{};
        GetModuleFileNameA(nullptr, modulePath, sizeof(modulePath));

        TraceLoggingWriteStart(local, "PVR_getInterface", TLArg(modulePath), TLArg(major_ver), TLArg(minor_ver));
        if (!g_realPvrLibrary) {
            *g_realPvrLibrary.put() = LoadLibraryA("real" PVRCLIENT_DLL_NAME);
        }
        if (g_realPvrLibrary) {
            const auto realGetPvrInterface =
                (getPvrInterface_Fn)(void*)GetProcAddress(g_realPvrLibrary.get(), PVR_GET_INTERFACE_FUNC_NAME);
            if (realGetPvrInterface) {
                result = realGetPvrInterface(major_ver, minor_ver);
                if (result) {
                    // We can't return our own copy without breaking possible future versions. So we patch the pointers
                    // in place.
                    if (!g_realPvrInterfaceValid) {
                        g_realPvrInterface = *result;
                        g_realPvrInterfaceValid = true;
                    }

                    result->initialise = wrapper_initialise;
                    result->shutdown = wrapper_shutdown;
                    result->getVersionString = wrapper_getVersionString;
                    result->getTimeSeconds = wrapper_getTimeSeconds;
                    result->getTrackingState = wrapper_getTrackingState;
                    result->getTrackedDevicePoseState = wrapper_getTrackedDevicePoseState;
                    result->destroyTextureSwapChain = wrapper_destroyTextureSwapChain;
                    result->getTextureSwapChainCurrentIndex = wrapper_getTextureSwapChainCurrentIndex;
                    result->commitTextureSwapChain = wrapper_commitTextureSwapChain;
                    result->getPredictedDisplayTime = wrapper_getPredictedDisplayTime;
                    result->beginFrame = wrapper_beginFrame;
                    result->endFrame = wrapper_endFrame;
                    result->logMessage = wrapper_logMessage;

                    // result->getDxGlInterface = wrapper_getDxGlInterface;
                }
            } else {
                TraceLoggingWriteTagged(local, "PVR_getInterface_GetProcAddress_Failed");
            }
        } else {
            TraceLoggingWriteTagged(local, "PVR_getInterface_LoadLibrary_Failed");
        }

        TraceLoggingWriteStop(local, "PVR_getInterface", TLPArg(result));

        return result;
    }

    // Functions we don't care to trace (yet).
#if 0
    pvrResult wrapper_createHmd(pvrHmdHandle* phmdh) {
    }

    void wrapper_destroyHmd(pvrHmdHandle hmdh) {
    }

    pvrResult wrapper_getHmdInfo(pvrHmdHandle hmdh, pvrHmdInfo* outInfo) {
    }

    pvrResult wrapper_getEyeDisplayInfo(pvrHmdHandle hmdh, pvrEyeType eye, pvrDisplayInfo* outInfo) {
    }

    pvrResult wrapper_getEyeRenderInfo(pvrHmdHandle hmdh, pvrEyeType eye, pvrEyeRenderInfo* outInfo) {
    }

    pvrResult wrapper_getHmdStatus(pvrHmdHandle hmdh, pvrHmdStatus* outStatus) {
    }

    pvrResult wrapper_setTrackingOriginType(pvrHmdHandle hmdh, pvrTrackingOrigin origin) {
    }

    pvrResult wrapper_getTrackingOriginType(pvrHmdHandle hmdh, pvrTrackingOrigin* origin) {
    }

    pvrResult wrapper_recenterTrackingOrigin(pvrHmdHandle hmdh) {
    }

    pvrResult wrapper_getTrackedDeviceCaps(pvrHmdHandle hmdh, pvrTrackedDeviceType device, uint32_t* pcap) {
    }

    pvrResult wrapper_getInputState(pvrHmdHandle hmdh, pvrInputState* inputState) {
    }

    float wrapper_getFloatConfig(pvrHmdHandle hmdh, const char* key, float def_val) {
    }

    pvrResult wrapper_setFloatConfig(pvrHmdHandle hmdh, const char* key, float val) {
    }

    int wrapper_getIntConfig(pvrHmdHandle hmdh, const char* key, int def_val) {
    }

    pvrResult wrapper_setIntConfig(pvrHmdHandle hmdh, const char* key, int val) {
    }

    int wrapper_getStringConfig(pvrHmdHandle hmdh, const char* key, char* val, int size) {
    }

    pvrResult wrapper_setStringConfig(pvrHmdHandle hmdh, const char* key, const char* val) {
    }

    pvrResult wrapper_getFovTextureSize(
        pvrHmdHandle hmdh, pvrEyeType eye, pvrFovPort fov, float pixelsPerDisplayPixel, pvrSizei* size) {
    }

    pvrResult wrapper_getHmdDistortedUV(pvrHmdHandle hmdh, pvrEyeType eye, pvrVector2f uv, pvrVector2f outUV[3]) {
    }

    pvrResult wrapper_getTextureSwapChainLength(pvrHmdHandle hmdh, pvrTextureSwapChain chain, int* out_Length) {
    }

    pvrResult wrapper_getTextureSwapChainDesc(pvrHmdHandle hmdh,
                                              pvrTextureSwapChain chain,
                                              pvrTextureSwapChainDesc* out_Desc) {
    }

    void wrapper_destroyMirrorTexture(pvrHmdHandle hmdh, pvrMirrorTexture mirrorTexture) {
    }

    void
    wrapper_Matrix4f_Projection(pvrFovPort fov, float znear, float zfar, pvrBool right_handled, pvrMatrix4f* outMat) {
    }

    void wrapper_Matrix4f_OrthoSubProjection(pvrMatrix4f projection,
                                             pvrVector2f orthoScale,
                                             float orthoDistance,
                                             float hmdToEyeOffsetX,
                                             pvrMatrix4f* outMat) {
    }

    void wrapper_calcEyePoses(pvrPosef headPose, const pvrPosef hmdToEyePose[2], pvrPosef outEyePoses[2]) {
    }

    void wrapper_Posef_FlipHandedness(const pvrPosef* inPose, pvrPosef* outPose) {
    }

    pvrDispStateType wrapper_getDisplayState(uint32_t edid_vid, uint32_t edid_pid) {
    }

    int wrapper_getClientCount() {
    }

    int wrapper_getClientPids(uint32_t pids[], int buf_count) {
    }

    unsigned int wrapper_getTrackerCount(pvrHmdHandle hmdh) {
    }

    pvrResult wrapper_getTrackerDesc(pvrHmdHandle hmdh, unsigned int idx, pvrTrackerDesc* desc) {
    }

    pvrResult wrapper_getTrackerPose(pvrHmdHandle hmdh, unsigned int idx, pvrTrackerPose* pose) {
    }

    pvrResult wrapper_triggerHapticPulse(pvrHmdHandle hmdh, pvrTrackedDeviceType device, float intensity) {
    }

    pvrVector3f wrapper_getVector3fConfig(pvrHmdHandle hmdh, const char* key, pvrVector3f def_val) {
    }

    pvrResult wrapper_setVector3fConfig(pvrHmdHandle hmdh, const char* key, pvrVector3f val) {
    }

    pvrQuatf wrapper_getQuatfConfig(pvrHmdHandle hmdh, const char* key, pvrQuatf def_val) {
    }

    pvrResult wrapper_setQuatfConfig(pvrHmdHandle hmdh, const char* key, pvrQuatf val) {
    }

    pvrResult wrapper_getConnectedDevices(pvrHmdHandle hmdh, uint32_t* pDevices) {
    }

    float wrapper_getTrackedDeviceFloatProperty(pvrHmdHandle hmdh,
                                                pvrTrackedDeviceType device,
                                                pvrTrackedDeviceProp prop,
                                                float def_val) {
    }

    int wrapper_getTrackedDeviceIntProperty(pvrHmdHandle hmdh,
                                            pvrTrackedDeviceType device,
                                            pvrTrackedDeviceProp prop,
                                            int def_val) {
    }

    int wrapper_getTrackedDeviceStringProperty(
        pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, char* val, int size) {
    }

    pvrVector3f wrapper_getTrackedDeviceVector3fProperty(pvrHmdHandle hmdh,
                                                         pvrTrackedDeviceType device,
                                                         pvrTrackedDeviceProp prop,
                                                         pvrVector3f def_val) {
    }

    pvrQuatf wrapper_getTrackedDeviceQuatfProperty(pvrHmdHandle hmdh,
                                                   pvrTrackedDeviceType device,
                                                   pvrTrackedDeviceProp prop,
                                                   pvrQuatf def_val) {
    }

    unsigned int wrapper_getEyeHiddenAreaMesh(pvrHmdHandle hmdh,
                                              pvrEyeType eye,
                                              pvrVector2f* outVertexBuffer,
                                              unsigned int bufferCount) {
    }

    int64_t wrapper_getInt64Config(pvrHmdHandle hmdh, const char* key, int64_t def_val) {
    }

    pvrResult wrapper_setInt64Config(pvrHmdHandle hmdh, const char* key, int64_t val) {
    }

    pvrResult wrapper_getSkeletalData(pvrHmdHandle hmdh,
                                      pvrTrackedDeviceType device,
                                      pvrSkeletalMotionRange range,
                                      pvrSkeletalData* data) {
    }

    pvrResult wrapper_getGripLimitSkeletalData(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrSkeletalData* data) {
    }

    int64_t wrapper_getTrackedDeviceInt64Property(pvrHmdHandle hmdh,
                                                  pvrTrackedDeviceType device,
                                                  pvrTrackedDeviceProp prop,
                                                  int64_t def_val) {
    }

    pvrResult wrapper_getEyeTrackingInfo(pvrHmdHandle hmdh, double absTime, pvrEyeTrackingInfo* outInfo) {
    }

    pvrResult wrapper_getTextureSwapChainBufferDX(
        pvrHmdHandle hmdh, pvrTextureSwapChain chain, int index, IID iid, void** out_Buffer) {
    }

    pvrResult wrapper_createMirrorTextureDX(pvrHmdHandle hmdh,
                                            IUnknown* d3dPtr,
                                            const pvrMirrorTextureDesc* desc,
                                            pvrMirrorTexture* out_MirrorTexture) {
    }

    pvrResult
    wrapper_getMirrorTextureBufferDX(pvrHmdHandle hmdh, pvrMirrorTexture mirrorTexture, IID iid, void** out_Buffer) {
    }
#endif
} // namespace

extern "C" __declspec(dllexport) pvrInterface* getPvrInterface(uint32_t major_ver, uint32_t minor_ver) {
    return wrapper_getPvrInterface(major_ver, minor_ver);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        TraceLoggingRegister(g_traceProvider);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
