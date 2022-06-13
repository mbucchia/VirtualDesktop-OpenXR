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

// Implements the necessary support for the XR_KHR_win32_convert_performance_counter_time extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_win32_convert_performance_counter_time

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrConvertWin32PerformanceCounterToTimeKHR
    XrResult OpenXrRuntime::xrConvertWin32PerformanceCounterToTimeKHR(XrInstance instance,
                                                                      const LARGE_INTEGER* performanceCounter,
                                                                      XrTime* time) {
        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrConvertWin32PerformanceCounterToTimeKHR",
                          TLXArg(instance, "Instance"),
                          TLArg(performanceCounter->QuadPart, "PerformanceCounter"));

        double pvrTime = (double)performanceCounter->QuadPart / m_qpcFrequency.QuadPart;
        pvrTime += m_pvrTimeFromQpcTimeOffset;

        *time = pvrTimeToXrTime(pvrTime);

        TraceLoggingWrite(g_traceProvider, "xrConvertWin32PerformanceCounterToTimeKHR", TLArg(*time, "Time"));

        return XR_SUCCESS;
    }

    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrConvertTimeToWin32PerformanceCounterKHR
    XrResult OpenXrRuntime::xrConvertTimeToWin32PerformanceCounterKHR(XrInstance instance,
                                                                      XrTime time,
                                                                      LARGE_INTEGER* performanceCounter) {
        if (!m_instanceCreated || instance != (XrInstance)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        TraceLoggingWrite(g_traceProvider,
                          "xrConvertTimeToWin32PerformanceCounterKHR",
                          TLXArg(instance, "Instance"),
                          TLArg(time, "Time"));

        double pvrTime = xrTimeToPvrTime(time);
        pvrTime -= m_pvrTimeFromQpcTimeOffset;

        performanceCounter->QuadPart = (LONGLONG)(pvrTime * m_qpcFrequency.QuadPart);

        TraceLoggingWrite(g_traceProvider,
                          "xrConvertTimeToWin32PerformanceCounterKHR",
                          TLArg(performanceCounter->QuadPart, "PerformanceCounter"));

        return XR_SUCCESS;
    }

} // namespace pimax_openxr
