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

namespace RUNTIME_NAMESPACE {
    std::filesystem::path dllHome;

    // The path to store logs & others.
    std::filesystem::path localAppData;

    namespace log {
        // The file logger.
        std::ofstream logStream;
    } // namespace log
} // namespace RUNTIME_NAMESPACE

using namespace RUNTIME_NAMESPACE;
using namespace RUNTIME_NAMESPACE::log;

extern "C" {

// Entry point for the loader.
XrResult __declspec(dllexport) XRAPI_CALL xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo* loaderInfo,
                                                                            XrNegotiateRuntimeRequest* runtimeRequest) {
    // Retrieve the path of the DLL.
    if (dllHome.empty()) {
        HMODULE module;
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)&dllHome,
                               &module)) {
            wchar_t path[_MAX_PATH];
            GetModuleFileNameW(module, path, sizeof(path));
            dllHome = std::filesystem::path(path).parent_path();
        } else {
            // Falling back to loading config/writing logs to the current working directory.
            DebugLog("Failed to locate DLL\n");
        }
    }

    localAppData = std::filesystem::path(getenv("LOCALAPPDATA")) / RuntimeName;
    CreateDirectoryA(localAppData.string().c_str(), nullptr);

    // Start logging to file.
    if (!logStream.is_open()) {
        std::string logFile = (localAppData / (RuntimeName + ".log")).string();
        logStream.open(logFile, std::ios_base::ate);
    }

    Log("%s\n", RuntimePrettyName.c_str());

    if (!loaderInfo || !runtimeRequest || loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
        runtimeRequest->structType != XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST ||
        runtimeRequest->structVersion != XR_RUNTIME_INFO_STRUCT_VERSION ||
        runtimeRequest->structSize != sizeof(XrNegotiateRuntimeRequest) ||
        loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxApiVersion < XR_CURRENT_API_VERSION || loaderInfo->minApiVersion > XR_CURRENT_API_VERSION) {
        Log("xrNegotiateLoaderRuntimeInterface validation failed\n");
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    // This is it! Tell the loader to use our API implementation.
    runtimeRequest->getInstanceProcAddr = xrGetInstanceProcAddr;
    runtimeRequest->runtimeInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
    runtimeRequest->runtimeApiVersion = XR_CURRENT_API_VERSION;

    return XR_SUCCESS;
}

}
