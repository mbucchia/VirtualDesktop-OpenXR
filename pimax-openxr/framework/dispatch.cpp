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

#include <runtime.h>

#include "dispatch.h"
#include "log.h"

#ifndef RUNTIME_NAMESPACE
#error Must define RUNTIME_NAMESPACE
#endif

using namespace RUNTIME_NAMESPACE::log;

namespace RUNTIME_NAMESPACE {

    // Handle cleanup of the layer's singleton.
    XrResult XRAPI_CALL xrDestroyInstance(XrInstance instance) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "xrDestroyInstance");

        XrResult result;
        try {
            result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyInstance(instance);
            if (XR_SUCCEEDED(result)) {
                RUNTIME_NAMESPACE::ResetInstance();
            }
        } catch (std::exception& exc) {
            TraceLoggingWriteTagged(local, "xrDestroyInstance_Error", TLArg(exc.what(), "Error"));
            ErrorLog("xrDestroyInstance: %s\n", exc.what());
            result = XR_ERROR_RUNTIME_FAILURE;
        }

        TraceLoggingWriteStop(local, "xrDestroyInstance", TLArg(xr::ToCString(result), "Result"));
        if (XR_FAILED(result)) {
            ErrorLog("xrDestroyInstance failed with %s\n", xr::ToCString(result));
        }

        return result;
    }

    // Forward the xrGetInstanceProcAddr() call to the dispatcher.
    XrResult XRAPI_CALL xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
        TraceLocalActivity(local);
        TraceLoggingWriteStart(local, "xrGetInstanceProcAddr");

        XrResult result;
        try {
            result = RUNTIME_NAMESPACE::GetInstance()->xrGetInstanceProcAddr(instance, name, function);
        } catch (std::exception& exc) {
            TraceLoggingWriteTagged(local, "xrGetInstanceProcAddr_Error", TLArg(exc.what(), "Error"));
            ErrorLog("xrGetInstanceProcAddr: %s\n", exc.what());
            result = XR_ERROR_RUNTIME_FAILURE;
        }

        TraceLoggingWriteStop(local, "xrGetInstanceProcAddr", TLArg(xr::ToCString(result), "Result"));
        if (XR_FAILED(result) && result != XR_ERROR_FUNCTION_UNSUPPORTED) {
            ErrorLog("xrGetInstanceProcAddr failed with %s\n", xr::ToCString(result));
        }

        return result;
    }

} // namespace RUNTIME_NAMESPACE
