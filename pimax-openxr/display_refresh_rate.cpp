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

// Implements the mock support for the XR_FB_display_refresh_rate extension:
// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#XR_FB_display_refresh_rate

namespace pimax_openxr {

    using namespace pimax_openxr::log;
    using namespace pimax_openxr::utils;

    XrResult OpenXrRuntime::xrEnumerateDisplayRefreshRatesFB(XrSession session,
                                                             uint32_t displayRefreshRateCapacityInput,
                                                             uint32_t* displayRefreshRateCountOutput,
                                                             float* displayRefreshRates) {
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateDisplayRefreshRatesFB",
                          TLXArg(session, "Session"),
                          TLArg(displayRefreshRateCapacityInput, "displayRefreshRateCapacityInput"));

        if (!has_XR_FB_display_refresh_rate) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (displayRefreshRateCapacityInput && displayRefreshRateCapacityInput < 1) {
            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        *displayRefreshRateCountOutput = 1;
        TraceLoggingWrite(g_traceProvider,
                          "xrEnumerateDisplayRefreshRatesFB",
                          TLArg(*displayRefreshRateCountOutput, "DisplayRefreshRateCountOutput"));

        if (displayRefreshRateCapacityInput && displayRefreshRates) {
            displayRefreshRates[0] = m_displayRefreshRate;
            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateDisplayRefreshRatesFB",
                              TLArg(displayRefreshRates[0], "DisplayRefreshRate"));
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntime::xrGetDisplayRefreshRateFB(XrSession session, float* displayRefreshRate) {
        TraceLoggingWrite(g_traceProvider, "xrGetDisplayRefreshRateFB", TLXArg(session, "Session"));

        if (!has_XR_FB_display_refresh_rate) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        *displayRefreshRate = m_displayRefreshRate;

        TraceLoggingWrite(
            g_traceProvider, "xrGetDisplayRefreshRateFB", TLArg(*displayRefreshRate, "DisplayRefreshRate"));

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntime::xrRequestDisplayRefreshRateFB(XrSession session, float displayRefreshRate) {
        TraceLoggingWrite(g_traceProvider,
                          "xrRequestDisplayRefreshRateFB",
                          TLXArg(session, "Session"),
                          TLArg(displayRefreshRate, "DisplayRefreshRate"));

        if (!has_XR_FB_display_refresh_rate) {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        if (!m_sessionCreated || session != (XrSession)1) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (std::abs(displayRefreshRate - m_displayRefreshRate) > FLT_EPSILON) {
            return XR_ERROR_DISPLAY_REFRESH_RATE_UNSUPPORTED_FB;
        }

        return XR_SUCCESS;
    }

} // namespace pimax_openxr
