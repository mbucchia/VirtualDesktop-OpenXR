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

// Implements the necessary support for the XR_OCULUS_audio_device_guid extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_OCULUS_audio_device_guid

namespace virtualdesktop_openxr {

    using namespace virtualdesktop_openxr::log;
    using namespace virtualdesktop_openxr::utils;
    using namespace DirectX;

    XrResult OpenXrRuntime::xrGetAudioOutputDeviceGuidOculus(XrInstance instance,
                                                             wchar_t buffer[XR_MAX_AUDIO_DEVICE_STR_SIZE_OCULUS]) {
        TraceLoggingWrite(g_traceProvider, "xrGetAudioOutputDeviceGuidOculus", TLXArg(instance, "Instance"));

        if (!has_XR_OCULUS_audio_device_guid) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        CHECK_OVRCMD(ovr_GetAudioDeviceOutGuidStr(buffer));

        TraceLoggingWrite(g_traceProvider, "xrGetAudioOutputDeviceGuidOculus", TLArg(buffer, "Buffer"));

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntime::xrGetAudioInputDeviceGuidOculus(XrInstance instance,
                                                            wchar_t buffer[XR_MAX_AUDIO_DEVICE_STR_SIZE_OCULUS]) {
        TraceLoggingWrite(g_traceProvider, "xrGetAudioInputDeviceGuidOculus", TLXArg(instance, "Instance"));

        if (!has_XR_OCULUS_audio_device_guid) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        CHECK_OVRCMD(ovr_GetAudioDeviceInGuidStr(buffer));

        TraceLoggingWrite(g_traceProvider, "xrGetAudioInputDeviceGuidOculus", TLArg(buffer, "Buffer"));

        return XR_SUCCESS;
    }

} // namespace virtualdesktop_openxr