// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
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

#include "version.h"

#ifndef RUNTIME_NAMESPACE
#error Must define RUNTIME_NAMESPACE
#endif

namespace RUNTIME_NAMESPACE {
    std::filesystem::path dllHome;

    // The path to store logs & others.
    std::filesystem::path programData;

    namespace log {
        // The file logger.
        std::ofstream logStream;
    } // namespace log
} // namespace RUNTIME_NAMESPACE

using namespace RUNTIME_NAMESPACE;
using namespace RUNTIME_NAMESPACE::log;

extern "C" {

XrVersion __declspec(dllexport) XRAPI_CALL getVersion() {
    return XR_MAKE_VERSION(RuntimeVersionMajor, RuntimeVersionMinor, RuntimeVersionPatch);
}

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

#ifndef STANDALONE_RUNTIME
    // This is the location for other Virtual Desktop logs.
    programData = std::filesystem::path(getenv("PROGRAMDATA")) / L"Virtual Desktop";
#else
    programData = std::filesystem::path(getenv("LOCALAPPDATA")) / L"VirtualDesktop-OpenXR";
#endif
    CreateDirectoryW(programData.wstring().c_str(), nullptr);

    // Start logging to file.
    if (!logStream.is_open()) {
        std::wstring logFile = (programData / ("OpenXR.log")).wstring();
        logStream.open(logFile, std::ios_base::ate);
    }

    Log("%s (%ls)\n", RuntimePrettyName.c_str(), dllHome.wstring().c_str());

#ifndef STANDALONE_RUNTIME
    {
        // Trampoline to the standalone/development runtime.
        const auto standaloneLibraryPath = RegGetString(HKEY_LOCAL_MACHINE, StandaloneRegPrefix, "redirect_to");
        HMODULE standaloneLibrary = nullptr;
        PFN_xrNegotiateLoaderRuntimeInterface pfnNegotiateLoaderRuntimeInterface = nullptr;
        decltype(getVersion)* pfnGetVersion = nullptr;
        if (standaloneLibraryPath && (standaloneLibrary = LoadLibraryW(standaloneLibraryPath.value().c_str())) &&
            (pfnNegotiateLoaderRuntimeInterface = (PFN_xrNegotiateLoaderRuntimeInterface)GetProcAddress(
                 standaloneLibrary, "xrNegotiateLoaderRuntimeInterface")) &&
            (pfnGetVersion = (decltype(getVersion)*)GetProcAddress(standaloneLibrary, "getVersion"))) {
            if (pfnGetVersion() >= getVersion()) {
                Log("Redirecting to standalone runtime (%ls)\n", standaloneLibraryPath.value().c_str());
                return pfnNegotiateLoaderRuntimeInterface(loaderInfo, runtimeRequest);
            } else {
                Log("Cancelled redirection to older standalone runtime.\n");
            }
        }
        if (standaloneLibrary) {
            FreeLibrary(standaloneLibrary);
        }
    }
#endif

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
