// MIT License
//
// Copyright(c) 2022-2026 Matthieu Bucchianeri
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

#pragma once

#include "pch.h"

namespace ovrnull::log {

    TRACELOGGING_DECLARE_PROVIDER(g_traceProvider);

    extern TraceLoggingActivity<g_traceProvider> g_traceGlobal;

#define IsTraceEnabled() TraceLoggingProviderEnabled(g_traceProvider, 0, 0)

#define TraceLocalActivity(activity) TraceLoggingActivity<g_traceProvider> activity;

#define TLArg(var, ...) TraceLoggingValue(var, ##__VA_ARGS__)
#define TLPArg(var, ...) TraceLoggingPointer(var, ##__VA_ARGS__)
#ifdef _M_IX86
#define TLXArg TLArg
#else
#define TLXArg TLPArg
#endif
#define TLPArray(var, count, ...) TraceLoggingCodePointerArray((void**)var, (UINT16)count, ##__VA_ARGS__)

#define CHK_STRINGIFY(x) #x
#define TOSTRING(x) CHK_STRINGIFY(x)
#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)

    inline std::string _Fmt(const char* fmt, ...) {
        va_list vl;
        va_start(vl, fmt);
        int size = std::vsnprintf(nullptr, 0, fmt, vl);
        va_end(vl);

        if (size != -1) {
            std::unique_ptr<char[]> buffer(new char[size + 1]);

            va_start(vl, fmt);
            size = std::vsnprintf(buffer.get(), size + 1, fmt, vl);
            va_end(vl);
            if (size != -1) {
                return std::string(buffer.get(), size);
            }
        }

        throw std::runtime_error("Unexpected vsnprintf failure");
    }

    [[noreturn]] inline void _Throw(std::string failureMessage,
                                    const char* originator = nullptr,
                                    const char* sourceLocation = nullptr) {
        if (originator != nullptr) {
            failureMessage += _Fmt("\n    Origin: %s", originator);
        }
        if (sourceLocation != nullptr) {
            failureMessage += _Fmt("\n    Source: %s", sourceLocation);
        }

        throw std::logic_error(failureMessage);
    }

} // namespace ovrnull::log
