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

#include "store.h"
#include "log.h"

namespace pimax_openxr::store {

    using namespace pimax_openxr::log;

    void storeAsyncInit() {
        const auto result = pvr_PlatformInit(10116220724823ull);
        if (result != pvrPlatformResult::pvrPlatformResult_Success) {
            TraceLoggingWrite(g_traceProvider, "PVR_Platform", TLArg((int)result, "Error"));
            // We just make this optional, this is only useful for users who downloaded PimaxXR from the Pimax Client.
        } else {
            TraceLoggingWrite(g_traceProvider, "PVR_Platform", TLArg("Login", "Action"));

            // Kick-off an entitlement check for compliance.
            pvr_CheckEntitlement();
        }

        bool running = true;
        while (running) {
            pvrMessageHandle message;
            while (running && (message = pvr_PollMessage())) {
                const auto messageType = pvr_Message_GetType(message);
                TraceLoggingWrite(g_traceProvider, "PVR_Platform", TLXArg((void*)messageType, "Message"));

                // Trace errors for good measure.
                if (pvr_Message_IsError(message)) {
                    TraceLoggingWrite(g_traceProvider,
                                      "PVR_Platform",
                                      TLArg(pvr_Message_GetErrorInfo(pvr_Message_GetError(message)), "Error"));
                }

                // Shutdown PVR platform on successful entitlement check or on any error.
                switch (messageType) {
                case pvrMessageType::pvrMessage_CheckEntitlement:
                    if (!pvr_Message_IsError(message)) {
                        TraceLoggingWrite(g_traceProvider,
                                          "PVR_Platform",
                                          TLArg((int)pvr_CheckEntitlement_GetResult(message), "Entitlement"));
                    }
                    running = false;
                    break;

                case pvrMessage_Notify_RuntimeError:
                    // The platform SDK does not seem to export this on 32-bit. It is misnamed "RunningError" instead.
                    if (!pvr_Message_IsError(message)) {
#ifdef _WIN64
                        TraceLoggingWrite(g_traceProvider,
                                          "PVR_Platform",
                                          TLArg((int)pvr_RuntimeError_GetError(message), "RuntimeError"));
#endif
                    }
                    running = false;
                    break;

                case pvrMessage_Notify_Logout:
                    if (!pvr_Message_IsError(message)) {
                        TraceLoggingWrite(g_traceProvider, "PVR_Platform", TLArg("Logout", "Action"));
                    }
                    running = false;
                    break;

                default:
                    break;
                }
            }

            // Yield the rest of the time.
            std::this_thread::sleep_for(100ms);
        }

        pvr_PlatformShutdown();
    }

} // namespace pimax_openxr::store
